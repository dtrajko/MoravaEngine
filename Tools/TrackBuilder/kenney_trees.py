# The racing game's trees and bushes (docs/games/Racing_Plan.html, Phase 8.3): copies of Kenney's Nature Kit models (CC0,
# https://kenney.nl/assets/nature-kit) with natural colors in place of the kit's teal. Plain Python, no Blender:
#
#   python Tools/TrackBuilder/kenney_trees.py "<Nature Kit>/Models/GLTF format" Projects/Racing/Assets/Models/TreesLowPoly
#
# Only the materials' base colors (and roughness) change; the meshes are the kit's. The models are about 1.2 m tall (the
# bushes 0.3 m): the circuit's Foliage species scale them (Scale [6, 8.5] for the trees, [3, 4.5] for the bushes).

import json
import os
import shutil
import struct
import sys

MODELS = ["tree_oak", "tree_detailed", "tree_fat", "tree_pineRoundA", "plant_bushLarge", "plant_bushDetailed"]

# Linear base colors by the kit's material names
COLORS = {
    "leafsGreen": [0.10, 0.28, 0.05, 1.0],
    "leafsDark": [0.05, 0.17, 0.05, 1.0],
    "grass": [0.12, 0.26, 0.05, 1.0],
    "woodBark": [0.22, 0.13, 0.07, 1.0],
    "woodBarkDark": [0.15, 0.09, 0.05, 1.0],
}


def recolor(source, target):
    data = open(source, "rb").read()
    json_length = struct.unpack_from("<I", data, 12)[0]
    gltf = json.loads(data[20:20 + json_length])
    rest = data[20 + json_length:]  # the binary chunk, as it is
    for material in gltf.get("materials", []):
        if material["name"] in COLORS:
            pbr = material.setdefault("pbrMetallicRoughness", {})
            pbr["baseColorFactor"] = COLORS[material["name"]]
            pbr["roughnessFactor"] = 0.85
            pbr["metallicFactor"] = 0.0
    text = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
    text += b" " * ((4 - len(text) % 4) % 4)
    with open(target, "wb") as f:
        f.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(text) + len(rest)))
        f.write(struct.pack("<II", len(text), 0x4E4F534A) + text + rest)


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: python kenney_trees.py <Nature Kit's 'Models/GLTF format' folder> <output folder>")
    source_dir, output_dir = sys.argv[1], sys.argv[2]
    os.makedirs(output_dir, exist_ok=True)
    for name in MODELS:
        target = os.path.join(output_dir, "Kenney_" + name + ".glb")
        recolor(os.path.join(source_dir, name + ".glb"), target)
        print("kenney_trees: wrote %s" % target)
    license_file = os.path.join(source_dir, "..", "..", "License.txt")
    if os.path.exists(license_file):
        shutil.copy(license_file, os.path.join(output_dir, "Kenney_NatureKit_License.txt"))


main()
