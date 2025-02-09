from enum import Enum
from threading import Thread
from dataclasses import dataclass
from typing import Optional, Union, Callable, Any

import zmq

from . import msg
from .entry import Entry


@dataclass
class LocalData:
    value: Union[int, float, str, bytes, None]
    owner: bool = False
    ownership_ack: bool = False  # reset on connection lost
    read_cb: Optional[Callable[[], [Any]]] = None
    write_cb: Optional[Callable[[Any], None]] = None


class NodeClient:
    RECV_TIMEOUT_MS = 100

    def __init__(self, entries: Entry, addr: str = "localhost", debug: bool = False):
        self._data = {entry: LocalData(entry.default) for entry in list(entries)}
        self._debug = debug

        self._context = zmq.Context()

        self._req_socket = self._context.socket(zmq.REQ)
        self._req_socket.connect(f"tcp://{addr}:5555")
        self._req_socket.setsockopt(zmq.RCVTIMEO, self.RECV_TIMEOUT_MS)
        self._req_socket.setsockopt(zmq.LINGER, 0)

        self._thread = Thread(target=self._res_thread, daemon=True)
        self._thread.start()

    def _res_thread(self):
        entries = {(entry.index, entry.subindex): entry for entry in self._data.keys()}

        request = msg.MsgReqPort(0).pack()
        while True:
            try:
                response = self._req_socket.send(request)
                if response is not None:
                    msg_req_port = msg.MsgReqPort(response)
                    break
            except Exception:
                continue

        res_socket = self._context.socket(zmq.REP)
        res_socket.connect(f"tcp://*:{msg_req_port.port}")
        res_socket.setsockopt(zmq.RCVTIMEO, self.RECV_TIMEOUT_MS)

        while True:
            request = res_socket.recv()
            try:
                if request[0] == msg.MsgOdWrite.id:
                    msg_od_write = msg.MsgOdWrite.unpack(request)
                    entry = entries[(msg_od_write.index, msg_od_write.subindex)]
                    value = entry.raw_to_value(msg_od_write.raw)
                    self.od_write(entry, value)
                    res_socket.send(msg_od_write.pack())
                elif request[0] == msg.MsgOdRead.id:
                    msg_od_read = msg.MsgOdRead.unpack(request)
                    entry = entries[(msg_od_write.index, msg_od_write.subindex)]
                    value = self.od_read(entry)
                    msg_od_read.raw = entry.value_to_raw(value)
                    res_socket.send(msg_od_read.pack())
                else:
                    res_socket.send(msg.MsgErrorUnknownId().pack())
            except Exception:
                continue

    def _send_and_recv(self, req_msg, response_match_check: bool = False):
        req_msg_raw = req_msg.pack()
        if self._debug:
            print("SEND: " + req_msg_raw.hex().upper())

        self._req_socket.send(req_msg_raw)

        res_msg_raw = self._req_socket.recv()
        if self._debug:
            print("RECV: " + res_msg_raw.hex().upper())

        res_msg = req_msg.unpack(res_msg_raw)
        if response_match_check and res_msg_raw != req_msg_raw:
            raise ValueError("sent message did not match recv message")
        return res_msg

    def send_emcy(self, code: int, info: int = 0, raise_send_error: bool = False):
        try:
            req_msg = msg.MsgEmcySend(code, info)
            self._send_and_recv(req_msg)
        except Exception as e:
            if raise_send_error:
                raise e

    def send_tpdo(self, num: int, raise_send_error: bool = False):
        try:
            req_msg = msg.MsgTpdoSend(num).pack()
            self._send_and_recv(req_msg)
        except Exception as e:
            if raise_send_error:
                raise e

    def od_write(self, entry: Entry, value: Any):
        if isinstance(value, Enum) and entry.enum:
            value = value.value

        if not self._data[entry].owner:
            raw = entry.value_to_raw(value)
            req_msg = msg.MsgOdWrite(entry.index, entry.subindex, entry.data_type.id, raw)
            res_msg = self._send_and_recv(req_msg)
            value = entry.raw_to_value(res_msg.raw)
            self._data[entry].value = value
        elif self._data[entry].write_cb is None:
            if not isinstance(value, entry.data_type.py_types):
                raise ValueError(f"value {value} ({type(value)}) invalid for {entry.data_type}")
            self._data[entry].value = value
        else:
            self._data[entry].write_cb(value)

    def od_write_multi(self, data: dict[Entry, Any]):
        for entry, value in data.items():
            self.od_write(entry, value)

    def od_read(self, entry: Entry, use_enum: bool = True) -> Any:
        value = self._data[entry].value
        if not self._data[entry].owner:
            req_msg = msg.MsgOdRead(entry.index, entry.subindex, entry.data_type.id, b"")
            res_msg = self._send_and_recv(req_msg)
            value = entry.raw_to_value(res_msg.raw)
        elif self._data[entry].read_cb is not None:
            value = self._data[entry].read_cb()

        if use_enum and entry.enum and isinstance(value, int):
            value = entry.enum[value]
        return value

    def sdo_write(self, node_id: int, entry: Entry, value: Any):
        raw = entry.value_to_raw(value)
        req_msg = msg.MsgSdoWrite(node_id, entry.index, entry.subindex, entry.data_type.id, raw)
        res_msg = self._send_and_recv(req_msg)
        value = entry.raw_to_value(res_msg.raw)
        self._data[entry].value = value

    def sdo_read(self, node_id: int, entry: Entry, use_enum: bool = True) -> Any:
        req_msg = msg.MsgSdoRead(node_id, entry.index, entry.subindex, entry.data_type.id, b"")
        res_msg = self._send_and_recv(req_msg)
        value = entry.raw_to_value(res_msg.raw)
        if use_enum and entry.enum and isinstance(value, int):
            value = entry.enum[value]
        return value

    def request_ownership(self, entry: Entry, read_cb=None, write_cb=None):
        self._data[entry].owner = True
        self._data[entry].read_cb = read_cb
        self._data[entry].write_cb = write_cb
        try:
            req_msg = msg.MsgOwnEntry(entry.index, entry.subindex)
            res_msg = self._send_and_recv(req_msg)
            self._data[entry].ownership_ack = req_msg == res_msg
        except Exception:
            pass

    def request_ownership_multi(self, data: list):
        for d in data:
            if isinstance(d, Entry):
                self.request_ownership(d)
            else:
                self.request_ownership(*d)
