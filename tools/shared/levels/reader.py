import struct


class Reader:
    def __init__(self, data: bytes, pos: int = 0) -> None:
        self.data = data
        self.pos = pos

    def _unpack(self, fmt: str) -> tuple:
        values = struct.unpack_from(fmt, self.data, self.pos)
        self.pos += struct.calcsize(fmt)
        return values

    def u8(self) -> int:
        return self._unpack("<B")[0]

    def u16(self) -> int:
        return self._unpack("<H")[0]

    def s16(self) -> int:
        return self._unpack("<h")[0]

    def u32(self) -> int:
        return self._unpack("<I")[0]

    def s32(self) -> int:
        return self._unpack("<i")[0]

    def u16s(self, count: int) -> list[int]:
        return list(self._unpack(f"<{count}H"))

    def s16s(self, count: int) -> list[int]:
        return list(self._unpack(f"<{count}h"))

    def s32s(self, count: int) -> list[int]:
        return list(self._unpack(f"<{count}i"))

    def skip(self, count: int) -> None:
        self.pos += count
