import unittest
from enum import Enum

from oresat_cand.entry import DataType, Entry, EntryBitField


class TestDataBitField(EntryBitField):
    FIELD_1 = 1, 0
    FIELD_2 = 1, 1
    FIELD_3 = 2, 4


class TestDataEnum(Enum):
    VALUE_1 = 0
    VALUE_2 = 2
    VALUE_3 = 5


class TestDataEntry(Entry):
    ENTRY_UINT8 = 0x6000, 0x1, DataType.UINT8, 1, 0, 100, None, TestDataBitField
    ENTRY_UINT16 = 0x6000, 0x2, DataType.UINT16, 1, 0, 0, TestDataEnum, None
    ENTRY_UINT32 = 0x6000, 0x3, DataType.UINT32, 1
    ENTRY_UINT64 = 0x6000, 0x4, DataType.UINT64, 1
    ENTRY_INT8 = 0x6000, 0x5, DataType.INT8, 1
    ENTRY_INT16 = 0x6000, 0x6, DataType.INT16, 1
    ENTRY_INT32 = 0x6000, 0x7, DataType.INT32, 1
    ENTRY_INT64 = 0x6000, 0x8, DataType.INT64, 1
    ENTRY_FLOAT32 = 0x6000, 0x5, DataType.FLOAT32, 1.0
    ENTRY_FLOAT64 = 0x6000, 0x6, DataType.FLOAT64, 1.0
    ENTRY_STR = 0x6000, 0x7, DataType.STR, ""
    ENTRY_STR_2 = 0x6000, 0x8, DataType.STR, "123"
    ENTRY_OCTET_STR = 0x6000, 0x9, DataType.OCTET_STR, b""
    ENTRY_OCTET_STR_2 = 0x6000, 0xA, DataType.OCTET_STR, b"\x01"
    ENTRY_DOMAIN = 0x6000, 0xB, DataType.DOMAIN, None
    ENTRY_BOOL = 0x6000, 0xC, DataType.BOOL, True
    ENTRY_BOOL_2 = 0x6000, 0xD, DataType.BOOL, 1


class TestEntry(unittest.TestCase):
    def test_encode_decode(self) -> None:
        for entry in list(TestDataEntry):
            raw = entry.encode(entry.default)
            value = entry.decode(raw)
            if entry.data_type == DataType.DOMAIN:
                self.assertEqual(value, b"", entry.name)
            else:
                self.assertEqual(value, entry.default, entry.name)

        # Domain default is always None, test encode/decode with bytes data
        for value in [b"", b"\x00", b"\x12\x34"]:
            raw = TestDataEntry.ENTRY_DOMAIN.encode(value)
            value_2 = TestDataEntry.ENTRY_DOMAIN.decode(raw)
            self.assertEqual(value, value_2)

    def test_find(self) -> None:
        self.assertEqual(TestDataEntry.find(0x6000, 0x4), TestDataEntry.ENTRY_UINT64)
        with self.assertRaises(ValueError):
            TestDataEntry.find(0x1234, 0)

    def test_bitfield(self) -> None:
        entry = TestDataEntry.ENTRY_UINT8
        for value in [0, 1, 2, 3, 51]:
            bitfield_value = entry.value_to_bitfield(value)
            value_2 = entry.bitfield_to_value(bitfield_value)
            self.assertEqual(value, value_2)

    def test_enum(self) -> None:
        entry = TestDataEntry.ENTRY_UINT16
        for e in list(TestDataEnum):
            raw = entry.encode(e)
            self.assertEqual(raw, e.value.to_bytes(2, "little"))
            e2 = entry.decode_to_enum(raw)
            self.assertEqual(e, e2)
