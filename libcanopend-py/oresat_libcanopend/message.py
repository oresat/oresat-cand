import os
import struct
from dataclasses import astuple, dataclass
from typing import ClassVar

PROTOCAL_VERSION = 0  # bump on breaking changes to message formats
PROTOCAL_VERSION_RAW = PROTOCAL_VERSION.to_bytes(1, "little")


@dataclass
class Message:
    _fmt: ClassVar[str]
    id: ClassVar[int]

    @property
    def size(self) -> int:
        return struct.calcsize(self._fmt) + len(PROTOCAL_VERSION_RAW)

    def pack(self) -> bytes:
        return PROTOCAL_VERSION_RAW + struct.pack("<" + self._fmt, self.id, *astuple(self))

    @classmethod
    def unpack(cls, raw: bytes):
        if raw[0] != PROTOCAL_VERSION:
            raise ValueError(f"protocal version byte must be {PROTOCAL_VERSION}")
        if raw[1] != cls.id:
            raise ValueError(f"message id byte must be {cls.id}")
        data = struct.unpack("<" + cls._fmt, raw[1:])
        return cls(*data[1:])


@dataclass
class DynamicMessage:
    _fmt: ClassVar[str]
    id: ClassVar[int]

    @property
    def size(self) -> int:
        return struct.calcsize(self._fmt)

    def pack(self) -> bytes:
        data = astuple(self)
        return PROTOCAL_VERSION_RAW + struct.pack("<" + self._fmt, self.id, *data[:-1]) + data[-1]

    @classmethod
    def unpack(cls, raw: bytes):
        if raw[0] != PROTOCAL_VERSION:
            raise ValueError(f"protocal version byte must be {PROTOCAL_VERSION}")
        if raw[1] != cls.id:
            raise ValueError(f"message id byte must be {cls.id}")
        data = struct.unpack("<" + cls._fmt, raw[1 : cls.size])
        return cls(*data[1:], raw[cls.size :])


@dataclass
class EmcySendMessage(Message):
    _fmt: ClassVar[str] = "BII"
    id: ClassVar[int] = 0x0
    code: int
    info: int


@dataclass
class TpdoSendMessage(Message):
    _fmt: ClassVar[str] = "BB"
    id: ClassVar[int] = 0x1
    num: int


@dataclass
class OdReadMessage(DynamicMessage):
    _fmt: ClassVar[str] = "BHB"
    id: ClassVar[int] = 0x2
    index: int
    subindex: int
    raw: bytes


@dataclass
class OdWriteMessage(DynamicMessage):
    _fmt: ClassVar[str] = "BHB"
    id: ClassVar[int] = 0x3
    index: int
    subindex: int
    raw: bytes


@dataclass
class SdoReadMessage(DynamicMessage):
    _fmt: ClassVar[str] = "BBHB"
    id: ClassVar[int] = 0x4
    node_id: int
    index: int
    subindex: int
    raw: bytes


@dataclass
class SdoWriteMessage(DynamicMessage):
    _fmt: ClassVar[str] = "BBHB"
    id: ClassVar[int] = 0x5
    node_id: int
    index: int
    subindex: int
    raw: bytes


@dataclass
class AddFileMessage(Message):
    _fmt: ClassVar[str] = "B"
    id: ClassVar[int] = 0x6
    file_path: str

    def pack(self) -> bytes:
        if not self.file_path.startswith("/"):
            raise ValueError(f"{self.file_path} must be an absolute path")
        if not os.path.isfile(self.file_path):
            raise FileNotFoundError(f"{self.file_path} does not exist")
        raw = self.file_path.encode()
        return PROTOCAL_VERSION_RAW + struct.pack("<" + self._fmt, self.id) + raw

    @classmethod
    def unpack(cls, raw: bytes):
        if raw[0] != PROTOCAL_VERSION:
            raise ValueError(f"protocal version byte must be {PROTOCAL_VERSION}")
        if raw[1] != cls.id:
            raise ValueError(f"message id byte must be {cls.id}")
        data = struct.pack("<" + cls._fmt, raw[1: cls.size]) + (raw[1 + cls.size :].decode(),)
        return cls(*data[1:])


@dataclass
class HbRecvMessage(Message):
    _fmt: ClassVar[str] = "BBB"
    id: ClassVar[int] = 0x7
    node_id: int
    state: int


@dataclass
class EmcyRecvMessage(Message):
    _fmt: ClassVar[str] = "BBHI"
    id: ClassVar[int] = 0x8
    node_id: int
    code: int
    info: int


@dataclass
class SyncSendMessage(Message):
    _fmt: ClassVar[str] = "BB"
    id: ClassVar[int] = 0x9


@dataclass
class BusStatusMessage(Message):
    _fmt: ClassVar[str] = "BB"
    id: ClassVar[int] = 0xA
    status: int


@dataclass
class ErrorMessage(Message):
    _fmt: ClassVar[str] = "Bi"
    id: ClassVar[int] = 0x80
    error: int


@dataclass
class UnknownIdErrorMessage(Message):
    _fmt: ClassVar[str] = "B"
    id: ClassVar[int] = 0x81


@dataclass
class AbortErrorMessage(Message):
    _fmt: ClassVar[str] = "BI"
    id: ClassVar[int] = 0x82
    code: int
