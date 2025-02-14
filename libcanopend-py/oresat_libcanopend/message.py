import struct
from dataclasses import astuple, dataclass
from typing import ClassVar


@dataclass
class Message:
    _fmt: ClassVar[str]
    id: ClassVar[int]

    @property
    def size(self) -> int:
        return struct.calcsize(self._fmt)

    def pack(self) -> bytes:
        return struct.pack(self._fmt, self.id, *astuple(self))

    @classmethod
    def unpack(cls, raw: bytes):
        if raw[0] != cls.id:
            raise ValueError(f"first byte must be {cls.id}")
        data = struct.unpack("<" + cls._fmt, raw)
        return cls(*data[1:])


@dataclass
class DynamicMessage:
    _fmt: ClassVar[str]
    id: ClassVar[int]

    def pack(self) -> bytes:
        data = astuple(self)
        return struct.pack("<" + self._fmt, self.id, *data[:-1]) + data[-1]

    @classmethod
    def unpack(cls, raw: bytes):
        if raw[0] != cls.id:
            raise ValueError(f"first byte must be {cls.id}")
        size = struct.calcsize(cls._fmt)
        data = struct.unpack("<" + cls._fmt, raw[:size])
        return cls(*data[1:], raw[size:])


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
    _fmt: ClassVar[str] = "BHBB"
    id: ClassVar[int] = 0x2
    index: int
    subindex: int
    data_type: int
    raw: bytes


@dataclass
class OdWriteMessage(DynamicMessage):
    _fmt: ClassVar[str] = "BHBB"
    id: ClassVar[int] = 0x3
    index: int
    subindex: int
    data_type: int
    raw: bytes


@dataclass
class SdoReadMessage(DynamicMessage):
    _fmt: ClassVar[str] = "BBHBB"
    id: ClassVar[int] = 0x4
    node_id: int
    index: int
    subindex: int
    data_type: int
    raw: bytes


@dataclass
class SdoWriteMessage(DynamicMessage):
    _fmt: ClassVar[str] = "BBHBB"
    id: ClassVar[int] = 0x5
    node_id: int
    index: int
    subindex: int
    data_type: int
    raw: bytes


@dataclass
class RequestPortMessage(Message):
    _fmt: ClassVar[str] = "BI"
    id: ClassVar[int] = 0x6
    port: int


@dataclass
class RequestOwnershipMessage(Message):
    _fmt: ClassVar[str] = "BI"
    id: ClassVar[int] = 0x7
    index: int
    subindex: int


@dataclass
class ErrorMessage(Message):
    _fmt: ClassVar[str] = "B"
    id: ClassVar[int] = 0x80


@dataclass
class UnknownIdErrorMessage(Message):
    _fmt: ClassVar[str] = "B"
    id: ClassVar[int] = 0x81


@dataclass
class AbortErrorMessage(Message):
    _fmt: ClassVar[str] = "BI"
    id: ClassVar[int] = 0x82
    code: int
