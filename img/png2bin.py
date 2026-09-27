#!/usr/bin/env python3

"""Convert an indexed-color PNG into XRAM data for the VGA.

usage:
  png2bin.py sprite <bpp> <width>x<height> in.png pixels.bin palette.bin
  png2bin.py bitmap <bpp> in.png pixels.bin palette.bin
  png2bin.py tiles <bpp> in.png tiles.bin map.bin palette.bin

sprite  Mode 5 custom sprite images of width x height pixels, taken left to
        right, then top to bottom. Each row starts on a byte boundary.
bitmap  One mode 3 bitmap of the whole image.
tiles   Mode 2 8x8 tiles with duplicates removed, all 8 rows of one tile
        stored before the next tile, and a map of one tile number per byte,
        row by row. At most 256 tiles.

bpp is 1, 2, 4 or 8. The leftmost pixel is in the most significant bits of
each byte. The palette file contains one little-endian RGB555 color for each
PNG palette entry, with the alpha bit set when the tRNS alpha of that entry
is 128 or more. Entries with less alpha are 0x0000, which is transparent.
"""

import struct
import sys
import zlib


def read_png(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        sys.exit(f"{path}: not a PNG file")

    pos = 8
    idat = []
    plte = trns = b""
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b"IHDR":
            width, height, depth, color_type, _, _, interlace = struct.unpack(
                ">IIBBBBB", body)
        elif kind == b"PLTE":
            plte = body
        elif kind == b"tRNS":
            trns = body
        elif kind == b"IDAT":
            idat.append(body)

    if color_type != 3:
        sys.exit(f"{path}: not an indexed-color PNG file")
    if interlace:
        sys.exit(f"{path}: interlaced PNG files are not supported")

    stride = (width * depth + 7) // 8
    lines = unfilter(zlib.decompress(b"".join(idat)), stride, height)
    per_byte = 8 // depth
    mask = (1 << depth) - 1
    rows = [[line[x // per_byte] >> (8 - depth * (x % per_byte + 1)) & mask
             for x in range(width)] for line in lines]

    palette = []
    for i in range(len(plte) // 3):
        alpha = trns[i] if i < len(trns) else 255
        palette.append((*plte[i * 3:i * 3 + 3], alpha))
    return width, height, rows, palette


def unfilter(raw, stride, height):
    """Indexed pixels of every depth are filtered one byte at a time."""
    lines = []
    above = bytearray(stride)
    pos = 0
    for _ in range(height):
        kind = raw[pos]
        line = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        for i in range(stride):
            left = line[i - 1] if i else 0
            up = above[i]
            up_left = above[i - 1] if i else 0
            if kind == 0:
                predict = 0
            elif kind == 1:
                predict = left
            elif kind == 2:
                predict = up
            elif kind == 3:
                predict = (left + up) // 2
            else:
                p = left + up - up_left
                pa, pb, pc = abs(p - left), abs(p - up), abs(p - up_left)
                if pa <= pb and pa <= pc:
                    predict = left
                elif pb <= pc:
                    predict = up
                else:
                    predict = up_left
            line[i] = (line[i] + predict) & 0xFF
        lines.append(line)
        above = line
    return lines


def pack(pixels, bpp):
    out = bytearray()
    per_byte = 8 // bpp
    for i in range(0, len(pixels), per_byte):
        byte = 0
        for j, index in enumerate(pixels[i:i + per_byte]):
            byte |= index << (8 - bpp * (j + 1))
        out.append(byte)
    return out


def color(r, g, b, a):
    if a < 128:
        return 0x0000
    return (b >> 3) << 11 | (g >> 3) << 6 | r >> 3 | 0x20


def sprites(rows, width, height, frame_w, frame_h, bpp):
    out = bytearray()
    for top in range(0, height, frame_h):
        for left in range(0, width, frame_w):
            for row in rows[top:top + frame_h]:
                out += pack(row[left:left + frame_w], bpp)
    return out


def tiles(path, rows, width, height, bpp):
    ids = {}
    tile_map = []
    for top in range(0, height, 8):
        for left in range(0, width, 8):
            tile = b"".join(pack(row[left:left + 8], bpp)
                            for row in rows[top:top + 8])
            tile_map.append(ids.setdefault(tile, len(ids)))
    if len(ids) > 256:
        sys.exit(f"{path}: {len(ids)} different tiles, more than 256")
    return b"".join(ids), bytes(tile_map)


def write(path, data):
    with open(path, "wb") as f:
        f.write(data)


def main():
    args = sys.argv[1:]
    need = {"sprite": 6, "bitmap": 5, "tiles": 6}
    if (not args or args[0] not in need or len(args) != need[args[0]]
            or args[1] not in ("1", "2", "4", "8")):
        sys.exit(__doc__)
    kind, bpp = args[0], int(args[1])
    if kind == "sprite":
        size = args[2].split("x")
        if len(size) != 2 or not all(s.isdigit() and int(s) for s in size):
            sys.exit(__doc__)
        frame_w, frame_h = int(size[0]), int(size[1])
        args = args[3:]
    else:
        args = args[2:]
        frame_w = frame_h = 8 if kind == "tiles" else 1

    path = args[0]
    width, height, rows, palette = read_png(path)
    if len(palette) > 1 << bpp:
        sys.exit(f"{path}: {len(palette)} palette entries do not fit in "
                 f"{bpp}-bit color")
    highest = max(max(row) for row in rows)
    if highest >> bpp:
        sys.exit(f"{path}: color index {highest} does not fit in {bpp} bits")
    if width % frame_w or height % frame_h:
        sys.exit(f"{path}: {width}x{height} is not a multiple of "
                 f"{frame_w}x{frame_h}")

    if kind == "sprite":
        write(args[1], sprites(rows, width, height, frame_w, frame_h, bpp))
    elif kind == "bitmap":
        write(args[1], b"".join(pack(row, bpp) for row in rows))
    else:
        tile_data, tile_map = tiles(path, rows, width, height, bpp)
        write(args[1], tile_data)
        write(args[2], tile_map)
    write(args[-1], struct.pack(f"<{len(palette)}H",
                                *(color(*entry) for entry in palette)))


if __name__ == "__main__":
    main()
