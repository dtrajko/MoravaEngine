# Makes the soldier's combat knife (the shooter's plan, Phase 2.4: the knife; no CC0 knife in the packs we use), a low
# poly model in the guns' look (flat colors, hard edges), in Blender without its window.
#
#   & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
#       --python Tools/Models/make_knife.py -- [--preview <folder>]
#
# Writes Projects/Outpost/Assets/Models/OutpostWeapons/Knife.glb
#
# Its frame is the guns' (Tools/CharacterPoses/make_rifle_poses.py): x forward, y to its left, z up, in meters. The knife
# is held as a pistol grip is (a hammer grip): its handle through the fist at the grip's slant (HANDLE_SLANT from up,
# leaning forward), the guard at the index finger, the blade going on up along the handle's line, its edge forward (+x),
# so pointing the knife forward turns the edge down. The handle's middle is the frame's origin.
#
# Everything is generated: edits by hand in Blender are lost on the next run.

import bpy
import bmesh
import math
import os
import sys
from mathutils import Matrix, Vector

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUTPUT = os.path.join(ROOT, "Projects", "Outpost", "Assets", "Models", "OutpostWeapons", "Knife.glb")

HANDLE_SLANT = 17.5     # degrees the handle leans forward from up (a pistol grip's: the fist closes at it)
HANDLE = (-0.060, 0.052)  # meters along the handle's line: the pommel's end and the guard's underside
HANDLE_HALF = (0.0135, 0.0115)  # the handle's half depth (front to back) and half width
GUARD = (0.052, 0.062)  # along the line: the guard's bottom and top
GUARD_HALF = (0.024, 0.010)
BLADE = (0.062, 0.232)  # along the line: the blade's base and its tip (17 cm)
BLADE_WIDTH = 0.032     # at the base, from the spine (-x) to the edge (+x)
BLADE_HALF_THICK = 0.0028
CLIP_FROM = 0.75        # of the blade's length the spine starts to drop to the tip (a clip point)

COLORS = {
    "Blade": ((0.62, 0.64, 0.66, 1.0), 0.9, 0.32),   # base color, metallic, roughness
    "Edge": ((0.82, 0.84, 0.86, 1.0), 1.0, 0.18),
    "Guard": ((0.16, 0.16, 0.17, 1.0), 0.8, 0.45),
    "Handle": ((0.07, 0.075, 0.07, 1.0), 0.0, 0.75),
}


def material(name):
    color, metallic, roughness = COLORS[name]
    m = bpy.data.materials.new(f"MI_Knife_{name}")
    m.use_nodes = True
    bsdf = m.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = color
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    m.diffuse_color = color  # (the previews' Workbench shows this one)
    return m


def box(bm, center_z0, center_z1, half_x0, half_y0, half_x1, half_y1, mat, sides=8, rounded=True):
    """A prism along z from z0 to z1, its cross-section an octagon (rounded) or a box, sizes lerped from end to end"""
    rings = []
    for z, hx, hy in ((center_z0, half_x0, half_y0), (center_z1, half_x1, half_y1)):
        ring = []
        for i in range(sides):
            a = 2.0 * math.pi * (i + 0.5) / sides
            if rounded:
                ring.append(bm.verts.new((hx * math.cos(a), hy * math.sin(a), z)))
            else:
                ring.append(bm.verts.new((hx * (1 if math.cos(a) > 0 else -1), hy * (1 if math.sin(a) > 0 else -1), z)))
        rings.append(ring)
    faces = []
    for i in range(sides):
        j = (i + 1) % sides
        faces.append(bm.faces.new((rings[0][i], rings[0][j], rings[1][j], rings[1][i])))
    faces.append(bm.faces.new(list(reversed(rings[0]))))
    faces.append(bm.faces.new(rings[1]))
    for f in faces:
        f.material_index = mat
    return faces


