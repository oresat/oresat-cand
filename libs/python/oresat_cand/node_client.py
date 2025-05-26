from __future__ import annotations

import logging
import os
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from threading import Lock, Thread
from time import sleep
from typing import Any, Callable

import zmq
from zmq.utils.monitor import recv_monitor_message

from .entry import Entry
from .message import (
    AddFileMessage,
    BusStateMessage,
    ConfigMessage,
    EmcyRecvMessage,
    EmcySendMessage,
    HbRecvMessage,
    OdWriteMessage,
    SdoListFilesMessage,
    SdoReadFileMessage,
    SdoReadMessage,
    SdoWriteFileMessage,
    SdoWriteMessage,
    SyncSendMessage,
    TpdoSendMessage,
)

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)


class NodeState(Enum):
    INITIALIZING = 0x0
    STOPPED = 0x4
    OPERATIONAL = 0x5
    PRE_OPERATIONAL = 0x7F


class BusState(Enum):
    NOT_FOUND = 0
    DOWN = 1
    UP = 2


@dataclass
class LocalData:
    value: int | float | str | bytes | None
    write_cb: Callable[[Any], None] | None = None


class NodeClientBase:
    RECV_TIMEOUT_MS = 1000

    def __init__(self, entries: Entry, addr: str, od_config_path: str | Path | None = None):
        self._data = {entry: LocalData(entry.default) for entry in list(entries)}
        self._lookup_entry = {(entry.index, entry.subindex): entry for entry in self._data.keys()}
        self._od_path = od_config_path
        self._addr = addr

        self._od_checked = False
        self._connected = False
        self._bus_state = BusState.NOT_FOUND
        self._emcy_cb: Callable | None = None
        self._hb_cb: Callable | None = None

        self._context = zmq.Context()

        self._command_socket = self._context.socket(zmq.REQ)
        self._command_socket.connect(f"tcp://{addr}:6000")
        self._command_socket.setsockopt(zmq.RCVTIMEO, self.RECV_TIMEOUT_MS)
        self._command_socket.setsockopt(zmq.LINGER, 0)
        self._command_socket_lock = Lock()

        self._consume_socket = self._context.socket(zmq.SUB)
        self._consume_socket.connect(f"tcp://{addr}:6001")
        self._consume_socket.setsockopt(zmq.SUBSCRIBE, b"")
        self._consume_thread = Thread(target=self._consume_thread_run, daemon=True)
        self._consume_thread.start()

        self._broadcast_socket = self._context.socket(zmq.PUB)
        self._broadcast_socket.connect(f"tcp://{addr}:6002")

        self._monitor_socket = self._consume_socket.get_monitor_socket()
        self._monitor_thread = Thread(target=self._monitor_thread_run, daemon=True)
        self._monitor_thread.start()

    @property
    def is_connected(self) -> bool:
        return self._connected

    @property
    def bus_state(self) -> BusState:
        return self._bus_state

    def _monitor_thread_run(self):
        while self._monitor_socket.poll():
            event = recv_monitor_message(self._monitor_socket)["event"]
            if event == zmq.EVENT_HANDSHAKE_SUCCEEDED:
                sleep(0.1)
                logger.info("sockets connected")
                self._connected = True
                for entry, data in self._data.items():
                    if not data.write_cb:
                        continue
                    raw = entry.encode(data.value)
                    self._broadcast(OdWriteMessage(entry.index, entry.subindex, raw))
                if self._od_path and not self._od_checked:
                    self._check_od_config(self._od_path)
                    path = self._od_path if isinstance(self._od_path, str) else str(self._od_path)
                    logger.debug("check od %s", path)
                    self._od_checked = True
            elif event in [zmq.EVENT_DISCONNECTED, zmq.EVENT_CLOSED] and self._connected:
                logger.info("sockets disconnected")
                self._connected = False
                self._bus_state = BusState.NOT_FOUND

    def _consume_thread_run(self):
        while True:
            msg_recv = self._consume_socket.recv()
            logger.debug("CONSUME: " + msg_recv.hex().upper())
            if len(msg_recv) < 3:
                continue  # invalid msg

            if msg_recv[1] == OdWriteMessage.id:
                try:
                    msg_req = OdWriteMessage.unpack(msg_recv)
                    entry = self._lookup_entry[(msg_req.index, msg_req.subindex)]
                    value = entry.decode(msg_req.raw)
                    if value == self._data[entry].value:
                        continue
                    self._data[entry].value = value
                    if self._data[entry].write_cb is not None:
                        self._data[entry].write_cb(value)
                except Exception as e:
                    logger.error(f"write callback error: {e}")
            elif msg_recv[1] == HbRecvMessage.id:
                if self._hb_cb:
                    try:
                        msg_req = HbRecvMessage.unpack(msg_recv)
                        self._hb_cb(msg_req.node_id, NodeState(msg_req.state))
                    except Exception as e:
                        logger.error(f"heartbeat callback error: {e}")
            elif msg_recv[1] == EmcyRecvMessage.id:
                if self._emcy_cb:
                    try:
                        msg_req = EmcyRecvMessage.unpack(msg_recv)
                        self._emcy_cb(msg_req.node_id, msg_req.code, msg_req.info)
                    except Exception as e:
                        logger.error(f"emcy callback error: {e}")
            elif msg_recv[1] == BusStateMessage.id:
                try:
                    msg_req = BusStateMessage.unpack(msg_recv)
                    self._bus_state = BusState(msg_req.state)
                except Exception as e:
                    logger.error(f"bus state callback error: {e}")

    def _send_and_recv(self, req_msg):
        self._command_socket_lock.acquire()
        try:
            req_msg_raw = req_msg.pack()

            logger.debug(f"CLIENT SEND {len(req_msg_raw)}: {req_msg_raw.hex().upper()}")
            self._command_socket.send(req_msg_raw)

            res_msg_raw = self._command_socket.recv()
            logger.debug(f"CLIENT RECV {len(res_msg_raw)}: {res_msg_raw.hex().upper()}")

            res_msg = req_msg.unpack(res_msg_raw)
        except Exception as e:
            self._command_socket_lock.release()
            raise e
        self._command_socket_lock.release()
        return res_msg

    def _broadcast(self, msg):
        msg_raw = msg.pack()
        logger.debug("BROADCAST: " + msg_raw.hex().upper())
        try:
            self._broadcast_socket.send(msg_raw)
        except Exception:
            pass

    def send_emcy(self, code: int, info: int = 0):
        self._broadcast(EmcySendMessage(code, info))

    def send_tpdo(self, tpdo: int | Enum | list[int] | list[Enum]):
        def _send_tpdo(num: int | Enum):
            if isinstance(num, Enum):
                num = num.value
            self._broadcast(TpdoSendMessage(num))

        if isinstance(tpdo, (int, Enum)):
            _send_tpdo(tpdo)
        else:
            for t in tpdo:
                _send_tpdo(t)

    def od_write(self, entry: Entry, value: Any):
        if isinstance(value, Enum):
            value = value.value
        if not isinstance(value, entry.data_type.py_types):
            raise ValueError(f"value {value} ({type(value)}) invalid for {entry.data_type}")

        self._data[entry].value = value
        raw = entry.encode(value)
        self._broadcast(OdWriteMessage(entry.index, entry.subindex, raw))

    def od_write_multi(self, data: dict[Entry, Any]):
        for entry, value in data.items():
            self.od_write(entry, value)

    def od_read(self, entry: Entry, use_enum: bool = True) -> Any:
        value = self._data[entry].value
        if use_enum and entry.enum and isinstance(value, int) and value in entry.enum:
            value = entry.enum(value)
        return value

    def add_write_callback(self, entry: Entry, write_cb: Callable[[Any], None]):
        if self._data[entry].write_cb is not None:
            raise ValueError(f"{entry.name} write callback is already set")
        self._data[entry].write_cb = write_cb

    def _check_od_config(self, config_path: str | Path):
        if isinstance(config_path, str):
            config_path = Path(config_path)
        path = str(config_path.absolute())
        self._broadcast(ConfigMessage(path))


