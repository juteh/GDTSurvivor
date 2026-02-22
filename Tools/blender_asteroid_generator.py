import bpy
import bmesh
import random
import math
from mathutils import Vector

# =========================================================
# CONFIG - Hier anpassen
# =========================================================

# Welcher Asteroid soll generiert werden?
# Aendere den Index um einen anderen Typ zu generieren.
SELECTED = 0

ASTEROIDS = [
    {
        "name": "Custom_Asteroid_Round_Small_1",
        "subdivisions": 2,
        "scale": (1.0, 0.9, 0.8),
        "surface_noise": 0.15,
        "micro_noise": 0.03,
    },
    {
        "name": "Custom_Asteroid_Round_Medium_1",
        "subdivisions": 3,
        "scale": (2.0, 1.8, 1.5),
        "surface_noise": 0.25,
        "micro_noise": 0.05,
    },
    {
        "name": "Custom_Asteroid_Round_Big_1",
        "subdivisions": 3,
        "scale": (3.0, 2.5, 2.0),
        "surface_noise": 0.3,
        "micro_noise": 0.06,
    },
    {
        "name": "Custom_Asteroid_Wall_Large_1",
        "subdivisions": 3,
        "scale": (4.0, 1.0, 3.0),
        "surface_noise": 0.2,
        "micro_noise": 0.04,
    },
]


# =========================================================
# CLEAN SCENE
# =========================================================

def delete_mesh_objects():
    if bpy.context.active_object and bpy.context.active_object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')

    bpy.ops.object.select_all(action='DESELECT')

    for obj in bpy.data.objects:
        if obj.type == 'MESH':
            obj.select_set(True)

    bpy.ops.object.delete()


# =========================================================
# ASTEROID GENERIERUNG
# =========================================================

