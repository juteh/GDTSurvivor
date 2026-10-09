"""
Generates the pixel-art textures for HUD resource bars (health, shield) as PNG files.
No external dependencies (PIL/numpy) - pure stdlib, so it runs with any Python 3
(e.g. the one bundled with the engine: Engine/Binaries/ThirdParty/Python3/Win64/python.exe).

Usage:
    python pixel_hud_bar_generator.py

Produces (in Tools/pixel_art_output/):
    hud_bar_frame.png    - 12x12 nine-slice frame with chamfered corners (Draw As: Box, margin 0.25)
    hud_bar_segment.png  - 6x8 grayscale fill segment with a 1 px gap (Draw As: Image, tiling: Horizontal)

The segment is grayscale on purpose: the progress bar's Fill Color tints it, so the same
texture serves the red health bar and the cyan shield bar. Import both with
Filter = Nearest, Compression = UserInterface2D, Mip Gen = NoMipmaps, so the pixels stay crisp.
"""
import os
import struct
import zlib

OUT_DIR = os.path.join(os.path.dirname(__file__), "pixel_art_output")
os.makedirs(OUT_DIR, exist_ok=True)

CLEAR = (0, 0, 0, 0)

# ---- frame palette: cool steel border around a dark, slightly see-through core ----
FRAME_LIGHT = (170, 190, 225, 255)  # outer border
FRAME_DARK  = (40, 48, 80, 255)     # inner border line
FRAME_CORE  = (10, 10, 24, 210)     # empty part of the bar

# ---- segment shading (multiplied by the fill color in UMG) ----
SEG_HIGHLIGHT = (255, 255, 255, 255)
SEG_BODY      = (215, 215, 215, 255)
SEG_SHADOW    = (140, 140, 140, 255)


def write_png(path, pixels):
    """pixels: list of rows, each a list of RGBA tuples."""
    height, width = len(pixels), len(pixels[0])
    raw = b"".join(b"\x00" + b"".join(struct.pack("4B", *px) for px in row) for row in pixels)

    def chunk(tag, data):
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))


def frame():
    size = 12
    last = size - 1
    rows = []
    for y in range(size):
        row = []
        for x in range(size):
            edge = min(x, y, last - x, last - y)
            corner = min(x, last - x) + min(y, last - y)
            if corner == 0:
                row.append(CLEAR)          # chamfer: cut the outermost corner pixel
            elif edge == 0 or corner == 1:
                row.append(FRAME_LIGHT)    # outer border, running diagonally around the cut
            elif edge == 1:
                row.append(FRAME_DARK)
            else:
                row.append(FRAME_CORE)
        rows.append(row)
    return rows


def segment():
    width, height = 6, 8
    rows = []
    for y in range(height):
        if y == 0:
            shade = SEG_HIGHLIGHT
        elif y >= height - 2:
            shade = SEG_SHADOW
        else:
            shade = SEG_BODY
        rows.append([shade] * (width - 1) + [CLEAR])
    return rows


if __name__ == "__main__":
    write_png(os.path.join(OUT_DIR, "hud_bar_frame.png"), frame())
    write_png(os.path.join(OUT_DIR, "hud_bar_segment.png"), segment())
    print("Wrote hud_bar_frame.png and hud_bar_segment.png to", OUT_DIR)
