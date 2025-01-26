import struct
from enum import Enum
from typing import Any

import zmq


class MsgId(Enum):
    EMCY_SEND = 0x00
    TPDO_SEND = 0x01
    OD_READ = 0x02
    OD_WRITE = 0x03
    SDO_READ = 0x04
    SDO_WRITE = 0x05
    ERROR_UNKNOWN_ID = 0x80
    ERROR_LENGTH = 0x81
    ERROR_TPDO_NUM = 0x82
    ERROR_OD_ABORT = 0x83
    ERROR_SDO_ABORT = 0x84


class Datatype(Enum):
    BOOLEAN = 0x1
    INTERGER8 = 0x2
    INTERGER16 = 0x3
    INTERGER32 = 0x4
    UNSIGNED8 = 0x5
    UNSIGNED16 = 0x6
    UNSIGNED32 = 0x7
    REAL32 = 0x8
    VISIBLE_STRING = 0x9
    OCTET_STRING = 0xA
    DOMAIN = 0xF
    REAL64 = 0x11
    INTERGER64 = 0x15
    UNSIGNED64 = 0x1B


DATATYPE_FMT_CHARS = {
    Datatype.BOOLEAN: "?",
    Datatype.INTERGER8: "b",
    Datatype.INTERGER16: "h",
    Datatype.INTERGER32: "i",
    Datatype.UNSIGNED8: "B",
    Datatype.UNSIGNED16: "H",
    Datatype.UNSIGNED32: "I",
    Datatype.REAL32: "f",
    Datatype.REAL64: "d",
    Datatype.INTERGER64: "q",
    Datatype.UNSIGNED64: "Q",
}
"""fmt chars for struct.pack() / struct.unpack()"""


ABORT_CODES = {
    0x05030000: "Toggle bit not alternated",
    0x05040000: "SDO protocol timed out",
    0x05040001: "Client/server command specifier not valid or unknown",
    0x05040002: "Invalid block size (block mode only)",
    0x05040003: "Invalid sequence number (block mode only)",
    0x05040004: "CRC error (block mode only)",
    0x05040005: "Out of memory",
    0x06010000: "Unsupported access to an object",
    0x06010001: "Attempt to read a write only object",
    0x06010002: "Attempt to write a read only object",
    0x06020000: "Object does not exist in the object dictionary",
    0x06040041: "Object cannot be mapped to the PDO",
    0x06040042: "The number and length of the objects to be mapped would exceed PDO length",
    0x06040043: "General parameter incompatibility reason",
    0x06040047: "General internal incompatibility in the device",
    0x06060000: "Access failed due to an hardware error",
    0x06070010: "Data type does not match, length of service parameter does not match",
    0x06070012: "Data type does not match, length of service parameter too high",
    0x06070013: "Data type does not match, length of service parameter too low",
    0x06090011: "Sub-index does not exist",
    0x06090030: "Invalid value for parameter (download only)",
    0x06090031: "Value of parameter written too high (download only)",
    0x06090032: "Value of parameter written too low (download only)",
    0x06090036: "Maximum value is less than minimum value",
    0x060A0023: "Resource not available: SDO connection",
    0x08000000: "General error",
    0x08000020: "Data cannot be transferred or stored to the application",
    0x08000021: "Data cannot be transferred or stored to the application because of local control",
    0x08000022: "Data cannot be transferred or stored to the application because of the present device state",
    0x08000023: "Object dictionary dynamic generation fails or no object dictionary is present",
    0x08000024: "No data available",
    0xFFFFFFFF: "No SDO client",
}