def create_asteroid(
        name="Asteroid",
        subdivisions=2,
        scale=(1.0, 1.0, 1.0),
        surface_noise_strength=0.2,
        micro_noise_strength=0.05,
        bulge_count=4,
        bulge_strength=0.4,
        bulge_size=0.6,
        seed=None):

    if seed is not None:
        random.seed(seed)

    if bpy.context.active_object and bpy.context.active_object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')

    # Ico Sphere als Basis (radius=100 = 100cm = 1m in UE)
    bpy.ops.mesh.primitive_ico_sphere_add(
        subdivisions=subdivisions,
        radius=100.0,
        location=(0, 0, 0)
    )

    asteroid = bpy.context.active_object
    asteroid.name = name

    # Skalierung anwenden
    asteroid.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    # Vertex Displacement
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')

    me = asteroid.data
    bm = bmesh.from_edit_mesh(me)

    # Zufaellige Bulge-Zentren auf der Kugeloberflaeche
    bulges = []
    for _ in range(bulge_count):
        theta = random.uniform(0, math.pi * 2)
        phi = random.uniform(-math.pi / 2, math.pi / 2)
        direction = Vector((
            math.cos(phi) * math.cos(theta),
            math.cos(phi) * math.sin(theta),
            math.sin(phi)
        ))
        strength = random.uniform(bulge_strength * 0.5, bulge_strength)
        bulges.append((direction, strength))

    for v in bm.verts:
        # Richtung des Vertex normalisiert
        v_dir = v.co.normalized()

        # Bulge-Displacement: Naehe zu jedem Bulge-Zentrum
        bulge_offset = 0.0
        for bulge_dir, strength in bulges:
            # Dot Product = wie aehnlich die Richtungen sind (1=gleich, -1=gegenueber)
            dot = v_dir.dot(bulge_dir)
            # Smooth Falloff: nur Vertices in der Naehe des Bulge
            influence = max(0, (dot - (1.0 - bulge_size)) / bulge_size)
            influence = influence * influence * (3 - 2 * influence)  # Smoothstep
            bulge_offset += influence * strength

        # Normaler Surface Noise + Micro Noise
        offset = random.uniform(-surface_noise_strength, surface_noise_strength)
        micro = random.uniform(-micro_noise_strength, micro_noise_strength)

        # Alles zusammen: Bulge + Noise
        total = (offset + micro) + bulge_offset * v.co.length
        v.co += v.normal * total

    bmesh.update_edit_mesh(me)

    # Cleanup
    bpy.ops.mesh.remove_doubles(threshold=0.0001)
    bpy.ops.mesh.normals_make_consistent(inside=False)

    # UV Unwrap (wichtig fuer UE-Material)
    bpy.ops.uv.smart_project(angle_limit=66, island_margin=0.02)

    bpy.ops.object.mode_set(mode='OBJECT')

    # Flat Shading
    bpy.ops.object.shade_flat()

    # Triangulierung (UE-kompatibel)
    tri = asteroid.modifiers.new(name="Triangulate", type='TRIANGULATE')
    bpy.ops.object.modifier_apply(modifier=tri.name)

    # ── Robuste Normalen-Korrektur (verhindert schwarze Stellen) ──
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')

    # 1. Normalen konsistent nach aussen berechnen
    bpy.ops.mesh.normals_make_consistent(inside=False)

    # 2. Custom Split Normals clearen (falls vorhanden)
    bpy.ops.mesh.customdata_custom_splitnormals_clear()

    bpy.ops.object.mode_set(mode='OBJECT')

    # 3. Auto-Smooth fuer bessere Normalen-Interpolation
    # In Blender 4.x ist auto_smooth in mesh.use_auto_smooth
    if hasattr(asteroid.data, 'use_auto_smooth'):
        asteroid.data.use_auto_smooth = True
        asteroid.data.auto_smooth_angle = math.radians(30)

    # 4. Nochmal Normalen recalculate im Object-Mode
    bpy.context.view_layer.objects.active = asteroid
    asteroid.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode='OBJECT')

    # ── Convex Collision Mesh fuer UE (UCX_ Prefix) ──
    collision = asteroid.copy()
    collision.data = asteroid.data.copy()
    collision.name = f"UCX_{name}"
    bpy.context.collection.objects.link(collision)

    # Convex Hull aus dem Asteroid-Mesh erzeugen
    bpy.context.view_layer.objects.active = collision
    collision.select_set(True)
    asteroid.select_set(False)

    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.convex_hull()
    bpy.ops.object.mode_set(mode='OBJECT')

    # WICHTIG: Collision-Mesh NICHT verstecken, sonst wird es nicht exportiert!
    # Nur hide_render setzen (fuer Render), aber hide_set(False) damit es im Export dabei ist
    collision.hide_render = True
    # collision.hide_set(True)  # NICHT verstecken - sonst kein Export!

    # Zurueck zum Asteroid als aktives Objekt
    collision.select_set(False)
    asteroid.select_set(True)
    bpy.context.view_layer.objects.active = asteroid

    col_verts = len(collision.data.vertices)
    print(f"[OK] Collision '{collision.name}' erstellt ({col_verts} Verts)")

    vert_count = len(asteroid.data.vertices)
    tri_count = len(asteroid.data.polygons)
    print(f"[OK] '{name}' erstellt ({vert_count} Verts, {tri_count} Tris)")
    print(f"     Export: File > Export > FBX")
    print(f"     Wichtig: 'FBX_SCALE_ALL', Axis Forward '-Y', Axis Up 'Z'")

    return asteroid


# =========================================================
# MAIN
# =========================================================

def main():
    delete_mesh_objects()

    config = ASTEROIDS[SELECTED]

    create_asteroid(name="Smooth_Bumpy", subdivisions=3,
    scale=(2.0, 1.8, 1.5), surface_noise_strength=0.75,
    micro_noise_strength=0.45, bulge_count=3,
    bulge_strength=0.3, bulge_size=0.7, seed=102)


if __name__ == "__main__":
    main()
