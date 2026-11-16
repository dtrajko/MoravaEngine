# Tints the soldiers' hair texture (the eyebrows), in Blender without its window (the shooter's plan, Phase 0.2).
#
#   & "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
#       --python Tools/CharacterPoses/tint_hair.py -- [R G B]
#
# Reads  Projects/Outpost/Assets/Models/QuaterniusCharacters/T_Hair_1_BaseColor.png (the pack's, near white)
# Writes Projects/Outpost/Assets/Models/QuaterniusCharacters/T_Hair_1_DarkBrown.png
#
# The pack's hair texture is gray-white: each engine is meant to tint it with the material's base color factor. Ours
# uses a material's color only when it has no map, so the tint is baked into a copy of the texture instead, and the
# character's glTF names the copy. The texture's light and shade are kept: each pixel is the tint (sRGB 0-255, dark brown
# by default) times the pixel's brightness relative to the texture's average.

import os
import sys

import bpy
import numpy as np

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
FOLDER = os.path.join(ROOT, "Projects", "Outpost", "Assets", "Models", "QuaterniusCharacters")
SOURCE = os.path.join(FOLDER, "T_Hair_1_BaseColor.png")
OUTPUT = os.path.join(FOLDER, "T_Hair_1_DarkBrown.png")

TINT = (45, 30, 20)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    tint = np.array([float(v) for v in argv[:3]] if len(argv) >= 3 else TINT) / 255.0

    image = bpy.data.images.load(SOURCE)
    image.colorspace_settings.name = "Non-Color"  # the file's values as they are (sRGB), no conversion
    width, height = image.size
    pixels = np.empty(width * height * 4, dtype=np.float32)
    image.pixels.foreach_get(pixels)
    pixels = pixels.reshape(-1, 4)

    brightness = pixels[:, :3].mean(axis=1)
    relative = brightness / max(float(brightness.mean()), 1e-6)
    pixels[:, :3] = np.clip(relative[:, None] * tint[None, :], 0.0, 1.0)

    result = bpy.data.images.new("T_Hair_1_DarkBrown", width, height, alpha=True)
    result.colorspace_settings.name = "Non-Color"
    result.pixels.foreach_set(pixels.ravel())
    result.filepath_raw = OUTPUT
    result.file_format = "PNG"
    result.save()
    print(f"Wrote {OUTPUT} ({width} x {height}, tint {tuple(round(v * 255) for v in tint)})")


main()