class NodeClient:
    def __init__(self, addr: str = "localhost", port: int = 5555, debug: bool = False):
        self._context = zmq.Context()
        self._socket = self._context.socket(zmq.REQ)
        self._socket.connect(f"tcp://{addr}:{port}")
        self._socket.setsockopt(zmq.RCVTIMEO, 1500)
        self._socket.setsockopt(zmq.LINGER, 0)
        self._debug = debug

    def _send_and_recv(self, msg: bytes) -> bytes:
        if self._debug:
            print("SEND: " + msg.hex().upper())
        self._socket.send(msg)

        try:
            msg_recv = self._socket.recv()
            if self._debug:
                print("RECV: " + msg_recv.hex().upper())
        except zmq.error.Again:
            raise TimeoutError("no response")

        if msg_recv[0] == MsgId.ERROR_LENGTH.value:
            raise ValueError("sent message length was wrong")
        if msg_recv[0] == MsgId.ERROR_TPDO_NUM.value:
            raise ValueError(f"tpdo num {msg_recv[1]} was not value")
        if msg_recv[0] == MsgId.ERROR_UNKNOWN_ID.value:
            raise ValueError(f"unknown id 0x{msg_recv[0]:X}")
        return msg_recv

    def emcy_send(self, code: int):
        msg = struct.pack("<BI", MsgId.EMCY_SEND.value, code)
        msg_recv = self._send_and_recv(msg)
        if msg_recv != msg:
            raise ValueError("sent message did not match recv message")

    def tpdo_send(self, num: int):
        msg = struct.pack("<2B", MsgId.TPDO_SEND.value, num)
        msg_recv = self._send_and_recv(msg)
        if msg_recv != msg:
            raise ValueError("sent message did not match recv message")

    def od_write(self, index: int, subindex: int, dtype: Datatype, value: Any):
        msg = struct.pack("<BH2B", MsgId.OD_WRITE.value, index, subindex, dtype.value)
        if dtype in [Datatype.OCTET_STRING, Datatype.DOMAIN]:
            msg += value
        elif dtype == Datatype.VISIBLE_STRING:
            msg += value.encode()
        else:
            msg += struct.pack("<" + DATATYPE_FMT_CHARS[dtype], value)

        msg_recv = self._send_and_recv(msg + value)
        if msg_recv[0] == MsgId.ERROR_OD_ABORT.value:
            ac = int.from_bytes(msg_recv[-4:], "little")
            raise ValueError(f"od abort 0x{ac:X} - {ABORT_CODES[ac]}")
        if msg_recv != msg:
            raise ValueError("sent message did not match recv message")

    def od_read(self, index: int, subindex: int, dtype: Datatype) -> Any:
        msg = struct.pack("<BH2B", MsgId.OD_READ.value, index, subindex, dtype.value)
        msg_recv = self._send_and_recv(msg)
        if msg_recv[0] == MsgId.ERROR_OD_ABORT.value:
            ac = int.from_bytes(msg_recv[-4:], "little")
            raise ValueError(f"od abort 0x{ac:X} - {ABORT_CODES[ac]}")
        if not msg_recv.startswith(msg):
            raise ValueError("recv message did start with sent message")

        value: Any = msg_recv[len(msg) :]
        if dtype == Datatype.VISIBLE_STRING:
            value = value.decode()
        elif dtype not in [Datatype.OCTET_STRING, Datatype.DOMAIN]:
            value = struct.unpack("<" + DATATYPE_FMT_CHARS[dtype], value)[0]
        return value

    def sdo_write(self, index: int, subindex: int, dtype: Datatype, value: Any):
        msg = struct.pack("<BH2B", MsgId.SDO_READ.value, index, subindex, dtype.value)
        if dtype in [Datatype.OCTET_STRING, Datatype.DOMAIN]:
            msg += value
        elif dtype == Datatype.VISIBLE_STRING:
            msg += value.encode()
        else:
            msg += struct.pack("<" + DATATYPE_FMT_CHARS[dtype], value)

        msg_recv = self._send_and_recv(msg)
        if msg_recv[0] == MsgId.ERROR_SDO_ABORT.value:
            ac = int.from_bytes(msg_recv[-4:], "little")
            raise ValueError(f"sdo abort 0x{ac:X} - {ABORT_CODES[ac]}")
        if msg_recv != msg:
            raise ValueError("sent message did not match recv message")

    def sdo_read(self, index: int, subindex: int, dtype: Datatype) -> Any:
        msg = struct.pack("<BH2B", MsgId.SDO_READ.value, index, subindex, dtype.value)
        msg_recv = self._send_and_recv(msg)
        if msg_recv[0] == MsgId.ERROR_SDO_ABORT.value:
            ac = int.from_bytes(msg_recv[-4:], "little")
            raise ValueError(f"sdo abort 0x{ac:X} - {ABORT_CODES[ac]}")
        if not msg_recv.startswith(msg):
            raise ValueError("recv message did start with sent message")

        value: Any = msg_recv[len(msg) :]
        if dtype == Datatype.VISIBLE_STRING:
            value = value.decode()
        elif dtype not in [Datatype.OCTET_STRING, Datatype.DOMAIN]:
            value = struct.unpack("<" + DATATYPE_FMT_CHARS[dtype], value)[0]
        return value
