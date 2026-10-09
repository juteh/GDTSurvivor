"""Builds a pixel font from Coalition: each glyph is rasterized on a coarse grid and rebuilt from squares."""
import sys
from fontTools.ttLib import TTFont
from fontTools.pens.boundsPen import BoundsPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.fontBuilder import FontBuilder
from PIL import Image, ImageDraw, ImageFont, ImageFilter

SRC = "C:/projects/gdtsurvivor/gdtsurvivor/Content/GDTSurvivor/UI/Fonts/Coalition_v2..ttf"
CAP = int(sys.argv[1]) if len(sys.argv) > 1 else 9     # cap height in pixels
OUT = sys.argv[2] if len(sys.argv) > 2 else f"CoalitionPixel{CAP}.ttf"
THRESH = float(sys.argv[3]) if len(sys.argv) > 3 else 0.5
SS = 24                    # supersampling per grid pixel
PX = 100                   # font units per grid pixel
SRC_CAP = 707              # cap height of the clean (lowercase) glyphs in source units
ASC, DESC = CAP + 1, max(2, round(CAP * 0.25))  # in pixels
UPM = (ASC + DESC) * PX

src = TTFont(SRC)
gs = src.getGlyphSet(); cmap = src.getBestCmap()
pil = ImageFont.truetype(SRC, round(1000 * CAP * SS / SRC_CAP))
unit = CAP * SS / SRC_CAP  # hi-res pixels per source unit

def source_char(c):
    """Uppercase letters use the clean lowercase outlines (the font is all-caps anyway)."""
    ch = chr(c)
    low = ch.lower()
    if ch != low and len(low) == 1 and ord(low) in cmap:
        return ord(low)
    return c

def rasterize(c):
    g = cmap[c]
    bp = BoundsPen(gs); gs[g].draw(bp)
    if bp.bounds is None:
        return None, round(gs[g].width / SRC_CAP * CAP)
    xmin, ymin, xmax, ymax = bp.bounds
    w = int((xmax - xmin) * unit) + 4 * SS
    top = (ASC + 2) * SS; h = top + (DESC + 2) * SS
    img = Image.new("L", (w, h), 0)
    ImageDraw.Draw(img).text((-xmin * unit, top), chr(c), font=pil, fill=255, anchor="ls")
    # close speckles of the grunge texture (digits, symbols)
    img = img.filter(ImageFilter.MaxFilter(5)).filter(ImageFilter.MinFilter(5))
    cols, rows = w // SS, h // SS
    small = img.resize((cols, rows), Image.BOX)
    grid = [[small.getpixel((x, y)) >= 255 * THRESH for x in range(cols)] for y in range(rows)]
    # rows: y index 0 = top; baseline at row index ASC+2
    return grid, None

def to_glyph(grid):
    pen = TTGlyphPen(None)
    if grid is None:
        return pen.glyph(), 0, 0
    rows = len(grid); base = ASC + 2
    used = [x for row in grid for x, v in enumerate(row) if v]
    if not used:
        return pen.glyph(), 0, 0
    x0, x1 = min(used), max(used)
    # merge horizontal runs, then stack identical runs vertically into rectangles
    rects = {}
    for y in range(rows):
        x = 0; row = grid[y]
        while x < len(row):
            if row[x]:
                s = x
                while x < len(row) and row[x]: x += 1
                rects.setdefault((s, x), []).append(y)
            else:
                x += 1
    for (s, e), ys in rects.items():
        ys.sort(); start = prev = ys[0]
        for y in ys[1:] + [None]:
            if y is not None and y == prev + 1:
                prev = y; continue
            l, r = (s - x0) * PX, (e - x0) * PX
            t, b = (base - start) * PX, (base - prev - 1) * PX
            pen.moveTo((l, b)); pen.lineTo((l, t)); pen.lineTo((r, t)); pen.lineTo((r, b)); pen.closePath()
            if y is not None: start = prev = y
    return pen.glyph(), x1 - x0 + 1, 0

names = [".notdef", "space"]; glyphs = {}; metrics = {}; outcmap = {}
glyphs[".notdef"] = TTGlyphPen(None).glyph(); metrics[".notdef"] = (CAP // 2 * PX, 0)
glyphs["space"] = TTGlyphPen(None).glyph(); metrics["space"] = (max(3, round(CAP * 0.45)) * PX, 0)
outcmap[32] = "space"; outcmap[0xA0] = "space"
cache = {}
for c in sorted(cmap):
    if c in (32, 0xA0): continue
    sc = source_char(c)
    if sc not in cache:
        grid, adv = rasterize(sc)
        glyph, width, _ = to_glyph(grid)
        if width == 0 and adv is None:
            continue
        cache[sc] = (glyph, ((width + 1) * PX) if width else adv * PX)
    name = cmap[c]
    if name in glyphs: outcmap[c] = name; continue
    names.append(name); glyphs[name], adv = cache[sc]; metrics[name] = (adv, 0)
    outcmap[c] = name

fb = FontBuilder(UPM, isTTF=True)
fb.setupGlyphOrder(names)
fb.setupCharacterMap(outcmap)
fb.setupGlyf(glyphs)
fb.setupHorizontalMetrics({n: (metrics[n][0], glyphs[n].xMin if hasattr(glyphs[n], "xMin") and glyphs[n].numberOfContours else 0) for n in names})
fb.setupHorizontalHeader(ascent=ASC * PX, descent=-DESC * PX)
fam = f"Coalition Pixel {CAP}"
fb.setupNameTable({"familyName": fam, "styleName": "Regular",
                   "psName": fam.replace(" ", "") + "-Regular",
                   "copyright": "Pixel derivative of " + (src["name"].getDebugName(0) or "Coalition")})
fb.setupOS2(sTypoAscender=ASC * PX, sTypoDescender=-DESC * PX, sTypoLineGap=0,
            usWinAscent=ASC * PX, usWinDescent=DESC * PX, sCapHeight=CAP * PX, sxHeight=CAP * PX)
fb.setupPost(); fb.setupDummyDSIG() if hasattr(fb, "setupDummyDSIG") else None
fb.save(OUT)
print("saved", OUT, "UPM", UPM, "glyphs", len(names), "| pixel-perfect at font size", (ASC + DESC) * 0.75, "pt (UE) * n")
