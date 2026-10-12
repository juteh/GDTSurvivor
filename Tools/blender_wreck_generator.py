"""
Generates a modular, low-poly freighter wreck and exports every piece as its own FBX for Unreal.

The freighter broke in two: a bow with the cargo hull and an armored nose, and a stern with the bridge
tower, side fins and three engines. Both have a jagged, recessed tear where they broke apart. Two
debris pieces (a bent hull plate and a broken girder) can be scattered around them. Flat shaded with
seeded jitter and a few dents, like the stylized asteroids (blender_asteroid_generator.py).

Designed for the top-down camera: the silhouette in X/Y tells what it is, the height stays low.
The player ship is about 1.2 m (120 uu) wide, so the whole freighter is about 10x the player.

Run headless (no Blender window needed):
    blender -b --python Tools/blender_wreck_generator.py

Produces (in Tools/model_output/):
    SM_Wreck_Bow.fbx      - front part, about 8 m long, tear at X = 0, nose towards +X
    SM_Wreck_Stern.fbx    - rear part, about 5.5 m long, tear at X = 0, engines towards -X
    SM_Wreck_Plate.fbx    - bent hull plate, about 1.6 m
    SM_Wreck_Girder.fbx   - broken girder, about 2.4 m
    preview/*.png         - renders of every piece and of an example layout

Every mesh comes with UCX_ convex collisions (one per big part) and these material slots:
    Hull      dark metal (most of the wreck)
    Accent    faded orange warning stripes
    Interior  almost black (inside of the tear, engine bells)
    Glow      dim engine glow (emissive in Unreal)
    Light     red warning lights (emissive in Unreal)

Import into Unreal with "Combine Meshes" on and the default scale; Blender meters become
Unreal centimeters.
"""
import math
import os
import random

import bmesh
import bpy
from mathutils import Euler, Matrix, Vector

OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "model_output")
PREVIEW_DIR = os.path.join(OUT_DIR, "preview")

SEED = 11

# Random offset per vertex, so the wreck does not look machine-made.
JITTER = 0.03

# Slot name -> viewport color (also the base color Unreal gets when importing materials).
MATERIALS = [
    ("Hull", (0.075, 0.085, 0.105, 1.0)),
    ("Accent", (0.55, 0.25, 0.05, 1.0)),
    ("Interior", (0.015, 0.015, 0.02, 1.0)),
    ("Glow", (1.0, 0.45, 0.12, 1.0)),
    ("Light", (1.0, 0.05, 0.03, 1.0)),
]
HULL, ACCENT, INTERIOR, GLOW, LIGHT = range(len(MATERIALS))


# =========================================================
# Building blocks. Every block is a closed solid, so the normals can be recalculated per island.
# =========================================================

class Piece:
    """One mesh under construction: geometry plus the point groups that become UCX_ collisions."""

    def __init__(self, name):
        self.name = name
        self.bm = bmesh.new()
        self.collision_groups = []

    def add_collision(self, verts):
        self.collision_groups.append([Vector(v.co) for v in verts])


def profile(half_width, half_height, chamfer):
    """Chamfered rectangle in the Y/Z plane, counter-clockwise when seen from +X."""
    c = min(half_width, half_height) * chamfer
    w, h = half_width, half_height
    corners = [(w, h - c), (w - c, h), (-w + c, h), (-w, h - c),
               (-w, -h + c), (-w + c, -h), (w - c, -h), (w, -h + c)]
    # Midpoints in between, so tears and dents have enough vertices to look jagged.
    points = []
    for i, (y, z) in enumerate(corners):
        ny, nz = corners[(i + 1) % len(corners)]
        points += [(y, z), ((y + ny) * 0.5, (z + nz) * 0.5)]
    return points


def loft(piece, stations, chamfer=0.35):
    """
    Hull section through the stations (x, half_width, half_height, z, band material).
    The band material colors the faces between this station and the next one.
    Returns the rings (lists of BMVerts), first ring at stations[0].
    """
    bm = piece.bm
    rings = []
    for x, hw, hh, z, _ in stations:
        rings.append([bm.verts.new((x, y, z + zz)) for y, zz in profile(hw, hh, chamfer)])

    for index in range(len(rings) - 1):
        a, b = rings[index], rings[index + 1]
        material = stations[index][4]
        for i in range(len(a)):
            j = (i + 1) % len(a)
            face = bm.faces.new((a[i], a[j], b[j], b[i]))
            face.material_index = material
    return rings


