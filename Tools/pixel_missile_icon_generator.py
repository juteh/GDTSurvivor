"""
Generates the weapon icon for the homing missile as a PNG file: a small missile flying to the
bottom right with a curved exhaust trail. The trail uses the same glow layers as the laser icon
(pixel_laser_icon_generator.py), in the yellow/orange of NS_RocketExhaust_Yellow; the curve
shows that the missile homes in on its target.
No external dependencies (PIL/numpy) - pure stdlib, so it runs with any Python 3
(e.g. the one bundled with the engine: Engine/Binaries/ThirdParty/Python3/Win64/python.exe).

Usage:
    python pixel_missile_icon_generator.py

Produces (in Tools/pixel_art_output/):
    homing_missile_trail.png - 52x52 icon with a transparent background

Import with Filter = Nearest, Compression = UserInterface2D, Mip Gen = NoMipmaps,
so the pixels stay crisp.
"""
import math
import os
import struct
import zlib

OUT_DIR = os.path.join(os.path.dirname(__file__), "pixel_art_output")
os.makedirs(OUT_DIR, exist_ok=True)

SIZE = 52

# ---- missile geometry (pixel coordinates) ----
NOZZLE = (23.5, 23.5)          # rear end of the missile, where the trail starts
DIRECTION = (1 / math.sqrt(2), 1 / math.sqrt(2))  # flight direction (bottom right)
BODY_LENGTH = 23.0             # nozzle to nose tip
BODY_HALF_WIDTH = 3.6
NOSE_LENGTH = 7.0              # last part of the body that tapers to the tip
FIN_LENGTH = 8.0               # fins at the rear
FIN_HALF_WIDTH = 6.6
STRIPE = (13.5, 15.0)          # colored ring around the body (distance from the nozzle)

# ---- missile palette ----
OUTLINE = (35, 42, 62, 255)
SHADOW = (105, 120, 148, 255)
BODY = (172, 188, 210, 255)
HIGHLIGHT = (228, 238, 252, 255)
STRIPE_COLOR = (255, 190, 40, 255)
FIN_COLOR = (120, 135, 165, 255)

# ---- exhaust trail: quadratic curve from the nozzle back to its end ----
TRAIL_CONTROL = (13.0, 13.0)
TRAIL_END = (4.5, 19.0)
TRAIL_START_RADIUS = 3.2
TRAIL_END_RADIUS = 0.7

# From the inside out: (max distance / radius, RGBA)
TRAIL_BANDS = [
    (0.40, (255, 250, 225, 255)),  # white-hot core
    (0.75, (255, 215, 60, 255)),   # yellow
    (1.05, (255, 135, 20, 255)),   # orange
    (1.60, (230, 70, 0, 150)),     # outer glow
    (2.20, (180, 40, 0, 60)),      # faint halo
]

SPARKS = [(10, 27), (22, 9)]
SPARK_CENTER = (255, 240, 180, 230)
SPARK_ARM = (255, 135, 20, 150)


def trail_points(steps=200):
    """Points along the exhaust curve with their parameter: 0 = nozzle, 1 = end of the trail."""
    points = []
    for step in range(steps + 1):
        t = step / steps
        a, b, c = (1 - t) ** 2, 2 * (1 - t) * t, t * t
        points.append((t, a * NOZZLE[0] + b * TRAIL_CONTROL[0] + c * TRAIL_END[0],
                          a * NOZZLE[1] + b * TRAIL_CONTROL[1] + c * TRAIL_END[1]))
    return points


def trail_pixel(cx, cy, points):
    best_t, best_distance = 0.0, float("inf")
    for t, px, py in points:
        distance = math.hypot(cx - px, cy - py)
        if distance < best_distance:
            best_t, best_distance = t, distance

    # Thick and bright at the nozzle, thin and faded at the end.
    closeness = 1.0 - best_t
    radius = TRAIL_END_RADIUS + (TRAIL_START_RADIUS - TRAIL_END_RADIUS) * (closeness ** 1.3)
    relative = best_distance / radius
    fade = 0.25 + 0.75 * closeness ** 0.8
    for index, (limit, color) in enumerate(TRAIL_BANDS):
        if relative > limit:
            continue
        if index == 0 and closeness < 0.25:
            continue  # no white core in the thin part of the trail
        alpha = color[3] * (fade if index >= 2 else min(1.0, fade * 1.6))
        if alpha < 20:
            return None
        return (color[0], color[1], color[2], int(alpha))
    return None


def missile_pixel(cx, cy):
    # Missile space: u along the flight direction from the nozzle, v across it
    # (negative v = the upper right side, which catches the light).
    rx, ry = cx - NOZZLE[0], cy - NOZZLE[1]
    u = rx * DIRECTION[0] + ry * DIRECTION[1]
    v = rx * DIRECTION[1] - ry * DIRECTION[0]
    if u < 0 or u > BODY_LENGTH:
        return None

    # Body: straight, then a rounded nose.
    nose_start = BODY_LENGTH - NOSE_LENGTH
    if u <= nose_start:
        half_width = BODY_HALF_WIDTH
    else:
        k = (u - nose_start) / NOSE_LENGTH
        half_width = BODY_HALF_WIDTH * math.sqrt(max(0.0, 1 - k * k))

    # Fins: full width at the rear, then swept back into the body.
    fin_full = FIN_LENGTH * 0.35
    if u < fin_full:
        fin_half_width = FIN_HALF_WIDTH
    elif u < FIN_LENGTH:
        fin_half_width = FIN_HALF_WIDTH + (BODY_HALF_WIDTH - FIN_HALF_WIDTH) * (u - fin_full) / (FIN_LENGTH - fin_full)
    else:
        fin_half_width = 0.0

    distance = abs(v)
    if distance <= half_width:
        if distance > half_width - 0.9:
            return OUTLINE
        if STRIPE[0] <= u <= STRIPE[1]:
            return STRIPE_COLOR
        side = v / max(half_width, 0.01)
        if side < -0.35:
            return HIGHLIGHT
        if side > 0.35:
            return SHADOW
        return BODY
    if distance <= fin_half_width:
        return OUTLINE if distance > fin_half_width - 0.9 else FIN_COLOR
    return None


def build_icon():
    pixels = [[(0, 0, 0, 0)] * SIZE for _ in range(SIZE)]
    points = trail_points()
    for y in range(SIZE):
        for x in range(SIZE):
            cx, cy = x + 0.5, y + 0.5
            color = missile_pixel(cx, cy) or trail_pixel(cx, cy, points)
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
    path = os.path.join(OUT_DIR, "homing_missile_trail.png")
    write_png(path, build_icon())
    print("Wrote", path)
