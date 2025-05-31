class CandError(Exception):
    """Base CANd exception."""


class MessagePackCandError(CandError):
    """Failed to unpack a IPC message."""

    def __init__(self, class_name: str, values: tuple, error: str):
        super().__init__(f"failed to pack {class_name} with {values}: {error}")


class MessageVersionCandError(CandError):
    """IPC message had an invalid message protocol version."""

    def __init__(self, expected_version: int, version: int):
        super().__init__(f"protocal version byte must be {expected_version} got {version}")


class MessageIdCandError(CandError):
    """IPC message had an invalid message id."""

    def __init__(self, expected_id: int, id: int):
        super().__init__(f"protocal version byte must be {expected_id} got {id}")


class MessageUnpackCandError(CandError):
    """Failed to pack a IPC message."""

    def __init__(self, class_name: str, raw: bytes, error: str):
        super().__init__(f"failed to unpack {class_name} with {raw}: {error}")


class GenericCandError(CandError):
    """A IPC request returned a error."""

    def __init__(self, error: int):
        super().__init__(f"generic cand error {error}")
        self.error = error


class SdoAbortCandError(CandError):
    """A IPC SDO read/write request returned a SDO abort code."""

    def __init__(self, code: int):
        super().__init__(f"sdo abort 0x{code:X}")
        self.code = code


class UnknownIdCandError(CandError):
    """A IPC request had a unknown id."""

    def __init__(self, id: int):
        super().__init__(f"unknown message id {id}")
        self.id = id
