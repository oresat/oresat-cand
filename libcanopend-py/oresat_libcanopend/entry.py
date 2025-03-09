import struct
from dataclasses import dataclass
from enum import Enum
from typing import Any, Optional, Union


@dataclass
class DataTypeDef:
    id: int
    py_types: tuple
    fmt: Optional[str]


class DataType(DataTypeDef, Enum):
    BOOL = 0x1, (bool,), "?"
    INT8 = 0x2, (int,), "b"
    INT16 = 0x3, (int,), "h"
    INT32 = 0x4, (int,), "i"
    UINT8 = 0x5, (int,), "B"
    UINT16 = 0x6, (int,), "H"
    UINT32 = 0x7, (int,), "I"
    FLOAT32 = 0x8, (float,), "f"
    STR = 0x9, (str,), None
    BYTES = 0xA, (bytes, bytearray), None
    DOMAIN = 0xF, (bytes, bytearray, None), None
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


@dataclass
class EntryDef:
    index: int
    subindex: int
    data_type: DataType
    default: Union[int, float, bool, str, bytes, None]
    low_limit: Optional[int] = None
    high_limit: Optional[int] = None
    enum: Optional[Enum] = None
    bitfield: Optional[EntryBitField] = None


class Entry(EntryDef, Enum):
    """Virtual base class for entries."""

    def __hash__(self) -> int:
        return self.index << 8 + self.subindex

    def find_entry(self, index: int, subindex: int):
        for entry in list(self):
            if entry.index == index and entry.subindex:
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
            values[bitfield] = (value & self.bitfield.mask) >> self.bitfield.offset
        return values

    def decode(self, raw: bytes, use_enum: bool = True) -> Any:
        value = raw
        try:
            if self.data_type == DataType.STR:
                value = raw.decode()
            elif self.data_type == DataType.BYTES:
                value = raw
            elif self.data_type != DataType.DOMAIN:
                value = struct.unpack("<" + self.data_type.fmt, raw)[0]

            if use_enum and self.enum and isinstance(value, int) and value in self.enum:
                value = self.enum(value)
        except Exception as e:
            raise ValueError(f"{self.name} decode {e}") from e
        return value

    def encode(self, value: Any) -> bytes:
        if isinstance(value, Enum):
            value = value.value

        raw = value
        try:
            raw = b""
            if self.data_type == DataType.STR:
                raw = value.encode()
            elif self.data_type == DataType.BYTES:
                raw = value
            elif self.data_type != DataType.DOMAIN:
                raw = struct.pack("<" + self.data_type.fmt, value)
        except Exception as e:
            raise ValueError(f"{self.name} encode {e}") from e
        return raw
