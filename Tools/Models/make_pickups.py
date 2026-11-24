# Makes the shooter's pickups and the greybox's electric floor (the shooter's plan, Phase 3.1: health, pickups and the
# hazards; our own models, no pack's), low poly in the guns' look (flat colors, bevelled hard edges), in Blender without
# its window.
#
#   & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
#       --python Tools/Models/make_pickups.py -- [--preview <folder>]
#
# Writes Projects/Outpost/Assets/Models/OutpostPickups/:
#   Medkit_Small.glb    a small olive drab case (26 x 18 x 9 cm), a white cross on its lid, two latches (+25 health)
#   Medkit_Large.glb    a larger hard case (42 x 28 x 15 cm) with a carrying handle, a red cross on a white square (+50)
#   Vest.glb            a plate carrier lying flat (coyote tan): front plate, shoulder straps, three magazine pouches
#   AmmoBox.glb         a steel ammo can (olive, 28 x 15 x 18 cm): the lid's lip, the folding handle, the latch, a
#                       yellow band
#   ElectricFloor.glb   a 3 x 3 m steel floor plate with hazard stripes round its edge, a broken junction box at a
#                       corner and its cables across the plate (the HazardZone's look)
#
# Each stands on its origin (the floor), x to its right, y back, z up in Blender (the export turns it Y up). Everything
# is generated: edits by hand in Blender are lost on the next run.

import bpy
import bmesh
import math
import os
import sys
from mathutils import Matrix, Vector

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUTPUT = os.path.join(ROOT, "Projects", "Outpost", "Assets", "Models", "OutpostPickups")

# Base color, metallic, roughness
COLORS = {
    "Olive": ((0.115, 0.135, 0.07, 1.0), 0.0, 0.7),
    "OliveDark": ((0.06, 0.07, 0.04, 1.0), 0.0, 0.75),
    "White": ((0.8, 0.8, 0.78, 1.0), 0.0, 0.5),
    "Red": ((0.6, 0.03, 0.02, 1.0), 0.0, 0.5),
    "Black": ((0.025, 0.025, 0.025, 1.0), 0.0, 0.6),
    "Coyote": ((0.33, 0.25, 0.15, 1.0), 0.0, 0.85),
    "CoyoteDark": ((0.2, 0.15, 0.09, 1.0), 0.0, 0.9),
    "Steel": ((0.12, 0.13, 0.12, 1.0), 0.0, 0.55),
    "Metal": ((0.45, 0.45, 0.44, 1.0), 0.9, 0.4),
    "Yellow": ((0.75, 0.55, 0.03, 1.0), 0.0, 0.55),
    "Plate": ((0.18, 0.18, 0.19, 1.0), 0.85, 0.5),
    "Copper": ((0.75, 0.35, 0.18, 1.0), 1.0, 0.3),
    "Cable": ((0.03, 0.03, 0.035, 1.0), 0.0, 0.5),
    "Grey": ((0.35, 0.36, 0.36, 1.0), 0.2, 0.6),
}
MATERIALS = {}


def material(name):
    if name in MATERIALS:
        return MATERIALS[name]
    color, metallic, roughness = COLORS[name]
    m = bpy.data.materials.new(f"MI_Pickup_{name}")
    m.use_nodes = True
    bsdf = m.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = color
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    m.diffuse_color = color  # (the previews' Workbench shows this one)
    MATERIALS[name] = m
    return m


