"""
Generates a small set of minimalist pixel-art space backgrounds as PNG files.
No external dependencies (PIL/numpy) - pure stdlib, so it runs with any Python 3.

Usage:
    python pixel_starfield_generator.py

Produces (next to this script, in Tools/pixel_art_output/):
    starfield_topdown_tile.png   - seamless tile for a flat background plane (top-down view)
    starfield_longlat.png        - 2:1 equirectangular map, for converting into a TextureCube
                                    (drop-in replacement for the existing skydome material)
    starfield_palette_only.png   - just the palette swatches used, for reference

Each is authored at a small internal resolution and nearest-neighbour upscaled, so the
pixels stay crisp/chunky (genuine pixel art) instead of being a smooth photo scaled down.
"""
import os
import random
import struct
import zlib

OUT_DIR = os.path.join(os.path.dirname(__file__), "pixel_art_output")
os.makedirs(OUT_DIR, exist_ok=True)

random.seed(1337)

# ---- tiny palette: deep space with a couple of accent colors (classic pixel-art restraint) ----
BG_DARK    = (10, 8, 22)     # near-black space
BG_MID     = (18, 14, 36)    # slightly lighter band
BG_NEBULA1 = (32, 18, 48)    # dim purple nebula
BG_NEBULA2 = (14, 30, 42)    # dim teal nebula
STAR_DIM   = (90, 96, 130)
STAR_MID   = (180, 190, 220)
STAR_BRIGHT= (255, 250, 230)
STAR_ACCENT= (255, 190, 120)  # rare warm star


def write_png(path, pixels, width, height):
    """pixels: list of rows, each row a list of (r,g,b) tuples. Writes an 8-bit RGB PNG."""
    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    raw = bytearray()
    for row in pixels:
        raw.append(0)  # filter type 0 (none)
        for (r, g, b) in row:
            raw += bytes((r, g, b))

    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)  # bit depth 8, color type 2 (RGB)
    idat = zlib.compress(bytes(raw), 9)
    with open(path, "wb") as f:
        f.write(sig)
        f.write(chunk(b"IHDR", ihdr))
        f.write(chunk(b"IDAT", idat))
        f.write(chunk(b"IEND", b""))


def nearest_upscale(pixels, factor):
    out = []
    for row in pixels:
        new_row = []
        for px in row:
            new_row.extend([px] * factor)
        for _ in range(factor):
            out.append(new_row)
    return out


def blank_canvas(w, h, color):
    return [[color for _ in range(w)] for _ in range(h)]


