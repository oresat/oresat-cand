import os
import logging
from dataclasses import dataclass
from enum import Enum
from threading import Lock, Thread
from typing import Any, Callable, Optional, Union

import zmq
from zmq.utils.monitor import recv_monitor_message

from .entry import Entry
from .message import (
    EmcySendMessage,
    OdWriteMessage,
    SdoReadMessage,
    SdoWriteMessage,
    TpdoSendMessage,
    AddFileMessage,
    HbRecvMessage,
    EmcyRecvMessage,
)


class NodeState(Enum):
    INITIALIZING = 0x0
    STOPPED = 0x4
    OPERATIONAL = 0x5
    PRE_OPERATIONAL = 0x7F


@dataclass
class LocalData:
    value: Union[int, float, str, bytes, None]
    write_cb: Optional[Callable[[Any], None]] = None


class NodeClient:
    RECV_TIMEOUT_MS = 1000

    def __init__(self, entries: Entry, addr: str = "localhost", debug: bool = False):
        self._data = {entry: LocalData(entry.default) for entry in list(entries)}
        self._lookup_entry = {(entry.index, entry.subindex): entry for entry in self._data.keys()}
        self._debug = debug
        self._addr = addr

        self._emcy_cb = None
        self._hb_cb = None

        self._context = zmq.Context()

        self._command_socket = self._context.socket(zmq.REQ)
        self._command_socket.connect(f"tcp://{addr}:6000")
        self._command_socket.setsockopt(zmq.RCVTIMEO, self.RECV_TIMEOUT_MS)
        self._command_socket.setsockopt(zmq.LINGER, 0)
        self._command_socket_lock = Lock()

        self._consume_socket = self._context.socket(zmq.SUB)
        self._consume_socket.connect(f"tcp://{addr}:6001")
        self._consume_thread = Thread(target=self._consume_thread_run, daemon=True)
        self._consume_thread.start()

        self._broadcast_socket = self._context.socket(zmq.PUB)
        self._broadcast_socket.connect(f"tcp://{addr}:6002")

        self._monitor_socket = self._consume_socket.get_monitor_socket()
        self._monitor_thread = Thread(target=self._monitor_thread_run, daemon=True)
        self._monitor_thread.start()

    def _monitor_thread_run(self):
        while self._monitor_socket.poll():
            event = recv_monitor_message(self._monitor_socket)["event"]
            if event == zmq.EVENT_CONNECTED:
                logging.info("sockets connected")
                for entry, data in self._data.items():
                    if not data.write_cb:
                        continue
                    raw = entry.value_to_raw(data.value)
                    self._broadcast(OdWriteMessage(entry.index, entry.subindex, raw))
            elif event == zmq.EVENT_DISCONNECTED:
                logging.info("sockets disconnected")

    def _consume_thread_run(self):
        while True:
            msg_recv = self._consume_socket.recv()
            if self._debug:
                logging.debug("CONSUME: " + msg_recv.hex().upper())

            if msg_recv[0] != OdWriteMessage.id:
                if len(msg_recv) < OdWriteMessage.size:
                    logging.debug(f"invalid od write message {msg_recv.hex().upper()}")
                    continue

                try:
                    msg_req = OdWriteMessage.unpack(msg_recv)
                    entry = self._lookup_entry[(msg_req.index, msg_req.subindex)]
                    value = entry.raw_to_value(msg_req.raw)
                    if value == self._data[entry].value:
                        continue
                    self._data[entry].value = value
                    if self._data[entry].write_cb is not None:
                        self._data[entry].write_cb(value)
                except Exception as e:
                    logging.error(f"write callback error {entry.name} {e}")
            elif msg_recv[0] != HbRecvMessage.id:
                if self._hb_cb:
                    try:
                        msg_req = HbRecvMessage(msg_recv)
                        self._hb_cb(msg_req.node_id, NodeState(msg_req.state))
                    except Exception as e:
                        logging.error(f"heartbeat callback error: {e}")
            elif msg_recv[0] != EmcyRecvMessage.id:
                if self._emcy_cb:
                    try:
                        msg_req = EmcyRecvMessage(msg_recv)
                        self._emcy_cb(msg_req.node_id, msg_req.code, msg_req.info)
                    except Exception as e:
                        logging.error(f"emcy callback error: {e}")

    def _send_and_recv(self, req_msg):
        self._command_socket_lock.acquire()
        try:
            req_msg_raw = req_msg.pack()

            if self._debug:
                logging.debug(f"CLIENT SEND {len(req_msg_raw)}: {req_msg_raw.hex().upper()}")
            self._command_socket.send(req_msg_raw)

            res_msg_raw = self._command_socket.recv()
            if self._debug:
                logging.debug(f"CLIENT RECV {len(res_msg_raw)}: {res_msg_raw.hex().upper()}")

            res_msg = req_msg.unpack(res_msg_raw)
        except Exception as e:
            self._command_socket_lock.release()
            raise e
        self._command_socket_lock.release()
        return res_msg

    def _broadcast(self, msg):
        msg_raw = msg.pack()
        if self._debug:
            logging.debug("BROADCAST: " + msg_raw.hex().upper())
        try:
            self._broadcast_socket.send(msg_raw)
        except Exception:
            pass

    def send_emcy(self, code: int, info: int = 0):
        self._broadcast(EmcySendMessage(code, info))

    def send_tpdo(self, tpdo: Union[int, Enum, list[int], list[Enum]]):
        def _send_tpdo(num: Union[int, Enum]):
            if isinstance(num, Enum):
                num = num.value
            self._broadcast(TpdoSendMessage(num))

        if isinstance(tpdo, (int, Enum)):
            _send_tpdo(tpdo)
        else:
            for t in tpdo:
                _send_tpdo(t)

    def od_write(self, entry: Entry, value: Any):
        if isinstance(value, Enum) and entry.enum:
            value = value.value
        if not isinstance(value, entry.data_type.py_types):
            raise ValueError(f"value {value} ({type(value)}) invalid for {entry.data_type}")

        self._data[entry].value = value
        raw = entry.value_to_raw(value)
        self._broadcast(OdWriteMessage(entry.index, entry.subindex, raw))

    def od_write_multi(self, data: dict[Entry, Any]):
        for entry, value in data.items():
            self.od_write(entry, value)

    def od_read(self, entry: Entry, use_enum: bool = True) -> Any:
        value = self._data[entry].value

        if use_enum and entry.enum and isinstance(value, int):
            value = entry.enum[value]
        return value

    def sdo_write(self, node_id: int, entry: Entry, value: Any):
        raw = entry.value_to_raw(value)
        req_msg = SdoWriteMessage(node_id, entry.index, entry.subindex, raw)
        self._send_and_recv(req_msg)

    def sdo_read(self, node_id: int, entry: Entry, use_enum: bool = True) -> Any:
        req_msg = SdoReadMessage(node_id, entry.index, entry.subindex, b"")
        res_msg = self._send_and_recv(req_msg)
        value = entry.raw_to_value(res_msg.raw)
        if use_enum and entry.enum and isinstance(value, int):
            value = entry.enum[value]
        return value

    def add_write_callback(self, entry: Entry, write_cb: Callable[[Any], None]):
        if self._data[entry].write_cb is not None:
            raise ValueError(f"{entry.name} write callback is already set")
        self._data[entry].write_cb = write_cb

    def add_file(self, file_path: str):
        if file_path[0] != "/":
            raise ValueError("file_path must be an absolute path")
        if not os.path.isfile(file_path):
            raise FileNotFoundError(f"{file_path} not found")
        req_msg = AddFileMessage(file_path)
        self._send_and_recv(req_msg)

    def add_heartbeat_callback(self, hb_cb: Callable[[int, NodeState], None]):
        self._hb_cb = hb_cb

    def add_emcy_callback(self, emcy_cb: Callable[[int, int, int], None]):
        self._emcy_cb = emcy_cb