class NodeClient(NodeClientBase):
    def __init__(
        self, entries: Entry, addr: str = "localhost", od_config_path: str | Path | None = None
    ):
        super().__init__(entries, addr, od_config_path)

    def add_file(self, file: str | Path):
        if isinstance(file, str):
            file = Path(file)
        file = file.absolute()
        if not file.exists():
            raise FileNotFoundError(f"{file} not found")
        req_msg = AddFileMessage(str(file))
        self._send_and_recv(req_msg)


class ManagerNodeClient(NodeClientBase):
    def __init__(
        self, entries: Entry, addr: str = "localhost", od_config_path: str | Path | None = None
    ):
        super().__init__(entries, addr, od_config_path)

    def sdo_list_files(self, node_id: Enum) -> list[str]:
        req_msg = SdoListFilesMessage(node_id.value)
        res_msg = self._send_and_recv(req_msg)
        return res_msg.files

    def sdo_write_raw(self, node_id: int, index: int, subindex: int, raw: bytes):
        req_msg = SdoWriteMessage(node_id, index, subindex, raw)
        self._send_and_recv(req_msg)

    def sdo_write(self, node_id: Enum, entry: Entry, value: Any):
        if isinstance(value, Enum):
            value = value.value
        raw = entry.encode(value)
        self.sdo_write_raw(node_id.value, entry.index, entry.subindex, raw)

    def sdo_write_file(self, node_id: Enum, local_file: str | Path, remote_file: str = ""):
        if isinstance(local_file, Path):
            local_file = str(local_file)
        req_msg = SdoWriteFileMessage(node_id.value, local_file, remote_file)
        self._send_and_recv(req_msg)

    def sdo_read_raw(self, node_id: int, index: int, subindex: int) -> bytes:
        req_msg = SdoReadMessage(node_id, index, subindex, b"")
        print(req_msg)
        res_msg = self._send_and_recv(req_msg)
        return res_msg.raw

    def sdo_read(self, node_id: Enum, entry: Entry, use_enum: bool = True) -> Any:
        raw = self.sdo_read_raw(node_id.value, entry.index, entry.subindex)
        value = entry.decode(raw)
        if use_enum and entry.enum and isinstance(value, int) and value in entry.enum:
            value = entry.enum(value)
        return value

    def sdo_read_file(
        self, node_id: Enum, remote_file: str, local_file: str | Path = "/tmp"
    ) -> str:
        if isinstance(local_file, Path):
            local_file = str(local_file)
        if os.path.isdir(local_file):
            local_file = os.path.join(local_file, os.path.basename(remote_file))
        req_msg = SdoReadFileMessage(node_id.value, remote_file, local_file)
        self._send_and_recv(req_msg)
        return local_file

    def add_heartbeat_callback(self, hb_cb: Callable[[int, NodeState], None]):
        self._hb_cb = hb_cb

    def add_emcy_callback(self, emcy_cb: Callable[[int, int, int], None]):
        self._emcy_cb = emcy_cb

    def send_sync(self):
        self._broadcast(SyncSendMessage())