class Builder:
    """One mesh: parts added with a material name each (the mesh's slots made as they come)"""

    def __init__(self, name):
        self.name = name
        self.bm = bmesh.new()
        self.slots = []

    def slot(self, mat):
        if mat not in self.slots:
            self.slots.append(mat)
        return self.slots.index(mat)

    def cylinder(self, start, end, radius, mat, sides=8):
        """A cylinder from start to end (a cable, a handle's bar)"""
        start, end = Vector(start), Vector(end)
        axis = end - start
        length = axis.length
        result = bmesh.ops.create_cone(self.bm, cap_ends=True, segments=sides, radius1=radius, radius2=radius, depth=length)
        verts = result["verts"]
        rot = axis.to_track_quat("Z", "Y").to_matrix()
        bmesh.ops.rotate(self.bm, verts=verts, cent=(0, 0, 0), matrix=rot)
        bmesh.ops.translate(self.bm, vec=(start + end) * 0.5, verts=verts)
        index = self.slot(mat)
        for f in {f for v in verts for f in v.link_faces}:
            f.material_index = index
        return verts

    def finish(self):
        mesh = bpy.data.meshes.new(self.name)
        for mat in self.slots:
            mesh.materials.append(material(mat))
        bmesh.ops.recalc_face_normals(self.bm, faces=self.bm.faces)
        self.bm.to_mesh(mesh)
        self.bm.free()
        for poly in mesh.polygons:
            poly.use_smooth = False
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        return obj


def bevelled(b, center, size, mat, bevel):
    """A box with bevelled edges (its own vertices found by the builder's mark)"""
    b.bm.verts.ensure_lookup_table()
    mark = len(b.bm.verts)
    result = bmesh.ops.create_cube(b.bm, size=1.0)
    bmesh.ops.scale(b.bm, vec=Vector(size), verts=result["verts"])
    edges = list({e for v in result["verts"] for e in v.link_edges})
    bmesh.ops.bevel(b.bm, geom=edges, offset=bevel, segments=1, affect="EDGES", profile=0.5)
    b.bm.verts.ensure_lookup_table()
    verts = [v for v in b.bm.verts[mark:]]
    bmesh.ops.translate(b.bm, vec=Vector(center), verts=verts)
    index = b.slot(mat)
    for f in {f for v in verts for f in v.link_faces}:
        f.material_index = index
    return verts


def plain(b, center, size, mat, rotate=None):
    result = bmesh.ops.create_cube(b.bm, size=1.0)
    verts = result["verts"]
    bmesh.ops.scale(b.bm, vec=Vector(size), verts=verts)
    if rotate is not None:
        bmesh.ops.rotate(b.bm, verts=verts, cent=(0, 0, 0), matrix=rotate)
    bmesh.ops.translate(b.bm, vec=Vector(center), verts=verts)
    index = b.slot(mat)
    for f in {f for v in verts for f in v.link_faces}:
        f.material_index = index
    return verts


def cross(b, center_z, size, bar, mat, plate=None, plate_mat=None):
    """A cross lying on a lid at height center_z: two bars (size long, bar wide), over a square plate (plate wide)"""
    lift = 0.0015
    if plate:
        plain(b, (0, 0, center_z + lift * 0.5), (plate, plate, lift), plate_mat)
        center_z += lift
    plain(b, (0, 0, center_z + lift * 0.5), (size, bar, lift), mat)
    plain(b, (0, 0, center_z + lift * 0.5), (bar, size, lift), mat)


def medkit_small():
    b = Builder("Medkit_Small")
    w, d, h = 0.26, 0.18, 0.09
    bevelled(b, (0, 0, h * 0.5), (w, d, h), "Olive", 0.012)
    # The lid's seam: a dark band round the case a little below the top
    plain(b, (0, 0, h * 0.68), (w + 0.002, d + 0.002, 0.006), "OliveDark")
    # Two latches on the front (-y)
    for x in (-0.07, 0.07):
        plain(b, (x, -d * 0.5 - 0.004, h * 0.62), (0.03, 0.01, 0.03), "Black")
    cross(b, h, 0.11, 0.035, "White")
    return b.finish()


def medkit_large():
    b = Builder("Medkit_Large")
    w, d, h = 0.42, 0.28, 0.15
    bevelled(b, (0, 0, h * 0.5), (w, d, h), "Olive", 0.018)
    plain(b, (0, 0, h * 0.66), (w + 0.002, d + 0.002, 0.01), "OliveDark")
    for x in (-0.13, 0.13):
        plain(b, (x, -d * 0.5 - 0.005, h * 0.6), (0.045, 0.012, 0.045), "Black")
    # The carrying handle on the lid along x: two posts and a bar
    for x in (-0.08, 0.08):
        plain(b, (x, 0.07, h + 0.015), (0.02, 0.025, 0.03), "Black")
    plain(b, (0, 0.07, h + 0.033), (0.18, 0.025, 0.014), "Black")
    # The red cross on a white square (beside the handle)
    cross_z = h
    b2 = 0.0015
    plain(b, (0, -0.035, cross_z + b2 * 0.5), (0.15, 0.15, b2), "White")
    plain(b, (0, -0.035, cross_z + b2 * 1.5), (0.11, 0.035, b2), "Red")
    plain(b, (0, -0.035, cross_z + b2 * 1.5), (0.035, 0.11, b2), "Red")
    return b.finish()


