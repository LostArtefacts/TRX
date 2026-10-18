"""Reads files from a PlayStation disc image in raw MODE2/2352 form."""

import struct
from collections.abc import Iterator
from pathlib import Path

SECTOR_SIZE = 2352
SECTOR_DATA_OFFSET = 24
SECTOR_DATA_SIZE = 2048


class Disc:
    def __init__(self, path: Path) -> None:
        self.data = path.read_bytes()

    def _sector(self, lba: int) -> bytes:
        start = lba * SECTOR_SIZE + SECTOR_DATA_OFFSET
        return self.data[start : start + SECTOR_DATA_SIZE]

    def _extent(self, lba: int, size: int) -> bytes:
        sectors = (size + SECTOR_DATA_SIZE - 1) // SECTOR_DATA_SIZE
        return b"".join(self._sector(lba + i) for i in range(sectors))[:size]

    def _walk(self, lba: int, size: int, prefix: str) -> Iterator[tuple[str, int, int]]:
        directory = self._extent(lba, size)
        pos = 0
        while pos < len(directory):
            record_size = directory[pos]
            if record_size == 0:
                pos = (pos // SECTOR_DATA_SIZE + 1) * SECTOR_DATA_SIZE
                continue
            extent, = struct.unpack_from("<I", directory, pos + 2)
            length, = struct.unpack_from("<I", directory, pos + 10)
            is_dir = directory[pos + 25] & 2
            name_size = directory[pos + 32]
            name = directory[pos + 33 : pos + 33 + name_size].decode("latin1")
            if name not in ("\x00", "\x01"):
                path = prefix + name.split(";")[0]
                if is_dir:
                    yield from self._walk(extent, length, path + "/")
                else:
                    yield path, extent, length
            pos += record_size

    def files(self) -> dict[str, tuple[int, int]]:
        descriptor = self._sector(16)
        if descriptor[1:6] != b"CD001":
            raise ValueError("not an ISO 9660 MODE2/2352 image")
        root = descriptor[156:190]
        lba, = struct.unpack_from("<I", root, 2)
        size, = struct.unpack_from("<I", root, 10)
        return {
            path: (extent, length)
            for path, extent, length in self._walk(lba, size, "")
        }

    def read(self, path: str) -> bytes:
        extent, length = self.files()[path]
        return self._extent(extent, length)
