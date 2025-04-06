import json
import os
import struct
from dataclasses import astuple, dataclass, field
from typing import ClassVar

PROTOCAL_VERSION = 0  # bump on breaking changes to message formats
PROTOCAL_VERSION_RAW = PROTOCAL_VERSION.to_bytes(1, "little")


def _validate_unpack(msg_id: int, raw: bytes):
    if raw[0] != PROTOCAL_VERSION:
        raise ValueError(f"protocal version byte must be {PROTOCAL_VERSION}")
    if raw[1] != msg_id:
        raise ValueError(f"message id byte must be {msg_id}")


def _validate_file_exist(file_path: str):
    if not file_path.startswith("/"):
        raise ValueError(f"{file_path} must be an absolute path")
    if not os.path.isfile(file_path):
        raise FileNotFoundError(f"{file_path} does not exist")


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
        _validate_unpack(cls.id, raw)
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
        _validate_unpack(cls.id, raw)
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
class OdWriteMessage(DynamicMessage):
    _fmt: ClassVar[str] = "BHB"
    id: ClassVar[int] = 0x2
    index: int
    subindex: int
    raw: bytes


@dataclass
class SdoReadMessage(DynamicMessage):
    _fmt: ClassVar[str] = "BBHB"
    id: ClassVar[int] = 0x3
    node_id: int
    index: int
    subindex: int
    raw: bytes


@dataclass
class SdoWriteMessage(DynamicMessage):
    _fmt: ClassVar[str] = "BBHB"
    id: ClassVar[int] = 0x4
    node_id: int
    index: int
    subindex: int
    raw: bytes


@dataclass
class AddFileMessage(Message):
    _fmt: ClassVar[str] = "B"
    id: ClassVar[int] = 0x5
    file_path: str

    def pack(self) -> bytes:
        _validate_file_exist(self.file_path)
        raw = self.file_path.encode()
        return PROTOCAL_VERSION_RAW + struct.pack("<" + self._fmt, self.id) + raw

    @classmethod
    def unpack(cls, raw: bytes):
        _validate_unpack(cls.id, raw)
        path_raw = raw[1 + cls.size :]
        if path_raw[-1] == b"\x00":
            path_raw = path_raw[:-1]
        path = path_raw.decode()
        data = struct.pack("<" + cls._fmt, raw[1 : cls.size])
        return cls(*data[1:], path)


@dataclass
class HbRecvMessage(Message):
    _fmt: ClassVar[str] = "BBB"
    id: ClassVar[int] = 0x6
    node_id: int
    state: int


@dataclass
class EmcyRecvMessage(Message):
    _fmt: ClassVar[str] = "BBHI"
    id: ClassVar[int] = 0x7
    node_id: int
    code: int
    info: int


@dataclass
class SyncSendMessage(Message):
    _fmt: ClassVar[str] = "BB"
    id: ClassVar[int] = 0x8


@dataclass
class BusStateMessage(Message):
    _fmt: ClassVar[str] = "BB"
    id: ClassVar[int] = 0x9
    state: int


@dataclass
class SdoReadFileMessage(Message):
    _fmt: ClassVar[str] = "BB"
    id: ClassVar[int] = 0xA
    node_id: int
    remote_file_path: str
    local_file_path: str

    def pack(self) -> bytes:
        raw = self.remote_file_path.encode() + b"\x00" + self.local_file_path.encode() + b"\x00"
        return PROTOCAL_VERSION_RAW + struct.pack("<" + self._fmt, self.id) + raw

    @classmethod
    def unpack(cls, raw: bytes):
        _validate_unpack(cls.id, raw)
        paths_raw = raw[1 + cls.size :].split(b"\x00")
        paths = (paths_raw[0].decode(), paths_raw[1].decode())
        data = struct.pack("<" + cls._fmt, raw[1 : cls.size]) + paths
        return cls(*data[1:])


@dataclass
class SdoWriteFileMessage(Message):
    _fmt: ClassVar[str] = "BB"
    id: ClassVar[int] = 0xB
    node_id: int
    remote_file_path: str
    local_file_path: str

    def pack(self) -> bytes:
        _validate_file_exist(self.local_file_path)
        raw = self.remote_file_path.encode() + b"\x00" + self.local_file_path.encode() + b"\x00"
        return PROTOCAL_VERSION_RAW + struct.pack("<" + self._fmt, self.id) + raw

    @classmethod
    def unpack(cls, raw: bytes):
        _validate_unpack(cls.id, raw)
        paths_raw = raw[1 + cls.size :].split(b"\x00")
        paths = (paths_raw[0].decode(), paths_raw[1].decode())
        data = struct.pack("<" + cls._fmt, raw[1 : cls.size]) + paths
        return cls(*data[1:])


@dataclass
class SdoListFilesMessage(Message):
    _fmt: ClassVar[str] = "BB"
    id: ClassVar[int] = 0xC
    node_id: int
    files: list[str] = field(default_factory=list)

    def pack(self) -> bytes:
        raw = json.dumps(self.files).encode()
        return PROTOCAL_VERSION_RAW + struct.pack("<" + self._fmt, self.id) + raw

    @classmethod
    def unpack(cls, raw: bytes):
        _validate_unpack(cls.id, raw)
        files_raw = raw[1 + cls.size :]
        if files_raw[-1] == b"\x00":
            files_raw = files_raw[:-1]
        files = json.dumps(files_raw.decode())
        data = struct.pack("<" + cls._fmt, raw[1 : cls.size])
        return cls(*data[1:], files)


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