def cap(piece, ring, material=HULL):
    face = piece.bm.faces.new(ring)
    face.material_index = material
    return face


def tear(piece, ring, direction, depth, rng):
    """
    Turns an open hull end into a broken one: the ring gets jagged along X and is closed by a
    dark funnel that leads into the hull, so the tear looks hollow from above.
    direction = +1 if the hull continues towards +X from this ring, -1 otherwise.
    """
    bm = piece.bm
    for index, vert in enumerate(ring):
        # Zigzag: every other vertex sticks out, the ones in between are torn back into the hull.
        if index % 2 == 0:
            vert.co.x -= direction * rng.uniform(0.15, 0.6)
        else:
            vert.co.x += direction * rng.uniform(0.1, 0.45)
        vert.co.y *= rng.uniform(0.85, 1.05)
        vert.co.z *= rng.uniform(0.8, 1.05)

    center = sum((v.co for v in ring), Vector()) / len(ring)
    inner = []
    for vert in ring:
        # A smaller ring a bit inside, then one point deep in the hull.
        co = center + (vert.co - center) * 0.55
        co.x += direction * depth * 0.5
        inner.append(bm.verts.new(co))
    deep = bm.verts.new(center + Vector((direction * depth, 0.0, 0.0)))

    for i in range(len(ring)):
        j = (i + 1) % len(ring)
        face = bm.faces.new((ring[i], ring[j], inner[j], inner[i]))
        face.material_index = INTERIOR
        face = bm.faces.new((inner[i], inner[j], deep))
        face.material_index = INTERIOR


def box(piece, center, size, rotation=(0.0, 0.0, 0.0), material=HULL, taper=1.0):
    """Box with optional taper (top face scaled in X/Y). Rotation in degrees (X, Y, Z)."""
    bm = piece.bm
    sx, sy, sz = (s * 0.5 for s in size)
    matrix = Matrix.Translation(center) @ Euler([math.radians(r) for r in rotation], "XYZ").to_matrix().to_4x4()
    corners = []
    for z in (-sz, sz):
        scale = taper if z > 0 else 1.0
        for x, y in ((-sx, -sy), (sx, -sy), (sx, sy), (-sx, sy)):
            corners.append(bm.verts.new(matrix @ Vector((x * scale, y * scale, z))))
    b, t = corners[:4], corners[4:]
    faces = [(b[3], b[2], b[1], b[0]), (t[0], t[1], t[2], t[3])]
    for i in range(4):
        j = (i + 1) % 4
        faces.append((b[i], b[j], t[j], t[i]))
    for verts in faces:
        bm.faces.new(verts).material_index = material
    return corners


def wing(piece, root, tip, z, thickness, side, droop=0.0, material=HULL):
    """
    Swept fin: root = (x_front, x_back) at the hull, tip = (x_front, x_back, y) at the outer end.
    side = +1 for the left (+Y) fin, -1 for the right one. droop lowers the tip (bent by an impact).
    """
    bm = piece.bm
    half = thickness * 0.5
    root_y = 1.4 * side
    outline = [(root[0], root_y, z), (tip[0], tip[2] * side, z - droop),
               (tip[1], tip[2] * side, z - droop), (root[1], root_y, z)]
    if side < 0:
        outline.reverse()
    bottom = [bm.verts.new((x, y, zz - half)) for x, y, zz in outline]
    top = [bm.verts.new((x, y, zz + half)) for x, y, zz in outline]
    bm.faces.new(list(reversed(bottom))).material_index = material
    bm.faces.new(top).material_index = material
    for i in range(4):
        j = (i + 1) % 4
        bm.faces.new((bottom[i], bottom[j], top[j], top[i])).material_index = material
    return bottom + top