def blade(bm, mat_blade, mat_edge):
    """The blade: a flat spine (-x) with a clip point, the edge (+x) bevelled to a line, the tip where they meet"""
    z0, z1 = BLADE
    length = z1 - z0
    steps = 8
    spine, flat_l, flat_r, edge = [], [], [], []
    for k in range(steps + 1):
        t = k / steps
        z = z0 + length * t
        # The edge: straight, curving up into the tip over the last fifth; the spine straight to CLIP_FROM, then dropping
        edge_x = BLADE_WIDTH * 0.5 * (1.0 - max(0.0, (t - 0.8) / 0.2) ** 1.6)
        spine_x = -BLADE_WIDTH * 0.5 if t < CLIP_FROM else -BLADE_WIDTH * 0.5 + (BLADE_WIDTH * 0.5) * ((t - CLIP_FROM) / (1.0 - CLIP_FROM))
        if k == steps:
            edge_x = spine_x = 0.0
        thick = BLADE_HALF_THICK * (1.0 - 0.6 * t)
        bevel_x = spine_x + (edge_x - spine_x) * 0.55  # where the flats end and the edge's bevel starts
        spine.append(bm.verts.new((spine_x, 0.0, z)))
        flat_l.append(bm.verts.new((bevel_x, thick, z)))
        flat_r.append(bm.verts.new((bevel_x, -thick, z)))
        edge.append(bm.verts.new((edge_x, 0.0, z)))
    for k in range(steps):
        a, b = k, k + 1
        bm.faces.new((spine[a], spine[b], flat_l[b], flat_l[a])).material_index = mat_blade
        bm.faces.new((flat_r[a], flat_r[b], spine[b], spine[a])).material_index = mat_blade
        bm.faces.new((flat_l[a], flat_l[b], edge[b], edge[a])).material_index = mat_edge
        bm.faces.new((edge[a], edge[b], flat_r[b], flat_r[a])).material_index = mat_edge
    bm.faces.new((spine[0], flat_l[0], edge[0], flat_r[0])).material_index = mat_blade  # the base, in the guard


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    preview = os.path.abspath(argv[argv.index("--preview") + 1]) if "--preview" in argv else None
    bpy.ops.wm.read_factory_settings(use_empty=True)

    mesh = bpy.data.meshes.new("Knife")
    for name in COLORS:
        mesh.materials.append(material(name))
    index = {name: i for i, name in enumerate(COLORS)}
    bm = bmesh.new()
    # The handle (a grip a little thicker in its middle) and the pommel
    mid = (HANDLE[0] + HANDLE[1]) * 0.5
    box(bm, HANDLE[0], mid, HANDLE_HALF[0] * 0.92, HANDLE_HALF[1] * 0.92, HANDLE_HALF[0], HANDLE_HALF[1], index["Handle"])
    box(bm, mid, HANDLE[1], HANDLE_HALF[0], HANDLE_HALF[1], HANDLE_HALF[0] * 0.9, HANDLE_HALF[1] * 0.9, index["Handle"])
    box(bm, HANDLE[0] - 0.008, HANDLE[0], HANDLE_HALF[0] * 0.8, HANDLE_HALF[1] * 0.8, HANDLE_HALF[0], HANDLE_HALF[1],
        index["Guard"])
    box(bm, GUARD[0], GUARD[1], GUARD_HALF[0], GUARD_HALF[1], GUARD_HALF[0], GUARD_HALF[1], index["Guard"], sides=4,
        rounded=False)
    blade(bm, index["Blade"], index["Edge"])
    # Leaning forward at the grip's slant, about the handle's middle (the frame's origin)
    bmesh.ops.rotate(bm, verts=bm.verts, cent=(0.0, 0.0, 0.0), matrix=Matrix.Rotation(math.radians(HANDLE_SLANT), 3, "Y"))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    for poly in mesh.polygons:
        poly.use_smooth = False
    knife = bpy.data.objects.new("Knife", mesh)
    bpy.context.scene.collection.objects.link(knife)
    # UVs (one map; the materials are flat colors): the engine's importer wants them for its tangents
    bpy.context.view_layer.objects.active = knife
    knife.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project()
    bpy.ops.object.mode_set(mode="OBJECT")

    tip = Matrix.Rotation(math.radians(HANDLE_SLANT), 3, "Y") @ Vector((0.0, 0.0, BLADE[1]))
    print(f"Knife: {len(mesh.vertices)} vertices, {len(mesh.polygons)} faces; the tip at {tuple(round(v, 4) for v in tip)} m")
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    bpy.ops.export_scene.gltf(filepath=OUTPUT, export_format="GLB", export_yup=True, use_selection=False)
    print(f"Wrote {OUTPUT}")

    if preview:
        os.makedirs(preview, exist_ok=True)
        scene = bpy.context.scene
        scene.render.engine = "BLENDER_WORKBENCH"
        scene.display.shading.color_type = "MATERIAL"
        scene.render.resolution_x = 500
        scene.render.resolution_y = 700
        camera = bpy.data.objects.new("C", bpy.data.cameras.new("C"))
        camera.data.type = "ORTHO"
        camera.data.ortho_scale = 0.36
        scene.collection.objects.link(camera)
        scene.camera = camera
        for view, (location, rotation) in {"side": ((0.03, -1.0, 0.09), (math.radians(90), 0, 0)),
                                           "front": ((1.0, 0.0, 0.09), (math.radians(90), 0, math.radians(90)))}.items():
            camera.location = location
            camera.rotation_euler = rotation
            scene.render.filepath = os.path.join(preview, f"knife_{view}.png")
            bpy.ops.render.render(write_still=True)


main()
