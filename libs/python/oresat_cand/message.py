from __future__ import annotations

import struct
from dataclasses import astuple, dataclass
from typing import ClassVar

PROTOCAL_VERSION = 0  # Bump on breaking changes to message formats
PROTOCAL_VERSION_RAW = PROTOCAL_VERSION.to_bytes(1, "little")

# custom struck-like formats
DYN_STR_FMT = "w"
DYN_BYTES_FMT = "y"
DYN_FMT_SIZE = 1


@dataclass
class Message:
    _fmt: ClassVar[list[str]]
    id: ClassVar[int]

    def pack(self) -> bytes:
        raw = PROTOCAL_VERSION_RAW + bytes([self.id])
        values = astuple(self)
        offset = 0
        for fmt in self._fmt:
            if fmt == DYN_BYTES_FMT:
                tmp = values[offset]
                raw += len(tmp).to_bytes(DYN_FMT_SIZE, "little") + tmp
                offset += 1
            elif fmt == DYN_STR_FMT:
                tmp = values[offset].encode("utf-8")
                raw += len(tmp).to_bytes(DYN_FMT_SIZE, "little") + tmp
                offset += 1
            else:
                count = len(fmt)
                raw += struct.pack("<" + fmt, *values[offset : offset + count])
                offset += count
        return raw

    @classmethod
    def unpack(cls, raw: bytes) -> Message:
        if raw[0] != PROTOCAL_VERSION:
            raise ValueError(f"protocal version byte must be {PROTOCAL_VERSION}")
        if raw[1] != cls.id:
            raise ValueError(f"message id byte must be {cls.id}")
        values = tuple(raw[:2])
        offset = 2
        for fmt in cls._fmt:
            if fmt == DYN_BYTES_FMT:
                size = int.from_bytes(raw[offset : offset + DYN_FMT_SIZE], "little")
                offset += DYN_FMT_SIZE
                values += (raw[offset : offset + size],)
                offset += size
            elif fmt == DYN_STR_FMT:
                size = int.from_bytes(raw[offset : offset + DYN_FMT_SIZE], "little")
                offset += DYN_FMT_SIZE
                values += (raw[offset : offset + size].decode("utf-8").strip(),)
                offset += size
            else:
                size = struct.calcsize("<" + fmt)
                values += struct.unpack("<" + fmt, raw[offset : offset + size])
                offset += size
        return cls(*values[2:])


@dataclass
class EmcySendMessage(Message):
    _fmt: ClassVar[list[str]] = ["II"]
    id: ClassVar[int] = 0x0
    code: int
    info: int


@dataclass
class TpdoSendMessage(Message):
    _fmt: ClassVar[list[str]] = ["B"]
    id: ClassVar[int] = 0x1
    num: int


@dataclass
class OdWriteMessage(Message):
    _fmt: ClassVar[list[str]] = ["HB", DYN_BYTES_FMT]
    id: ClassVar[int] = 0x2
    index: int
    subindex: int
    raw: bytes


@dataclass
class SdoReadMessage(Message):
    _fmt: ClassVar[list[str]] = ["BHB", DYN_BYTES_FMT]
    id: ClassVar[int] = 0x3
    node_id: int
    index: int
    subindex: int
    raw: bytes


@dataclass
class SdoWriteMessage(Message):
    _fmt: ClassVar[list[str]] = ["BHB", DYN_BYTES_FMT]
    id: ClassVar[int] = 0x4
    node_id: int
    index: int
    subindex: int
    raw: bytes


@dataclass
class AddFileMessage(Message):
    _fmt: ClassVar[list[str]] = [DYN_STR_FMT]
    id: ClassVar[int] = 0x5
    file_path: str


@dataclass
class HbRecvMessage(Message):
    _fmt: ClassVar[list[str]] = ["BB"]
    id: ClassVar[int] = 0x6
    node_id: int
    state: int


@dataclass
class EmcyRecvMessage(Message):
    _fmt: ClassVar[list[str]] = ["BHI"]
    id: ClassVar[int] = 0x7
    node_id: int
    code: int
    info: int


@dataclass
class SyncSendMessage(Message):
    _fmt: ClassVar[list[str]] = [""]
    id: ClassVar[int] = 0x8


@dataclass
class BusStateMessage(Message):
    _fmt: ClassVar[list[str]] = ["B"]
    id: ClassVar[int] = 0x9
    state: int


@dataclass
class SdoReadToFileMessage(Message):
    _fmt: ClassVar[list[str]] = ["B", DYN_STR_FMT]
    id: ClassVar[int] = 0xA
    node_id: int
    file_path: str


@dataclass
class SdoWriteFromFileMessage(Message):
    _fmt: ClassVar[list[str]] = ["B", DYN_STR_FMT]
    id: ClassVar[int] = 0xB
    node_id: int
    file_path: str


@dataclass
class ConfigMessage(Message):
    _fmt: ClassVar[list[str]] = [DYN_STR_FMT]
    id: ClassVar[int] = 0xC
    file: str


@dataclass
class ErrorMessage(Message):
    _fmt: ClassVar[list[str]] = ["i"]
    id: ClassVar[int] = 0x80
    error: int


@dataclass
class UnknownIdErrorMessage(Message):
    _fmt: ClassVar[list[str]] = ["B"]
    id: ClassVar[int] = 0x81
    value: int


@dataclass
class AbortErrorMessage(Message):
    _fmt: ClassVar[list[str]] = ["I"]
    id: ClassVar[int] = 0x82
    code: int