def vest():
    b = Builder("Vest")
    # Lying flat on its back: the front plate's carrier (36 x 44 cm, 5 cm thick), its edges rounded a little
    w, l, t = 0.36, 0.44, 0.05
    bevelled(b, (0, 0, t * 0.5), (w, l, t), "Coyote", 0.012)
    # The shoulder straps going back over the top edge (+y), lying on the floor beyond it
    for x in (-0.11, 0.11):
        bevelled(b, (x, l * 0.5 + 0.06, 0.012), (0.07, 0.14, 0.024), "CoyoteDark", 0.006)
    # The cummerbund's ends at the sides, folded in
    for s in (-1, 1):
        bevelled(b, (s * (w * 0.5 + 0.05), -0.08, 0.015), (0.1, 0.18, 0.03), "CoyoteDark", 0.006)
    # Three magazine pouches across the lower front (-y), on the plate's top face
    for x in (-0.11, 0.0, 0.11):
        bevelled(b, (x, -0.1, t + 0.035), (0.085, 0.16, 0.07), "Coyote", 0.008)
        plain(b, (x, -0.1 + 0.055, t + 0.071), (0.07, 0.05, 0.004), "CoyoteDark")  # the flap
    # Webbing rows (MOLLE) above the pouches
    for k in range(3):
        plain(b, (0, 0.04 + 0.04 * k, t + 0.002), (w * 0.86, 0.012, 0.004), "CoyoteDark")
    # A name tape's velcro patch
    plain(b, (0, 0.16, t + 0.002), (0.12, 0.04, 0.004), "Olive")
    return b.finish()


def ammo_box():
    b = Builder("AmmoBox")
    l, w, h = 0.28, 0.15, 0.18
    bevelled(b, (0, 0, h * 0.45), (l, w, h * 0.9), "Olive", 0.006)
    # The lid, a little wider, with its lip
    bevelled(b, (0, 0, h * 0.95), (l + 0.008, w + 0.008, h * 0.12), "Olive", 0.004)
    # The yellow band round the body (the ammo's marking)
    plain(b, (0, 0, h * 0.5), (l + 0.002, w + 0.002, 0.018), "Yellow")
    # The folding handle lying on the lid along its length: a bar on two hinges
    for x in (-0.06, 0.06):
        plain(b, (x, 0, h + 0.012), (0.014, 0.022, 0.016), "Black")
    plain(b, (0, 0, h + 0.022), (0.13, 0.018, 0.01), "Black")
    # The latch on the +x end: a lever over the lid's edge
    plain(b, (l * 0.5 + 0.008, 0, h * 0.82), (0.012, 0.05, 0.08), "Steel")
    # The hinge on the -x end
    b.cylinder((-l * 0.5 - 0.006, -w * 0.4, h * 0.88), (-l * 0.5 - 0.006, w * 0.4, h * 0.88), 0.008, "Steel")
    return b.finish()