def engine(piece, origin, radius, length, sides=8):
    """Engine bell pointing towards -X: flared outer wall, a dark inner wall and a glowing core."""
    bm = piece.bm

    def ring(x, r):
        return [bm.verts.new(origin + Vector((x, r * math.cos(2 * math.pi * k / sides),
                                              r * math.sin(2 * math.pi * k / sides))))
                for k in range(sides)]

    start = ring(0.0, radius)                  # hidden inside the engine block
    outer = ring(-length, radius * 1.18)       # rim of the bell
    lip = ring(-length, radius * 0.92)
    inner = ring(-length * 0.35, radius * 0.9)

    def band(a, b, material):
        for i in range(sides):
            j = (i + 1) % sides
            bm.faces.new((a[i], a[j], b[j], b[i])).material_index = material

    band(start, outer, HULL)
    band(outer, lip, ACCENT)
    band(lip, inner, INTERIOR)
    bm.faces.new(start).material_index = HULL
    bm.faces.new(list(reversed(inner))).material_index = GLOW
    return start + outer


def shard(piece, base_a, base_b, outwards, length, rng):
    """Torn-off plating spike (closed tetrahedron) that sticks out of a tear."""
    bm = piece.bm
    mid = (base_a + base_b) * 0.5
    a = bm.verts.new(base_a)
    b = bm.verts.new(base_b)
    c = bm.verts.new(mid - outwards.normalized() * 0.12 + Vector((0, 0, rng.uniform(-0.05, 0.05))))
    tip = bm.verts.new(mid + outwards.normalized() * length + Vector((0, 0, rng.uniform(-0.15, 0.1))))
    for verts in ((a, b, tip), (b, c, tip), (c, a, tip), (a, c, b)):
        bm.faces.new(verts).material_index = HULL


def dent(piece, center, radius, depth):
    """Pushes the hull in around center (smooth falloff), like an impact."""
    for vert in piece.bm.verts:
        distance = (vert.co - center).length
        if distance < radius:
            falloff = 1.0 - distance / radius
            falloff = falloff * falloff * (3 - 2 * falloff)
            inwards = Vector((0.0, vert.co.y, vert.co.z))
            if inwards.length > 0.001:
                vert.co -= inwards.normalized() * depth * falloff


def jitter(piece, rng, amount=JITTER):
    for vert in piece.bm.verts:
        vert.co += Vector((rng.uniform(-amount, amount) for _ in range(3)))


# =========================================================
# Pieces
# =========================================================

def build_bow(rng):
    piece = Piece("SM_Wreck_Bow")

    # Cargo hull from the tear (X = 0) to the armored nose (+X).
    #            x    half-w half-h  z    band material (towards the next station)
    stations = [(0.0, 1.45, 0.55, 0.0, HULL),
                (1.2, 1.5, 0.6, 0.0, HULL),
                (3.6, 1.5, 0.6, 0.0, HULL),
                (4.0, 1.5, 0.6, 0.0, ACCENT),
                (4.35, 1.5, 0.6, 0.0, HULL),
                (5.8, 1.4, 0.58, 0.0, HULL),
                (6.2, 1.38, 0.57, 0.0, ACCENT),
                (6.5, 1.3, 0.55, 0.0, HULL),
                (7.4, 0.95, 0.45, -0.03, HULL),
                (8.0, 0.45, 0.28, -0.08, HULL)]
    rings = loft(piece, stations)
    cap(piece, rings[-1])
    piece.add_collision([v for ring in rings for v in ring])

    # Cargo hatches on top (slightly raised), the last one blown open.
    for index, x in enumerate((1.0, 2.2, 5.0)):
        box(piece, Vector((x + 0.5, 0.0, 0.62)), (0.95, 2.1, 0.08), material=HULL)
    box(piece, Vector((3.0, 0.55, 0.75)), (0.9, 0.95, 0.06), rotation=(-25.0, 8.0, 12.0))
    box(piece, Vector((3.2, -0.5, 0.6)), (0.6, 0.8, 0.06), material=INTERIOR)

    # Spine and a broken antenna on the nose, one warning light that is still on.
    box(piece, Vector((6.3, 0.0, 0.66)), (1.4, 0.35, 0.16), taper=0.7)
    box(piece, Vector((6.7, 0.0, 0.95)), (0.06, 0.06, 0.6), rotation=(0.0, 25.0, 0.0))
    box(piece, Vector((7.55, 0.0, 0.47)), (0.12, 0.12, 0.08), material=LIGHT)

    # Exposed frame ribs and torn plating at the break.
    for y, z, length, bend in ((0.9, 0.2, 0.9, 12.0), (-0.7, 0.3, 0.6, -18.0), (0.1, -0.3, 0.75, 6.0)):
        box(piece, Vector((-length * 0.4, y, z)), (length, 0.1, 0.1), rotation=(0.0, rng.uniform(-10, 10), bend))
    tear_ring = rings[0]
    for i in (0, 3, 5):
        a, b = tear_ring[i].co.copy(), tear_ring[(i + 1) % len(tear_ring)].co.copy()
        outwards = Vector((-1.0, (a.y + b.y) * 0.3, (a.z + b.z) * 0.3))
        shard(piece, a, b, outwards, rng.uniform(0.4, 0.8), rng)
    tear(piece, tear_ring, direction=1, depth=1.6, rng=rng)

    dent(piece, Vector((2.6, 1.5, 0.3)), 0.9, 0.25)
    dent(piece, Vector((5.2, -1.4, 0.5)), 0.7, 0.2)
    return piece