def add_flat_bands(canvas, colors_top_to_bottom):
    """Discrete horizontal flat-color bands (no per-pixel dithering) - classic pixel-art sky."""
    w, h = len(canvas[0]), len(canvas)
    n = len(colors_top_to_bottom)
    for y in range(h):
        band = min(n - 1, (y * n) // h)
        for x in range(w):
            canvas[y][x] = colors_top_to_bottom[band]


def scatter_stars(canvas, count, color, cluster=False, cluster_spread=6):
    w, h = len(canvas[0]), len(canvas)
    for _ in range(count):
        if cluster:
            cx, cy = random.randrange(w), random.randrange(h)
            x = min(w - 1, max(0, cx + random.randint(-cluster_spread, cluster_spread)))
            y = min(h - 1, max(0, cy + random.randint(-cluster_spread, cluster_spread)))
        else:
            x, y = random.randrange(w), random.randrange(h)
        canvas[y][x] = color


def scatter_nebula_blob(canvas, cx, cy, radius, color, density=0.35):
    w, h = len(canvas[0]), len(canvas)
    for dy in range(-radius, radius + 1):
        for dx in range(-radius, radius + 1):
            x, y = cx + dx, cy + dy
            if 0 <= x < w and 0 <= y < h:
                dist = (dx * dx + dy * dy) ** 0.5
                if dist <= radius and random.random() < density * (1 - dist / radius):
                    canvas[y][x] = color


def draw_small_planet(canvas, cx, cy, radius, base_color, rim_color):
    w, h = len(canvas[0]), len(canvas)
    for dy in range(-radius, radius + 1):
        for dx in range(-radius, radius + 1):
            x, y = cx + dx, cy + dy
            if 0 <= x < w and 0 <= y < h:
                dist2 = dx * dx + dy * dy
                if dist2 <= radius * radius:
                    canvas[y][x] = rim_color if dist2 >= (radius - 1) * (radius - 1) else base_color


# =========================================================================
# 1) Top-down tile: a seamless-ish square tile for a flat background plane
# =========================================================================
BASE = 64  # internal pixel resolution
UPSCALE = 8  # -> 512x512 export, each "pixel" is an 8x8 block on screen

canvas = blank_canvas(BASE, BASE, BG_DARK)
add_flat_bands(canvas, [BG_MID, BG_DARK, BG_DARK])
scatter_nebula_blob(canvas, 14, 44, 10, BG_NEBULA1, density=0.4)
scatter_nebula_blob(canvas, 46, 16, 8, BG_NEBULA2, density=0.35)
scatter_stars(canvas, 90, STAR_DIM)
scatter_stars(canvas, 28, STAR_MID)
scatter_stars(canvas, 6, STAR_BRIGHT)
scatter_stars(canvas, 2, STAR_ACCENT)
draw_small_planet(canvas, 50, 50, 3, (60, 70, 90), (120, 130, 150))

up = nearest_upscale(canvas, UPSCALE)
write_png(os.path.join(OUT_DIR, "starfield_topdown_tile.png"), up, BASE * UPSCALE, BASE * UPSCALE)


# =========================================================================
# 2) Equirectangular long-lat map (2:1) - for converting into a TextureCube
#    to drop straight into the existing SkyboxMaterial / BP_SkyStarfield_SphereMap.
# =========================================================================
LL_W, LL_H = 128, 64
canvas2 = blank_canvas(LL_W, LL_H, BG_DARK)
add_flat_bands(canvas2, [BG_MID, BG_DARK, BG_DARK, BG_DARK])
for _ in range(6):
    scatter_nebula_blob(canvas2, random.randrange(LL_W), random.randrange(LL_H),
                         random.randint(6, 14),
                         random.choice([BG_NEBULA1, BG_NEBULA2]), density=0.3)
scatter_stars(canvas2, 260, STAR_DIM)
scatter_stars(canvas2, 80, STAR_MID)
scatter_stars(canvas2, 16, STAR_BRIGHT)
scatter_stars(canvas2, 5, STAR_ACCENT)
draw_small_planet(canvas2, 100, 12, 4, (70, 55, 50), (140, 110, 95))
draw_small_planet(canvas2, 20, 50, 2, (55, 70, 65), (110, 140, 130))

UPSCALE2 = 6
up2 = nearest_upscale(canvas2, UPSCALE2)
write_png(os.path.join(OUT_DIR, "starfield_longlat.png"), up2, LL_W * UPSCALE2, LL_H * UPSCALE2)


# =========================================================================
# 2b) Widescreen (16:9) version for a single background plane behind the play field
#     (avoids the squish you'd get stretching the square tile onto a wide plane)
# =========================================================================
PW, PH = 160, 90
canvas3 = blank_canvas(PW, PH, BG_DARK)
add_flat_bands(canvas3, [BG_MID, BG_DARK, BG_DARK])
scatter_nebula_blob(canvas3, 40, 60, 16, BG_NEBULA1, density=0.35)
scatter_nebula_blob(canvas3, 120, 20, 14, BG_NEBULA2, density=0.3)
scatter_stars(canvas3, 260, STAR_DIM)
scatter_stars(canvas3, 80, STAR_MID)
scatter_stars(canvas3, 16, STAR_BRIGHT)
scatter_stars(canvas3, 5, STAR_ACCENT)
draw_small_planet(canvas3, 135, 65, 5, (60, 70, 90), (130, 140, 160))

UPSCALE3 = 8
up3 = nearest_upscale(canvas3, UPSCALE3)
write_png(os.path.join(OUT_DIR, "starfield_background_plane.png"), up3, PW * UPSCALE3, PH * UPSCALE3)


# =========================================================================
# 2c) Tileable pure starfield (no planet/blob - safe to repeat via UV tiling
#     in the material) so scaling the plane doesn't have to make stars bigger.
# =========================================================================
TW, TH = 96, 96
canvas4 = blank_canvas(TW, TH, BG_DARK)
add_flat_bands(canvas4, [BG_DARK])  # flat, uniform - so it tiles perfectly with no band seam
scatter_stars(canvas4, 140, STAR_DIM)
scatter_stars(canvas4, 46, STAR_MID)
scatter_stars(canvas4, 10, STAR_BRIGHT)
scatter_stars(canvas4, 3, STAR_ACCENT)

UPSCALE4 = 8
up4 = nearest_upscale(canvas4, UPSCALE4)
write_png(os.path.join(OUT_DIR, "starfield_tileable.png"), up4, TW * UPSCALE4, TH * UPSCALE4)


# =========================================================================
# 3) Palette reference swatch strip
# =========================================================================
palette = [BG_DARK, BG_MID, BG_NEBULA1, BG_NEBULA2, STAR_DIM, STAR_MID, STAR_BRIGHT, STAR_ACCENT]
swatch = [[palette[x % len(palette)] for x in range(len(palette))] for _ in range(1)]
swatch_up = nearest_upscale(swatch, 64)
write_png(os.path.join(OUT_DIR, "starfield_palette_only.png"), swatch_up, len(palette) * 64, 64)

print("wrote:")
for f in sorted(os.listdir(OUT_DIR)):
    print(" -", os.path.join(OUT_DIR, f))