def electric_floor():
    b = Builder("ElectricFloor")
    size, t = 3.0, 0.03
    plain(b, (0, 0, t * 0.5), (size, size, t), "Plate")
    # Hazard stripes round the edge: a yellow band with black diagonal-looking blocks (alternating squares)
    band, lift = 0.16, 0.002
    stripes = 15
    step = size / stripes
    for side in range(4):
        rot = Matrix.Rotation(math.radians(90 * side), 3, "Z")
        for k in range(stripes):
            x = -size * 0.5 + step * (k + 0.5)
            color = "Yellow" if k % 2 == 0 else "Black"
            center = rot @ Vector((x, -size * 0.5 + band * 0.5, t + lift * 0.5))
            plain(b, center, (step, band, lift), color, rotate=rot)
    # Tread lines across the plate (raised strips)
    for k in range(-6, 7):
        plain(b, (k * 0.2, 0, t + 0.002), (0.03, size - 2 * band - 0.1, 0.004), "Grey")
    # The junction box at the corner (+x, +y), its door hanging open, and its cables across the plate
    bx, by = size * 0.5 - 0.35, size * 0.5 - 0.3
    bevelled(b, (bx, by, 0.3), (0.42, 0.3, 0.55), "Steel", 0.01)
    plain(b, (bx - 0.24, by - 0.15, 0.3), (0.02, 0.28, 0.5), "Steel",
          rotate=Matrix.Rotation(math.radians(-35), 3, "Z"))
    plain(b, (bx, by - 0.151, 0.45), (0.3, 0.004, 0.1), "Yellow")  # a warning label
    # Cables out of its bottom, snaking to the plate's middle, their ends bare (copper)
    paths = [
        [(bx - 0.1, by - 0.16, 0.05), (0.6, 0.4, 0.05), (0.1, 0.2, 0.05), (-0.25, -0.2, 0.05)],
        [(bx + 0.05, by - 0.16, 0.06), (0.8, 0.0, 0.06), (0.3, -0.45, 0.06), (-0.1, -0.7, 0.06)],
        [(bx - 0.15, by - 0.1, 0.045), (0.2, 0.9, 0.045), (-0.4, 0.6, 0.045), (-0.7, 0.1, 0.045)],
    ]
    for path in paths:
        for a, c in zip(path, path[1:]):
            b.cylinder(a, c, 0.018, "Cable", sides=6)
        end = Vector(path[-1])
        prev = Vector(path[-2])
        direction = (end - prev).normalized()
        b.cylinder(end, end + direction * 0.05, 0.008, "Copper", sides=6)
    return b.finish()


def export(obj, name):
    for o in bpy.context.scene.objects:
        o.select_set(o == obj)
    bpy.context.view_layer.objects.active = obj
    # UVs (one map; the materials are flat colors): the engine's importer wants them for its tangents
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project()
    bpy.ops.object.mode_set(mode="OBJECT")
    path = os.path.join(OUTPUT, f"{name}.glb")
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", export_yup=True, use_selection=True)
    dims = obj.dimensions
    print(f"{name}: {len(obj.data.vertices)} vertices, {len(obj.data.polygons)} faces, "
          f"{dims.x:.2f} x {dims.y:.2f} x {dims.z:.2f} m; wrote {path}")


def preview(objects, folder):
    os.makedirs(folder, exist_ok=True)
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.light = "STUDIO"
    scene.render.resolution_x = 600
    scene.render.resolution_y = 450
    camera = bpy.data.objects.new("C", bpy.data.cameras.new("C"))
    scene.collection.objects.link(camera)
    scene.camera = camera
    for obj in objects:
        for o in objects:
            o.hide_render = o != obj
        dims = obj.dimensions
        radius = max(dims.x, dims.y, dims.z)
        target = Vector((0, 0, dims.z * 0.4))
        location = target + Vector((0.9, -1.4, 1.1)).normalized() * radius * 2.2
        camera.location = location
        camera.rotation_euler = (target - location).to_track_quat("-Z", "Y").to_euler()
        camera.data.lens = 50
        scene.render.filepath = os.path.join(folder, f"{obj.name}.png")
        bpy.ops.render.render(write_still=True)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    folder = os.path.abspath(argv[argv.index("--preview") + 1]) if "--preview" in argv else None
    bpy.ops.wm.read_factory_settings(use_empty=True)
    os.makedirs(OUTPUT, exist_ok=True)
    objects = []
    for name, maker in (("Medkit_Small", medkit_small), ("Medkit_Large", medkit_large), ("Vest", vest),
                        ("AmmoBox", ammo_box), ("ElectricFloor", electric_floor)):
        obj = maker()
        export(obj, name)
        objects.append(obj)
    if folder:
        preview(objects, folder)


main()
