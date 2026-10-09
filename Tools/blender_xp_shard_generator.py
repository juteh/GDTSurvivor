"""
Generates the low-poly experience shard (XP pickup) and exports it as FBX for Unreal.

A faceted crystal: one main shard (hexagonal prism with pointed ends), optionally with smaller shards that
grow out of its base at an angle. Flat shaded with a little seeded jitter, so the facets catch the
light like the stylized asteroids (blender_asteroid_generator.py).

Run headless (no Blender window needed):
    blender -b --python Tools/blender_xp_shard_generator.py

Produces (in Tools/model_output/):
    SM_XPShard.fbx - mesh + UCX_ convex collision, pivot in the center, about 25 cm tall

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
OUT_FILE = os.path.join(OUT_DIR, "SM_XPShard.fbx")

NAME = "SM_XPShard"
SEED = 7

# ---- main shard (meters) ----
SIDES = 6
RADIUS = 0.06
BAND_BOTTOM = -0.035     # the straight middle part of the crystal
BAND_TOP = 0.045
TOP_APEX = 0.15          # long point
BOTTOM_APEX = -0.10      # short point

# ---- side shards: (scale, rotation around Z in degrees, tilt outwards in degrees) ----
# They grow with their lower point from the axis of the main shard, so they stick out of its side.
# Empty = a single, simple crystal. Example entry: (0.58, 20.0, 52.0)
SIDE_SHARDS = []
SIDE_SHARD_OFFSET = -0.05  # height on the main shard's axis where their lower point sits

# Random offset per vertex, so the crystal does not look machine-made.
JITTER = 0.006


def clear_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def add_crystal(bm, transform):
    """Adds one crystal (prism band with a point on each end) to bm, transformed by the matrix."""
    def ring(z, twist):
        verts = []
        for index in range(SIDES):
            angle = 2 * math.pi * index / SIDES + twist
            verts.append(bm.verts.new(transform @ Vector((RADIUS * math.cos(angle), RADIUS * math.sin(angle), z))))
        return verts

    # The upper ring is twisted a little, so the facets are not perfectly regular.
    lower = ring(BAND_BOTTOM, 0.0)
    upper = ring(BAND_TOP, math.radians(8))
    top = bm.verts.new(transform @ Vector((0.0, 0.0, TOP_APEX)))
    bottom = bm.verts.new(transform @ Vector((0.0, 0.0, BOTTOM_APEX)))

    for index in range(SIDES):
        nxt = (index + 1) % SIDES
        bm.faces.new((lower[index], lower[nxt], upper[nxt], upper[index]))
        bm.faces.new((upper[index], upper[nxt], top))
        bm.faces.new((lower[nxt], lower[index], bottom))


def build_shard():
    random.seed(SEED)
    bm = bmesh.new()

    add_crystal(bm, Matrix.Identity(4))
    for scale, around, tilt in SIDE_SHARDS:
        rotation = Euler((0.0, math.radians(tilt), math.radians(around)), "ZYX").to_matrix().to_4x4()
        # Move the lower point to the origin, tilt, then place it on the main shard's axis.
        to_base = Matrix.Translation((0.0, 0.0, -BOTTOM_APEX * scale))
        transform = Matrix.Translation((0.0, 0.0, SIDE_SHARD_OFFSET)) @ rotation @ to_base @ Matrix.Scale(scale, 4)
        add_crystal(bm, transform)

    for vert in bm.verts:
        vert.co += Vector((random.uniform(-JITTER, JITTER) for _ in range(3)))

    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)

    mesh = bpy.data.meshes.new(NAME)
    bm.to_mesh(mesh)
    bm.free()

    obj = bpy.data.objects.new(NAME, mesh)
    bpy.context.collection.objects.link(obj)
    for polygon in mesh.polygons:
        polygon.use_smooth = False

    # Pivot in the center of the bounds, so the pickup spins around its middle.
    corners = [Vector(corner) for corner in obj.bound_box]
    center = sum(corners, Vector()) / 8
    mesh.transform(Matrix.Translation(-center))
    return obj


def build_collision(shard):
    """Convex hull of the shard as Unreal collision (UCX_ prefix)."""
    bm = bmesh.new()
    bm.from_mesh(shard.data)
    bmesh.ops.convex_hull(bm, input=bm.verts)
    mesh = bpy.data.meshes.new("UCX_" + NAME + "_00")
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new("UCX_" + NAME + "_00", mesh)
    bpy.context.collection.objects.link(obj)
    return obj


def export(objects):
    os.makedirs(OUT_DIR, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.ops.export_scene.fbx(
        filepath=OUT_FILE,
        use_selection=True,
        object_types={"MESH"},
        mesh_smooth_type="FACE",
        apply_unit_scale=True,
        axis_forward="-Y",
        axis_up="Z",
    )


def main():
    clear_scene()
    shard = build_shard()
    collision = build_collision(shard)
    export([shard, collision])
    dims = shard.dimensions
    print("Wrote %s (%d triangles, %.1f x %.1f x %.1f cm)" % (
        OUT_FILE, sum(len(p.vertices) - 2 for p in shard.data.polygons), dims.x * 100, dims.y * 100, dims.z * 100))


if __name__ == "__main__":
    main()