def build_stern(rng):
    piece = Piece("SM_Wreck_Stern")

    # Main hull from the tear (X = 0) back to the engine block (-X).
    stations = [(0.0, 1.5, 0.6, 0.0, HULL),
                (-1.4, 1.55, 0.62, 0.0, ACCENT),
                (-1.75, 1.55, 0.62, 0.0, HULL),
                (-3.6, 1.6, 0.65, 0.0, HULL),
                (-4.2, 1.7, 0.7, 0.0, HULL)]
    rings = loft(piece, stations)
    hull_points = [v for ring in rings for v in ring]

    # Engine block: wider than the hull, three engines in a row.
    block = box(piece, Vector((-4.6, 0.0, 0.0)), (0.9, 3.6, 1.3))
    cap(piece, rings[-1])
    piece.add_collision(hull_points + block)
    for y in (-1.1, 0.0, 1.1):
        engine(piece, Vector((-5.05, y, 0.0)), radius=0.42, length=0.7)

    # Side fins, one of them bent down by the impact.
    fin_l = wing(piece, root=(-1.9, -4.0), tip=(-3.3, -4.3, 3.2), z=-0.05, thickness=0.14, side=1)
    # The right fin's tip is torn off and bent down.
    fin_r = wing(piece, root=(-2.1, -4.0), tip=(-3.1, -3.9, 2.5), z=-0.1, thickness=0.14, side=-1, droop=0.45)
    piece.add_collision(fin_l)
    piece.add_collision(fin_r)
    box(piece, Vector((-3.85, 3.0, 0.05)), (0.5, 0.35, 0.1), material=ACCENT)
    box(piece, Vector((-3.85, 3.1, -0.04)), (0.14, 0.14, 0.12), material=LIGHT)

    # Bridge tower with a dark window band, and two warning lights.
    box(piece, Vector((-2.7, 0.0, 0.85)), (1.6, 1.5, 0.45), taper=0.85)
    box(piece, Vector((-2.35, 0.0, 1.2)), (0.9, 1.0, 0.3), taper=0.8)
    box(piece, Vector((-1.92, 0.0, 1.2)), (0.06, 0.8, 0.12), material=GLOW)
    box(piece, Vector((-2.4, 0.0, 1.45)), (0.08, 0.08, 0.35))
    box(piece, Vector((-4.3, 1.75, 0.55)), (0.12, 0.12, 0.08), material=LIGHT)
    box(piece, Vector((-0.9, -1.2, 0.65)), (0.1, 0.1, 0.08), material=LIGHT)

    # Exposed ribs and torn plating at the break (the stern continues towards -X).
    for y, z, length, bend in ((1.0, 0.25, 0.8, -15.0), (-0.9, 0.1, 1.0, 10.0), (0.2, 0.35, 0.5, 4.0)):
        box(piece, Vector((length * 0.4, y, z)), (length, 0.1, 0.1), rotation=(0.0, rng.uniform(-10, 10), bend))
    tear_ring = rings[0]
    for i in (1, 4, 6):
        a, b = tear_ring[i].co.copy(), tear_ring[(i + 1) % len(tear_ring)].co.copy()
        outwards = Vector((1.0, (a.y + b.y) * 0.3, (a.z + b.z) * 0.3))
        shard(piece, a, b, outwards, rng.uniform(0.4, 0.9), rng)
    tear(piece, tear_ring, direction=-1, depth=1.4, rng=rng)

    dent(piece, Vector((-1.0, 1.55, 0.2)), 0.8, 0.22)
    return piece


