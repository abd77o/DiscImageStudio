#!/usr/bin/env python3
"""Write a deterministic 512x512 checkerboard BMP for the hardware-free tests.

Standard library only, so CI needs no imaging packages:

    python3 tests/make_test_pattern.py ci-pattern.bmp
"""
import struct
import sys

SIZE = 512
CELL = 32


def main() -> int:
    path = sys.argv[1] if len(sys.argv) > 1 else "test_pattern.bmp"
    row_bytes = SIZE * 3
    pad = (-row_bytes) % 4
    pixel_data = bytearray()
    for y in range(SIZE - 1, -1, -1):  # BMP rows are bottom-up
        for x in range(SIZE):
            value = 255 if ((x // CELL) + (y // CELL)) % 2 else 0
            pixel_data += bytes((value, value, value))
        pixel_data += b"\x00" * pad

    offset = 14 + 40
    header = struct.pack("<2sIHHI", b"BM", offset + len(pixel_data), 0, 0, offset)
    info = struct.pack(
        "<IiiHHIIiiII", 40, SIZE, SIZE, 1, 24, 0, len(pixel_data), 2835, 2835, 0, 0
    )
    with open(path, "wb") as f:
        f.write(header + info + pixel_data)
    print(f"wrote {path} ({SIZE}x{SIZE}, {CELL}px checkerboard, 24 bpp)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
