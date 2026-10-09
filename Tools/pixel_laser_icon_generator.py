"""
Generates the weapon icon for the player laser as a PNG file: a glowing bolt in flight, styled like
the projectile effect NS_Projectile_Body_GDT (white-hot core, pink inner glow, red outer glow).
No external dependencies (PIL/numpy) - pure stdlib, so it runs with any Python 3
(e.g. the one bundled with the engine: Engine/Binaries/ThirdParty/Python3/Win64/python.exe).

Usage:
    python pixel_laser_icon_generator.py

Produces (in Tools/pixel_art_output/):
    player_laser_bolt.png - 52x52 icon with a transparent background

The bolt flies from the top left to the bottom right: a thick, bright head and a trail that gets
thinner and fades out. Import with Filter = Nearest, Compression = UserInterface2D,
Mip Gen = NoMipmaps, so the pixels stay crisp.
"""
import math
import os
import struct
import zlib

OUT_DIR = os.path.join(os.path.dirname(__file__), "pixel_art_output")
os.makedirs(OUT_DIR, exist_ok=True)

SIZE = 52

# ---- bolt geometry (pixel coordinates) ----
TAIL = (7.5, 7.5)     # end of the trail
HEAD = (40.5, 40.5)   # front of the bolt
HEAD_RADIUS = 4.4     # half thickness at the head
TAIL_RADIUS = 1.0     # half thickness at the end of the trail

# ---- palette, from the inside out: (max distance / radius, RGBA) ----
BANDS = [
    (0.40, (255, 245, 248, 255)),  # white-hot core
    (0.75, (255, 120, 160, 255)),  # pink inner glow
    (1.05, (235, 25, 75, 255)),    # red body
    (1.60, (190, 0, 50, 150)),     # outer glow
    (2.20, (150, 0, 40, 60)),      # faint halo
]

# Small sparks left behind by the bolt: center of a 3x3 cross with a bright middle pixel.
SPARKS = [(16, 28), (30, 18)]
SPARK_CENTER = (255, 200, 215, 230)
SPARK_ARM = (235, 25, 75, 150)


def bolt_pixel(x, y):
    """Color of the pixel at (x, y), or None if the bolt does not cover it."""
    cx, cy = x + 0.5, y + 0.5
    dx, dy = HEAD[0] - TAIL[0], HEAD[1] - TAIL[1]
    length_sq = dx * dx + dy * dy

    # Position along the bolt: 0 = tail, 1 = head. Beyond the head the bolt is rounded.
    t = ((cx - TAIL[0]) * dx + (cy - TAIL[1]) * dy) / length_sq
    clamped = min(max(t, 0.0), 1.0)
    px, py = TAIL[0] + clamped * dx, TAIL[1] + clamped * dy
    distance = math.hypot(cx - px, cy - py)

    # The trail tapers towards the tail; the head stays round.
    radius = TAIL_RADIUS + (HEAD_RADIUS - TAIL_RADIUS) * (clamped ** 1.3)
    relative = distance / radius

    # The trail fades out: the outer bands lose opacity first, the core is cut off near the tail.
    fade = 0.25 + 0.75 * clamped ** 0.8
    for index, (limit, color) in enumerate(BANDS):
        if relative > limit:
            continue
        if index == 0 and clamped < 0.25:
            continue  # no white core in the thin part of the trail
        alpha = color[3] * (fade if index >= 2 else min(1.0, fade * 1.6))
        if alpha < 20:
            return None
        return (color[0], color[1], color[2], int(alpha))
    return None


def build_icon():
    pixels = [[(0, 0, 0, 0)] * SIZE for _ in range(SIZE)]
    for y in range(SIZE):
        for x in range(SIZE):
            color = bolt_pixel(x, y)
            if color:
                pixels[y][x] = color
    for x, y in SPARKS:
        for ox, oy, color in [(0, 0, SPARK_CENTER), (-1, 0, SPARK_ARM), (1, 0, SPARK_ARM), (0, -1, SPARK_ARM), (0, 1, SPARK_ARM)]:
            if pixels[y + oy][x + ox][3] == 0:
                pixels[y + oy][x + ox] = color
    return pixels


def write_png(path, pixels):
    """pixels: list of rows, each a list of RGBA tuples."""
    height, width = len(pixels), len(pixels[0])
    raw = b"".join(b"\x00" + b"".join(struct.pack("4B", *px) for px in row) for row in pixels)

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    with open(path, "wb") as file:
        file.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


if __name__ == "__main__":
    path = os.path.join(OUT_DIR, "player_laser_bolt.png")
    write_png(path, build_icon())
    print("Wrote", path)