def build_plate(rng):
    """A hull plate torn off and bent in the middle, with half a warning stripe."""
    piece = Piece("SM_Wreck_Plate")
    a = box(piece, Vector((-0.4, 0.0, 0.0)), (0.8, 1.1, 0.06), rotation=(0.0, -14.0, 0.0))
    b = box(piece, Vector((0.38, 0.05, 0.12)), (0.8, 1.0, 0.06), rotation=(6.0, 18.0, 9.0))
    c = box(piece, Vector((-0.4, 0.42, 0.04)), (0.75, 0.2, 0.03), rotation=(0.0, -14.0, 0.0), material=ACCENT)
    piece.add_collision(a + b + c)
    return piece


def build_girder(rng):
    """Broken truss girder: two rails with cross struts, one end bent away."""
    piece = Piece("SM_Wreck_Girder")
    points = []
    points += box(piece, Vector((0.0, 0.25, 0.0)), (2.0, 0.09, 0.09))
    points += box(piece, Vector((0.0, -0.25, 0.0)), (2.0, 0.09, 0.09))
    points += box(piece, Vector((1.25, 0.32, 0.05)), (0.7, 0.09, 0.09), rotation=(0.0, 0.0, 25.0))
    for x in (-0.9, -0.3, 0.3, 0.9):
        points += box(piece, Vector((x, 0.0, 0.0)), (0.07, 0.5, 0.07))
    for x in (-0.6, 0.6):
        points += box(piece, Vector((x, 0.0, 0.0)), (0.07, 0.75, 0.07), rotation=(0.0, 0.0, 50.0))
    piece.add_collision(points)
    return piece


# =========================================================
# Mesh objects, collision, export
# =========================================================

def make_materials():
    materials = []
    for name, color in MATERIALS:
        material = bpy.data.materials.new(name)
        material.diffuse_color = color
        material.use_nodes = True
        bsdf = material.node_tree.nodes.get("Principled BSDF")
        if bsdf:
            bsdf.inputs["Base Color"].default_value = color
            bsdf.inputs["Roughness"].default_value = 0.7
        materials.append(material)
    return materials


def finish(piece, materials, rng):
    jitter(piece, rng)
    bmesh.ops.recalc_face_normals(piece.bm, faces=piece.bm.faces)
    bmesh.ops.triangulate(piece.bm, faces=piece.bm.faces)

    mesh = bpy.data.meshes.new(piece.name)
    piece.bm.to_mesh(mesh)
    piece.bm.free()
    for material in materials:
        mesh.materials.append(material)
    for polygon in mesh.polygons:
        polygon.use_smooth = False

    obj = bpy.data.objects.new(piece.name, mesh)
    bpy.context.collection.objects.link(obj)

    collisions = []
    for index, points in enumerate(piece.collision_groups):
        bm = bmesh.new()
        for co in points:
            bm.verts.new(co)
        bmesh.ops.convex_hull(bm, input=bm.verts)
        name = "UCX_%s_%02d" % (piece.name, index)
        collision_mesh = bpy.data.meshes.new(name)
        bm.to_mesh(collision_mesh)
        bm.free()
        collision = bpy.data.objects.new(name, collision_mesh)
        bpy.context.collection.objects.link(collision)
        collision.hide_render = True
        collisions.append(collision)
    return obj, collisions


