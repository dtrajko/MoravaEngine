# Renders the shooter's weapon silhouettes for the HUD's slots (the shooter's plan, Phase 3.2: the weapons' slots), from
# the weapons' own models, in Blender without its window:
#
#   & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
#       --python Tools/Textures/RenderWeaponIcons.py
#
# Writes Projects/Outpost/Assets/Textures/UI/Weapon_<Name>.png (Rifle, Sniper, Pistol, Knife): each weapon seen from its
# right side, the muzzle (the knife's point) to the right, flat white on a clear background (the HUD tints them), as
# large as fits the picture with a margin (each by its own size: the pistol as large as the rifle, as slots show them).
# Workbench's flat light: a silhouette, its holes (the trigger guard, the grip's gap) kept.
#
# The result is the same on every run.

import math
import os

import bpy
from mathutils import Matrix, Vector

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
PROJECT = os.path.join(ROOT, "Projects", "Outpost")
OUTPUT = os.path.join(PROJECT, "Assets", "Textures", "UI")

WIDTH, HEIGHT = 512, 160
MARGIN = 0.06  # of the picture, kept clear round the weapon

# The model, and its turn about Blender's Y (the view's axis) so the muzzle points to +X: the guns lie along +X already;
# the knife stands (its blade up the handle's line, leaning HANDLE_SLANT forward: Tools/Models/make_knife.py), turned so
# the blade points right, its edge down. The last number: how much of the usable width it may fill at most (the knife,
# long and thin, would fill all of it and look larger than the guns).
WEAPONS = [
    ("Rifle", "Assets/Models/QuaterniusGuns/AssaultRifle_2.fbx", 0.0, 1.0),
    ("Sniper", "Assets/Models/QuaterniusGuns/SniperRifle_3.fbx", 0.0, 1.0),
    ("Pistol", "Assets/Models/QuaterniusGuns/Pistol_1.fbx", 0.0, 1.0),
    ("Knife", "Assets/Models/OutpostWeapons/Knife.glb", 90.0 - 17.5, 0.6),
]


def clear():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)


def import_model(path):
    before = set(bpy.data.objects)
    if path.lower().endswith(".fbx"):
        bpy.ops.import_scene.fbx(filepath=path)
    else:
        bpy.ops.import_scene.gltf(filepath=path)
    return [o for o in bpy.data.objects if o not in before]


def bounds(objects):
    lo = Vector((math.inf,) * 3)
    hi = Vector((-math.inf,) * 3)
    for obj in objects:
        if obj.type != "MESH":
            continue
        # (the vertices: a turned bounding box's corners would stand out past a turned model)
        for vertex in obj.data.vertices:
            p = obj.matrix_world @ vertex.co
            lo = Vector(map(min, lo, p))
            hi = Vector(map(max, hi, p))
    return lo, hi


def setup_scene():
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.render.resolution_x = WIDTH
    scene.render.resolution_y = HEIGHT
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.view_settings.view_transform = "Standard"
    scene.display.render_aa = "16"
    shading = scene.display.shading
    shading.light = "FLAT"
    shading.color_type = "SINGLE"
    shading.single_color = (1.0, 1.0, 1.0)
    shading.show_object_outline = False
    shading.show_cavity = False
    shading.show_shadows = False
    shading.show_specular_highlight = False

    camera_data = bpy.data.cameras.new("IconCamera")
    camera_data.type = "ORTHO"
    camera = bpy.data.objects.new("IconCamera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    return scene, camera


def render(name, path, turn, fill, scene, camera):
    objects = import_model(os.path.join(PROJECT, path))
    root = bpy.data.objects.new("IconRoot", None)
    scene.collection.objects.link(root)
    for obj in objects:
        if obj.parent is None:
            obj.parent = root
    root.matrix_world = Matrix.Rotation(math.radians(turn), 4, "Y")
    bpy.context.view_layer.update()
    lo, hi = bounds(objects)
    size = hi - lo
    middle = (lo + hi) * 0.5

    # Looking along +Y from its -Y side (Blender's front view): +X right, +Z up
    camera.location = Vector((middle.x, lo.y - 1.0 - size.y, middle.z))
    camera.rotation_euler = (math.radians(90.0), 0.0, 0.0)
    camera.data.clip_end = 10.0 + size.y * 4.0
    usable = 1.0 - 2.0 * MARGIN
    aspect = WIDTH / HEIGHT
    # The ortho scale spans the picture's wider side (its width)
    camera.data.ortho_scale = max(size.x / (usable * fill), size.z * aspect / usable)

    out = os.path.join(OUTPUT, f"Weapon_{name}.png")
    scene.render.filepath = out
    bpy.ops.render.render(write_still=True)
    print(f"wrote {out} ({size.x:.3f} x {size.z:.3f})")

    for obj in objects + [root]:
        bpy.data.objects.remove(obj, do_unlink=True)


def main():
    clear()
    os.makedirs(OUTPUT, exist_ok=True)
    scene, camera = setup_scene()
    for name, path, turn, fill in WEAPONS:
        render(name, path, turn, fill, scene, camera)


main()
