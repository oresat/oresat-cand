from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import Enum


@dataclass
class DataTypeDef:
    id: int
    py_types: tuple
    fmt: str


class DataType(DataTypeDef, Enum):
    BOOL = 0x1, (bool, int), "?"
    INT8 = 0x2, (int,), "b"
    INT16 = 0x3, (int,), "h"
    INT32 = 0x4, (int,), "i"
    UINT8 = 0x5, (int,), "B"
    UINT16 = 0x6, (int,), "H"
    UINT32 = 0x7, (int,), "I"
    FLOAT32 = 0x8, (float,), "f"
    STR = 0x9, (str,), ""
    BYTES = 0xA, (bytes, bytearray), ""
    DOMAIN = 0xF, (bytes, bytearray, type(None)), ""
    FLOAT64 = 0x11, (float,), "d"
    INT64 = 0x15, (int,), "q"
    UINT64 = 0x1B, (int,), "Q"

    @property
    def size(self) -> int:
        return 0 if self.fmt is None else struct.calcsize(self.fmt)


@dataclass
class EntryBitFieldDef:
    bits: int
    offset: int

    @property
    def mask(self) -> int:
        return ((1 << self.bits) - 1) << self.offset


class EntryBitField(EntryBitFieldDef, Enum):
    """Virtual base class for entry enums."""

    def __hash__(self) -> int:
        return self.mask


@dataclass
class EntryDef:
    index: int
    subindex: int
    data_type: DataType
    default: int | float | bool | str | bytes | None
    low_limit: int | None = None
    high_limit: int | None = None
    enum: Enum | None = None
    bitfield: EntryBitField | None = None


class Entry(EntryDef, Enum):
    """Virtual base class for entries."""

    def __hash__(self) -> int:
        return self.index << 8 + self.subindex

    @classmethod
    def find(cls, index: int, subindex: int) -> Entry:
        for entry in list(cls):
            if entry.index == index and entry.subindex == subindex:
                return entry
        raise ValueError(f"no entry with index 0x{index:X} and subindex 0x{subindex:X} exist")

    def bitfield_to_value(self, values: dict[EntryBitField, int]) -> int:
        if not self.bitfield:
            raise ValueError(f"entry {self.name} does not have a bitfield")
        tmp = 0
        for bitfield, value in values.items():
            if value < 0:
                raise ValueError(
                    f"entry {self.name} value {value} for {bitfield.name} must be positive"
                )
            if value > (bitfield.bits << 1):
                raise ValueError(f"entry {self.name} value {value} exceeded {bitfield.name} size")
            tmp += value << bitfield.offset
        return tmp

    def value_to_bitfield(self, value: int) -> dict[EntryBitField, int]:
        if not self.bitfield:
            raise ValueError(f"entry {self.name} does not have a bitfield")
        if value < 0:
            raise ValueError(f"entry {self.name} value {value} must be positive")
        values = {}
        for bitfield in list(self.bitfield):
            values[bitfield] = (value & bitfield.mask) >> bitfield.offset
        return values

    def decode(self, raw: bytes) -> bool | int | float | str | bytes | None:
        value = raw
        try:
            if self.data_type == DataType.STR:
                value = raw.decode()
            elif self.data_type == DataType.BYTES:
                value = raw
            elif self.data_type != DataType.DOMAIN:
                value = struct.unpack("<" + self.data_type.fmt, raw)[0]
        except Exception as e:
            raise ValueError(f"{self.name} decode {e}") from e
        return value

    def decode_to_enum(self, raw: bytes) -> Enum:
        value = self.decode(raw)
        if self.enum is not None:
            return self.enum(value)
        raise ValueError(f"entry {self.name} has no enum def")

    def encode(self, value: bool | int | float | str | bytes | None | Enum) -> bytes:
        if isinstance(value, Enum):
            value = value.value

        if not isinstance(value, self.data_type.py_types):
            raise TypeError(f"{self.name} encode {value} ({type(value)}) invalid")

        raw = value
        try:
            raw = b""
            if self.data_type == DataType.STR:
                raw = value.encode()
            elif self.data_type == DataType.BYTES:
                raw = value
            elif self.data_type == DataType.DOMAIN:
                if value is not None:
                    raw = value
            else:
                raw = struct.pack("<" + self.data_type.fmt, value)
        except Exception as e:
            raise ValueError(f"{self.name} encode {e}") from e
        return raw
