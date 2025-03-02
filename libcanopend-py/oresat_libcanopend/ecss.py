"""ECSS Space engineering CANbus extension protocol time formats"""

from dataclasses import dataclass
from time import time

SEC_IN_DAY = 86_400
MSEC_IN_SEC = 1_000_000
USEC_IN_SEC = 1_000_000


@dataclass
class EcssScet:
    coarse: int
    fine: int  # sub seconds

    @classmethod
    def from_time(cls, unix_time: float):
        coarse = int(unix_time)
        fine = int(unix_time % 1 * USEC_IN_SEC)
        return cls(coarse, fine)

    def to_time(self) -> float:
        return self.coarse + self.fine / USEC_IN_SEC

    @classmethod
    def from_uint64(cls, value: int):
        raw = value.to_bytes(8, "little")
        coarse = int.from_bytes(raw[:4], "little")
        fine = int.from_bytes(raw[4:-1], "little")
        return cls(coarse, fine)

    def to_uint64(self) -> int:
        coarse_bytes = self.coarse.to_bytes(4, "little")
        fine_bytes = self.fine.to_bytes(3, "little")
        raw = coarse_bytes + fine_bytes + b"\x00"
        return int.from_bytes(raw, "little")

    @classmethod
    def now(cls):
        return cls.from_time(time())


@dataclass
class EcssUtc:
    day: int
    milliseconds: int
    microseconds: int

    @classmethod
    def from_time(cls, unix_time: float):
        day = int(unix_time / SEC_IN_DAY)
        temp_us = unix_time % SEC_IN_DAY * USEC_IN_SEC
        ms_of_day = int(temp_us / MSEC_IN_SEC)
        us_of_day = int(temp_us % MSEC_IN_SEC)
        return cls(day, ms_of_day, us_of_day)

    def to_time(self) -> float:
        return (
            self.day * SEC_IN_DAY
            + self.milliseconds / MSEC_IN_SEC
            + self.microseconds / USEC_IN_SEC
        )

    @classmethod
    def from_uint64(cls, value: int):
        raw = value.to_bytes(8, "little")
        day = int.from_bytes(raw[:2], "little")
        ms_of_day = int.from_bytes(raw[2:-2], "little")
        us_of_day = int.from_bytes(raw[-2:], "little")
        return cls(day, ms_of_day, us_of_day)

    def to_uint64(self) -> int:
        day_bytes = self.day.to_bytes(2, "little")
        ms_of_day_bytes = self.milliseconds.to_bytes(4, "little")
        us_of_day_bytes = self.microseconds.to_bytes(2, "little")
        raw = day_bytes + ms_of_day_bytes + us_of_day_bytes
        return int.from_bytes(raw, "little")

    @classmethod
    def now(cls):
        return cls.from_time(time())
