import os
import unittest

from oresat_cand.message import (
    AddFileMessage,
    BusStateMessage,
    EmcyRecvMessage,
    EmcySendMessage,
    ErrorMessage,
    HbRecvMessage,
    OdWriteMessage,
    SdoAbortErrorMessage,
    SdoReadMessage,
    SdoReadToFileMessage,
    SdoWriteFromFileMessage,
    SdoWriteMessage,
    SyncSendMessage,
    TpdoSendMessage,
    UnknownIdErrorMessage,
)


class TestEmcySendMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        msg = EmcySendMessage(1, 2)
        raw = msg.pack()
        msg2 = EmcySendMessage.unpack(raw)
        self.assertEqual(msg, msg2)


class TestTpdoSendMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        msg = TpdoSendMessage(3)
        raw = msg.pack()
        msg2 = TpdoSendMessage.unpack(raw)
        self.assertEqual(msg, msg2)


class TestOdWriteMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        msg = OdWriteMessage(0x7000, 0x1, b"\x12\x34")
        raw = msg.pack()
        msg2 = OdWriteMessage.unpack(raw)
        self.assertEqual(msg, msg2)


class TestSdoReadMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        msg = SdoReadMessage(0x10, 0x7000, 0x1, b"\x12\x34")
        raw = msg.pack()
        msg2 = SdoReadMessage.unpack(raw)
        self.assertEqual(msg, msg2)


class TestSdoWriteMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        msg = SdoWriteMessage(0x10, 0x7000, 0x1, b"\x12\x34")
        raw = msg.pack()
        msg2 = SdoWriteMessage.unpack(raw)
        self.assertEqual(msg, msg2)


class TestAddFileMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        test_file = "/tmp/test.txt"
        with open(test_file, "w") as f:
            f.write("test")
        msg = AddFileMessage(test_file)
        raw = msg.pack()
        msg2 = AddFileMessage.unpack(raw)
        self.assertEqual(msg, msg2)
        os.remove(test_file)


class TestHbRecvMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        msg = HbRecvMessage(0x11, 0x5)
        raw = msg.pack()
        msg2 = HbRecvMessage.unpack(raw)
        self.assertEqual(msg, msg2)


class TestEmcyRecvMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        msg = EmcyRecvMessage(0x11, 0x1234, 0xABCD)
        raw = msg.pack()
        msg2 = EmcyRecvMessage.unpack(raw)
        self.assertEqual(msg, msg2)


class TestSyncSendMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        msg = SyncSendMessage()
        raw = msg.pack()
        msg2 = SyncSendMessage.unpack(raw)
        self.assertEqual(msg, msg2)


class TestBusStateMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        msg = BusStateMessage(0x1)
        raw = msg.pack()
        msg2 = BusStateMessage.unpack(raw)
        self.assertEqual(msg, msg2)


class TestSdoReadFileMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        msg = SdoReadToFileMessage(0x1, "remote.txt")
        raw = msg.pack()
        msg2 = SdoReadToFileMessage.unpack(raw)
        self.assertEqual(msg, msg2)


class TestSdoWriteFileMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        test_file = "/tmp/test.txt"
        with open(test_file, "w") as f:
            f.write("test")
        msg = SdoWriteFromFileMessage(0x1, test_file)
        raw = msg.pack()
        msg2 = SdoWriteFromFileMessage.unpack(raw)
        self.assertEqual(msg, msg2)
        os.remove(test_file)


class TestErrorMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        msg = ErrorMessage(0x1234)
        raw = msg.pack()
        msg2 = ErrorMessage.unpack(raw)
        self.assertEqual(msg, msg2)


class TestUnknownIdErrorMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        msg = UnknownIdErrorMessage(0x40)
        raw = msg.pack()
        msg2 = UnknownIdErrorMessage.unpack(raw)
        self.assertEqual(msg, msg2)


class TestAbortErrorMessage(unittest.TestCase):
    def test_pack_unpack(self) -> None:
        msg = SdoAbortErrorMessage(0x1234)
        raw = msg.pack()
        msg2 = SdoAbortErrorMessage.unpack(raw)
        self.assertEqual(msg, msg2)
