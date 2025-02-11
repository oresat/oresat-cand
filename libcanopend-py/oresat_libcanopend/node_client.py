import logging
from enum import Enum
from threading import Thread
from dataclasses import dataclass
from typing import Optional, Union, Callable, Any

import zmq

from . import message
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
        self._lookup_entry = {(entry.index, entry.subindex): entry for entry in self._data.keys()}
        self._debug = debug

        self._context = zmq.Context()

        self._command_socket = self._context.socket(zmq.REQ)
        self._command_socket.connect(f"tcp://{addr}:6000")
        self._command_socket.setsockopt(zmq.RCVTIMEO, self.RECV_TIMEOUT_MS)
        self._command_socket.setsockopt(zmq.LINGER, 0)

        self._broadcast_socket = self._context.socket(zmq.PUB)
        self._broadcast_socket.connect(f"tcp://{addr}:6001")

        self._consume_socket = self._context.socket(zmq.SUB)
        self._consume_socket.connect(f"tcp://{addr}:6002")
        self._consume_thread = Thread(target=self._consume_thread_run, daemon=True)
        self._consume_thread.start()

        self._reply_thread = Thread(target=self._reply_thread_run, daemon=True)
        self._reply_thread.start()

    def _consume_thread_run(self):
        while True:
            msg_recv = self._consume_socket.recv()
            if self._debug:
                logging.debug("RECV: " + msg_recv.hex().upper())
            if (
                len(msg_recv) < message.MessageOdWrite.size
                or msg_recv[0] != message.MessageOdWrite.id
            ):
                logging.debug(f"invalid od write message {msg_recv.hex().upper()}")
                continue

            try:
                msg_od_write = message.MessageOdWrite.unpack(msg_recv)
                entry = self._lookup_entry[(msg_od_write.index, msg_od_write.subindex)]
                value = entry.raw_to_value(msg_od_write.raw)
                if self._data[entry].write_cb is None:
                    self._data[entry].value = value
                else:
                    self._data[entry].write_cb(value)
            except Exception as e:
                logging.error(f"consume error {entry.name} {e}")

    def _reply_thread_run(self):
        request = message.MessageReqPort(0).pack()

        while True:
            try:
                response = self._command_socket.send(request)
                if response is not None:
                    msg_req_port = message.MessageReqPort(response)
                    break
            except Exception:
                continue

        reply_socket = self._context.socket(zmq.REP)
        reply_socket.connect(f"tcp://*:{msg_req_port.port}")

        while True:
            request = reply_socket.recv()
            try:
                if request[0] == message.MessageOdWrite.id:
                    msg_od_write = message.MessageOdWrite.unpack(request)
                    entry = self._lookup_entry[(msg_od_write.index, msg_od_write.subindex)]
                    value = entry.raw_to_value(msg_od_write.raw)
                    self.od_write(entry, value)
                    reply_socket.send(msg_od_write.pack())
                elif request[0] == message.MessageOdRead.id:
                    msg_od_read = message.MessageOdRead.unpack(request)
                    entry = self._lookup_entry[(msg_od_write.index, msg_od_write.subindex)]
                    value = self.od_read(entry)
                    msg_od_read.raw = entry.value_to_raw(value)
                    reply_socket.send(msg_od_read.pack())
                else:
                    reply_socket.send(message.MessageErrorUnknownId().pack())
            except Exception as e:
                logging.error(f"reply error {entry.name} {e}")

    def _send_and_recv(self, req_msg, response_match_check: bool = False):
        req_msg_raw = req_msg.pack()
        if self._debug:
            logging.debug("SEND: " + req_msg_raw.hex().upper())

        self._command_socket.send(req_msg_raw)

        res_msg_raw = self._command_socket.recv()
        if self._debug:
            logging.debug("RECV: " + res_msg_raw.hex().upper())

        res_msg = req_msg.unpack(res_msg_raw)
        if response_match_check and res_msg_raw != req_msg_raw:
            raise ValueError("sent message did not match recv message")
        return res_msg

    def _broadcast(self, msg: bytes):
        try:
            self._broadcast_socket.send(msg)
        except Exception:
            pass

    def send_emcy(self, code: int, info: int = 0):
        self._broadcast(message.MessageEmcySend(code, info))

    def _send_tpdo(self, num: int):
        self._broadcast(message.MessageTpdoSend(num).pack())

    def send_tpdo(self, num: Union[int, list[int]]):
        if isinstance(num, int):
            self._send_tpdo(num)
        else:
            for n in num:
                self._send_tpdo(n)

    def od_write(self, entry: Entry, value: Any):
        if not isinstance(value, entry.data_type.py_types):
            raise ValueError(f"value {value} ({type(value)}) invalid for {entry.data_type}")
        if isinstance(value, Enum) and entry.enum:
            value = value.value

        self._data[entry].value = value
        raw = entry.value_to_raw(value)
        req_msg = message.MessageOdWrite(entry.index, entry.subindex, entry.data_type.id, raw)
        self._broadcast(req_msg)

    def od_write_multi(self, data: dict[Entry, Any]):
        for entry, value in data.items():
            self.od_write(entry, value)

    def od_read(self, entry: Entry, use_enum: bool = True) -> Any:
        value = self._data[entry].value
        if self._data[entry].read_cb is not None:
            value = self._data[entry].read_cb()

        if use_enum and entry.enum and isinstance(value, int):
            value = entry.enum[value]
        return value

    def sdo_write(self, node_id: int, entry: Entry, value: Any):
        raw = entry.value_to_raw(value)
        req_msg = message.MessageSdoWrite(
            node_id, entry.index, entry.subindex, entry.data_type.id, raw
        )
        res_msg = self._send_and_recv(req_msg)
        value = entry.raw_to_value(res_msg.raw)
        self._data[entry].value = value

    def sdo_read(self, node_id: int, entry: Entry, use_enum: bool = True) -> Any:
        req_msg = message.MessageSdoRead(
            node_id, entry.index, entry.subindex, entry.data_type.id, b""
        )
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
            req_msg = message.MessageOwnEntry(entry.index, entry.subindex)
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