def export(obj, collisions):
    path = os.path.join(OUT_DIR, obj.name + ".fbx")
    bpy.ops.object.select_all(action="DESELECT")
    for selected in [obj] + collisions:
        selected.select_set(True)
    bpy.ops.export_scene.fbx(
        filepath=path,
        use_selection=True,
        object_types={"MESH"},
        mesh_smooth_type="FACE",
        apply_unit_scale=True,
        axis_forward="-Y",
        axis_up="Z",
    )
    tris = len(obj.data.polygons)
    dims = obj.dimensions
    print("Wrote %s (%d triangles, %d collision hulls, %.0f x %.0f x %.0f cm)" % (
        path, tris, len(collisions), dims.x * 100, dims.y * 100, dims.z * 100))


# =========================================================
# Previews (Workbench, material colors, dark background)
# =========================================================

def setup_render():
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.show_cavity = True
    scene.display.shading.show_object_outline = False
    scene.render.resolution_x = 900
    scene.render.resolution_y = 700
    scene.render.film_transparent = False
    world = bpy.data.worlds.new("Space")
    world.color = (0.01, 0.01, 0.02)
    scene.world = world
    scene.display.shading.background_type = "WORLD"

    camera = bpy.data.objects.new("Camera", bpy.data.cameras.new("Camera"))
    scene.collection.objects.link(camera)
    scene.camera = camera
    return camera


def render(camera, objects, filename, top_down):
    scene = bpy.context.scene
    for obj in scene.objects:
        if obj.type == "MESH":
            obj.hide_render = obj not in objects

    corners = [obj.matrix_world @ Vector(c) for obj in objects for c in obj.bound_box]
    low = Vector((min(c.x for c in corners), min(c.y for c in corners), min(c.z for c in corners)))
    high = Vector((max(c.x for c in corners), max(c.y for c in corners), max(c.z for c in corners)))
    center = (low + high) * 0.5
    size = max(high.x - low.x, high.y - low.y) * 1.15

    camera.data.type = "ORTHO"
    camera.data.ortho_scale = size
    if top_down:
        camera.location = center + Vector((0.0, 0.0, 30.0))
        camera.rotation_euler = (0.0, 0.0, 0.0)
    else:
        direction = Vector((-0.55, -0.75, 0.6)).normalized()
        camera.location = center + direction * 30.0
        camera.rotation_euler = direction.to_track_quat("Z", "Y").to_euler()

    os.makedirs(PREVIEW_DIR, exist_ok=True)
    scene.render.filepath = os.path.join(PREVIEW_DIR, filename)
    bpy.ops.render.render(write_still=True)


def main():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    os.makedirs(OUT_DIR, exist_ok=True)
    rng = random.Random(SEED)
    materials = make_materials()

    pieces = []
    for build in (build_bow, build_stern, build_plate, build_girder):
        obj, collisions = finish(build(rng), materials, rng)
        export(obj, collisions)
        pieces.append(obj)

    camera = setup_render()
    for obj in pieces:
        render(camera, [obj], obj.name + "_top.png", top_down=True)
        render(camera, [obj], obj.name + "_angle.png", top_down=False)

    # Example layout like in a tile: both halves drifted apart, debris in between.
    bow, stern, plate, girder = pieces
    bow.location = (1.6, 0.9, 0.0)
    bow.rotation_euler = (0.0, 0.0, math.radians(14.0))
    stern.location = (-1.4, -0.6, 0.0)
    stern.rotation_euler = (0.0, 0.0, math.radians(-8.0))
    plate.location = (0.3, -2.4, 0.0)
    plate.rotation_euler = (0.0, 0.0, math.radians(40.0))
    girder.location = (-0.2, 2.6, 0.0)
    girder.rotation_euler = (0.0, 0.0, math.radians(-30.0))
    bpy.context.view_layer.update()
    render(camera, pieces, "Layout_top.png", top_down=True)
    render(camera, pieces, "Layout_angle.png", top_down=False)


if __name__ == "__main__":
    main()
