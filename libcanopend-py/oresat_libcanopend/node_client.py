import logging
from dataclasses import dataclass
from enum import Enum
from threading import Lock, Thread
from typing import Any, Callable, Optional, Union
from time import sleep

import zmq

from .entry import Entry
from .message import (
    AbortErrorMessage,
    EmcySendMessage,
    OdReadMessage,
    OdWriteMessage,
    RequestOwnershipMessage,
    RequestPortMessage,
    SdoReadMessage,
    SdoWriteMessage,
    TpdoSendMessage,
    UnknownIdErrorMessage,
)


@dataclass
class LocalData:
    value: Union[int, float, str, bytes, None]
    owner: bool = False
    ownership_ack: bool = False  # reset on connection lost
    read_cb: Optional[Callable[[], [Any]]] = None
    write_cb: Optional[Callable[[Any], None]] = None


class NodeClient:
    RECV_TIMEOUT_MS = 1000

    def __init__(self, entries: Entry, addr: str = "localhost", debug: bool = True):
        self._data = {entry: LocalData(entry.default) for entry in list(entries)}
        self._lookup_entry = {(entry.index, entry.subindex): entry for entry in self._data.keys()}
        self._debug = debug
        self._addr = addr

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

        self._reply_port = 0
        self._reply_thread = Thread(target=self._reply_thread_run, daemon=True)
        self._command_socket.setsockopt(zmq.RCVTIMEO, self.RECV_TIMEOUT_MS)
        self._command_socket.setsockopt(zmq.LINGER, 0)
        self._reply_thread.start()

    def _consume_thread_run(self):
        while True:
            msg_recv = self._consume_socket.recv()
            if self._debug:
                logging.debug("CONSUME: " + msg_recv.hex().upper())
            if len(msg_recv) < OdWriteMessage.size or msg_recv[0] != OdWriteMessage.id:
                logging.debug(f"invalid od write message {msg_recv.hex().upper()}")
                continue

            try:
                msg_req = OdWriteMessage.unpack(msg_recv)
                entry = self._lookup_entry[(msg_req.index, msg_req.subindex)]
                value = entry.raw_to_value(msg_req.raw)
                if self._data[entry].write_cb is None:
                    self._data[entry].value = value
                else:
                    self._data[entry].write_cb(value)
            except Exception as e:
                logging.error(f"consume error {entry.name} {e}")

    def _reply_thread_run(self):
        req_msg = RequestPortMessage(self._reply_port)

        while True:
            try:
                res_msg = self._send_and_recv(req_msg)
                self._reply_port = res_msg.port
                break
            except Exception:
                sleep(1)

        reply_socket = self._context.socket(zmq.REP)
        reply_socket.connect(f"tcp://{self._addr}:{self._reply_port}")
        logging.info(f"connected to reply port {self._reply_port}")

        for entry, data in self._data.items():
            if data.owner and not data.ownership_ack:
                try:
                    req_msg = RequestOwnershipMessage(
                        entry.index, entry.subindex, data.read_cb is None, data.write_cb is None
                    )
                    res_msg = self._send_and_recv(req_msg)
                    if res_msg == req_msg:
                        data.ownership_ack = True
                        logging.info(f"got ownership of {entry.name}")
                    else:
                        logging.error(f"failed to get ownership of {entry.name}")
                except Exception:
                    pass

        while True:
            request = reply_socket.recv()
            if request is None or len(request) == 0:
                continue

            if self._debug:
                logging.debug("SERVER REQ: " + request.hex().upper())

            if request[0] == OdWriteMessage.id:
                try:
                    msg_req = OdWriteMessage.unpack(request)
                    entry = self._lookup_entry[(msg_req.index, msg_req.subindex)]
                    value = entry.raw_to_value(msg_req.raw)
                    if self._data[entry].read_cb is not None:
                        self._data[entry].write_cb(value)
                    else:
                        self._data[entry].value = value
                    reply = request
                except Exception as e:
                    logging.error(f"reply to od write error: {e}")
                    reply = AbortErrorMessage(0x0800_0020).pack()
            elif request[0] == OdReadMessage.id:
                try:
                    msg_req = OdReadMessage.unpack(request)
                    entry = self._lookup_entry[(msg_req.index, msg_req.subindex)]
                    if self._data[entry].read_cb is not None:
                        value = self._data[entry].read_cb()
                    else:
                        value = self._data[entry].value
                    reply = request + entry.value_to_raw(value)
                except Exception as e:
                    logging.error(f"reply to od read error: {e}")
                    reply = AbortErrorMessage(0x0800_0020).pack()
            else:
                reply = UnknownIdErrorMessage().pack()

            if self._debug:
                logging.debug("SERVER REPLY: " + reply.hex().upper())
            try:
                reply_socket.send(reply)
            except Exception:
                pass

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
        res_msg = self._send_and_recv(req_msg)
        value = entry.raw_to_value(res_msg.raw)

    def sdo_read(self, node_id: int, entry: Entry, use_enum: bool = True) -> Any:
        req_msg = SdoReadMessage(node_id, entry.index, entry.subindex, b"")
        res_msg = self._send_and_recv(req_msg)
        value = entry.raw_to_value(res_msg.raw)
        if use_enum and entry.enum and isinstance(value, int):
            value = entry.enum[value]
        return value

    def request_ownership(self, entry: Entry, read_cb=None, write_cb=None):
        self._data[entry].owner = True
        self._data[entry].read_cb = read_cb
        self._data[entry].write_cb = write_cb
        if self._reply_port != 0:
            try:
                req_msg = RequestOwnershipMessage(
                    entry.index, entry.subindex, read_cb is None, write_cb is None
                )
                res_msg = self._send_and_recv(req_msg)
                self._data[entry].ownership_ack = req_msg == res_msg
            except Exception:
                pass
