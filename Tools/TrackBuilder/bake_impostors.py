# The impostors of the racing game's trees and bushes (docs/games/Racing_Plan.html, Phase 8.4): for each model, what it looks
# like from 16 directions around it (level, from the side), baked into an atlas of its colors (with alpha) and one of its
# normals (4 x 4 views), and a small glTF model next to it: one card, with the first view's part of the atlases. The engine's
# foliage draws a copy far from the camera as that card turned to face the camera, showing the view nearest to the camera's
# direction: its shaders move the card's texture coordinates to that view's part (FoliageSpecies::Impostor, ImpostorDistance;
# Include/MeshInstance.glslh). The 4 x 4 layout is the engine's: it reads no settings from the file. Plain Python with numpy (no Blender, no imaging library):
#
#   python Tools/TrackBuilder/bake_impostors.py <model.glb> [<model.glb> ...]
#
# writes <model>_Impostor.glb, <model>_Impostor_Albedo.png and <model>_Impostor_Normal.png beside each model.
#
# The models' colors are their materials' base color factors times their vertex colors (Kenney's Nature Kit has no textures);
# a base color texture isn't read (a warning says so). The views are rendered orthographically, 4 x 4 samples a pixel; the
# alpha is the share of a pixel's samples the model covers, the colors and normals are spread into the empty pixels around
# the model (no dark fringe in the smaller mip levels).
#
# The card is a square in its own x-y plane (z towards the viewer), centered on the model's vertical axis (its origin's: the
# copy turns about it), from the model's lowest point up. View k (column k % 4, row k // 4 of the atlases) is seen from angle
# k * 360 / 16 about the vertical axis: 0 from +z, 90 from +x. The normal atlas is in the card's tangent space (x right, y up, z towards the viewer), as normal
# maps are.

import json
import math
import os
import struct
import sys
import zlib

import numpy as np

VIEWS = 16         # the engine's layout: 4 x 4
COLUMNS = 4
CELL = 256          # pixels of a view in the atlas
SAMPLES = 4         # per pixel side
CUTOFF = 0.5        # the material's alpha cutoff
DILATE = 16         # pixels the colors spread into the empty space around the model

COMPONENTS = {5120: np.int8, 5121: np.uint8, 5122: np.int16, 5123: np.uint16, 5125: np.uint32, 5126: np.float32}
COUNTS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}


# ---- Reading a glb: its triangles in the model's space, with a normal and a color per corner

def read_glb(path):
    data = open(path, "rb").read()
    magic, _, _ = struct.unpack_from("<III", data, 0)
    if magic != 0x46546C67:
        raise SystemExit("bake_impostors: %s isn't a glb file" % path)
    offset, gltf, binary = 12, None, b""
    while offset < len(data):
        length, kind = struct.unpack_from("<II", data, offset)
        chunk = data[offset + 8:offset + 8 + length]
        if kind == 0x4E4F534A:
            gltf = json.loads(chunk)
        elif kind == 0x004E4942:
            binary = chunk
        offset += 8 + length
    return gltf, binary


def accessor(gltf, binary, index):
    a = gltf["accessors"][index]
    view = gltf["bufferViews"][a["bufferView"]]
    dtype = np.dtype(COMPONENTS[a["componentType"]])
    count, width = a["count"], COUNTS[a["type"]]
    start = view.get("byteOffset", 0) + a.get("byteOffset", 0)
    stride = view.get("byteStride", dtype.itemsize * width)
    raw = np.frombuffer(binary, dtype=np.uint8, count=stride * (count - 1) + dtype.itemsize * width, offset=start)
    rows = np.lib.stride_tricks.as_strided(raw, shape=(count, dtype.itemsize * width), strides=(stride, 1))
    values = np.ascontiguousarray(rows).view(dtype).reshape(count, width).astype(np.float64)
    if a.get("normalized"):
        values /= float(np.iinfo(dtype).max)
    return values


def node_matrix(node):
    if "matrix" in node:
        return np.array(node["matrix"], dtype=np.float64).reshape(4, 4).T
    t = np.array(node.get("translation", [0, 0, 0]), dtype=np.float64)
    x, y, z, w = node.get("rotation", [0, 0, 0, 1])
    s = np.array(node.get("scale", [1, 1, 1]), dtype=np.float64)
    r = np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                  [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                  [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])
    m = np.eye(4)
    m[:3, :3] = r * s
    m[:3, 3] = t
    return m


def triangles(path):
    gltf, binary = read_glb(path)
    positions, normals, colors = [], [], []
    warned = set()

    def add_mesh(mesh, matrix):
        normal_matrix = np.linalg.inv(matrix[:3, :3]).T
        for primitive in mesh["primitives"]:
            if primitive.get("mode", 4) != 4:
                continue
            attributes = primitive["attributes"]
            p = accessor(gltf, binary, attributes["POSITION"])
            p = (matrix[:3, :3] @ p.T).T + matrix[:3, 3]
            n = accessor(gltf, binary, attributes["NORMAL"]) if "NORMAL" in attributes else None
            c = np.ones((len(p), 3))
            if "COLOR_0" in attributes:
                c = accessor(gltf, binary, attributes["COLOR_0"])[:, :3]
            material = gltf.get("materials", [{}])[primitive["material"]] if "material" in primitive else {}
            pbr = material.get("pbrMetallicRoughness", {})
            c = c * np.array(pbr.get("baseColorFactor", [1, 1, 1, 1])[:3])
            if "baseColorTexture" in pbr and material.get("name") not in warned:
                warned.add(material.get("name"))
                print("bake_impostors: %s: the base color texture of '%s' isn't read (its factor is)" % (os.path.basename(path), material.get("name")))
            index = accessor(gltf, binary, primitive["indices"]).astype(np.int64).ravel() if "indices" in primitive else np.arange(len(p))
            tri = index.reshape(-1, 3)
            corners = p[tri]
            if n is None:
                face = np.cross(corners[:, 1] - corners[:, 0], corners[:, 2] - corners[:, 0])
                face /= np.maximum(np.linalg.norm(face, axis=1, keepdims=True), 1e-12)
                tn = np.repeat(face[:, None, :], 3, axis=1)
            else:
                tn = (normal_matrix @ n.T).T[tri]
            positions.append(corners)
            normals.append(tn)
            colors.append(c[tri])

    def visit(index, parent):
        node = gltf["nodes"][index]
        matrix = parent @ node_matrix(node)
        if "mesh" in node:
            add_mesh(gltf["meshes"][node["mesh"]], matrix)
        for child in node.get("children", []):
            visit(child, matrix)

    scene = gltf.get("scenes", [{"nodes": list(range(len(gltf["nodes"])))}])[gltf.get("scene", 0)]
    for root in scene["nodes"]:
        visit(root, np.eye(4))
    return np.concatenate(positions), np.concatenate(normals), np.concatenate(colors)


# ---- Rendering a view: orthographic, a z-buffer, samples per pixel

def render_view(positions, normals, colors, angle, square, bottom, size):
    right = np.array([math.cos(angle), 0.0, -math.sin(angle)])
    toward = np.array([math.sin(angle), 0.0, math.cos(angle)])  # from the model to the viewer
    up = np.array([0.0, 1.0, 0.0])
    scale = size / square
    # Screen: x to the right from the card's left edge, y down from its top; depth: larger is nearer
    sx = (positions @ right + square * 0.5) * scale
    sy = (bottom + square - positions[..., 1]) * scale
    sz = positions @ toward
    depth = np.full((size, size), -np.inf)
    albedo = np.zeros((size, size, 3))
    normal = np.zeros((size, size, 3))
    covered = np.zeros((size, size), dtype=bool)
    view_normals = np.stack([normals @ right, normals @ up, normals @ toward], axis=-1)
    for t in range(len(sx)):
        x0, y0 = sx[t], sy[t]
        x_min, x_max = max(int(math.floor(x0.min())), 0), min(int(math.ceil(x0.max())), size - 1)
        y_min, y_max = max(int(math.floor(y0.min())), 0), min(int(math.ceil(y0.max())), size - 1)
        if x_min > x_max or y_min > y_max:
            continue
        (ax, bx, cx), (ay, by, cy) = x0, y0
        area = (bx - ax) * (cy - ay) - (cx - ax) * (by - ay)
        if abs(area) < 1e-12:
            continue
        px, py = np.meshgrid(np.arange(x_min, x_max + 1) + 0.5, np.arange(y_min, y_max + 1) + 0.5)
        w0 = ((bx - px) * (cy - py) - (cx - px) * (by - py)) / area
        w1 = ((cx - px) * (ay - py) - (ax - px) * (cy - py)) / area
        w2 = 1.0 - w0 - w1
        inside = (w0 >= 0) & (w1 >= 0) & (w2 >= 0)
        if not inside.any():
            continue
        z = w0 * sz[t, 0] + w1 * sz[t, 1] + w2 * sz[t, 2]
        region = depth[y_min:y_max + 1, x_min:x_max + 1]
        nearer = inside & (z > region)
        if not nearer.any():
            continue
        region[nearer] = z[nearer]
        weights = np.stack([w0, w1, w2], axis=-1)[nearer]
        n = weights @ view_normals[t]
        n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)
        n[n[:, 2] < 0] *= np.array([1.0, 1.0, -1.0])  # a back face seen: its normal towards the viewer
        albedo[y_min:y_max + 1, x_min:x_max + 1][nearer] = weights @ colors[t]
        normal[y_min:y_max + 1, x_min:x_max + 1][nearer] = n
        covered[y_min:y_max + 1, x_min:x_max + 1][nearer] = True
    return albedo, normal, covered


def downsample(albedo, normal, covered, samples):
    size = covered.shape[0] // samples
    shape = (size, samples, size, samples)
    weight = covered.astype(np.float64)
    alpha = weight.reshape(shape).mean(axis=(1, 3))
    total = np.maximum(weight.reshape(shape).sum(axis=(1, 3)), 1e-12)[..., None]
    color = (albedo * weight[..., None]).reshape(size, samples, size, samples, 3).sum(axis=(1, 3)) / total
    n = (normal * weight[..., None]).reshape(size, samples, size, samples, 3).sum(axis=(1, 3))
    n /= np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-12)
    return color, n, alpha


def dilate(color, n, alpha, steps):
    filled = alpha > 0.0
    for _ in range(steps):
        if filled.all():
            break
        grown_color, grown_n, count = np.zeros_like(color), np.zeros_like(n), np.zeros(filled.shape)
        for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            shifted = np.roll(filled, (dy, dx), axis=(0, 1))
            grown_color += np.roll(color, (dy, dx), axis=(0, 1)) * shifted[..., None]
            grown_n += np.roll(n, (dy, dx), axis=(0, 1)) * shifted[..., None]
            count += shifted
        new = ~filled & (count > 0)
        color[new] = grown_color[new] / count[new][:, None]
        g = grown_n[new]
        n[new] = g / np.maximum(np.linalg.norm(g, axis=1, keepdims=True), 1e-12)
        filled |= new
    n[~filled] = (0.0, 0.0, 1.0)
    return color, n


# ---- Writing: PNGs (zlib, no imaging library) and the cards' glb

def write_png(path, pixels):
    height, width, channels = pixels.shape
    rows = b"".join(b"\x00" + pixels[y].tobytes() for y in range(height))

    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)

    color_type = {3: 2, 4: 6}[channels]
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, color_type, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(rows, 9)))
        f.write(chunk(b"IEND", b""))


def to_srgb(linear):
    linear = np.clip(linear, 0.0, 1.0)
    return np.where(linear <= 0.0031308, linear * 12.92, 1.055 * np.power(linear, 1.0 / 2.4) - 0.055)


def write_cards(path, square, bottom, rows, albedo_name, normal_name):
    binary = bytearray()
    views, accessors, meshes, nodes = [], [], [], []

    def add(values, component, kind, target, minmax=False):
        raw = values.tobytes()
        while len(binary) % 4:
            binary.append(0)
        views.append({"buffer": 0, "byteOffset": len(binary), "byteLength": len(raw), "target": target})
        binary.extend(raw)
        a = {"bufferView": len(views) - 1, "componentType": component, "count": len(values), "type": kind}
        if minmax:
            a["min"] = values.min(axis=0).tolist()
            a["max"] = values.max(axis=0).tolist()
        accessors.append(a)
        return len(accessors) - 1

    half = square * 0.5
    position = np.array([[-half, bottom + square, 0], [half, bottom + square, 0], [half, bottom, 0], [-half, bottom, 0]], dtype=np.float32)
    normal = np.tile(np.array([0, 0, 1], dtype=np.float32), (4, 1))
    tangent = np.tile(np.array([1, 0, 0, 1], dtype=np.float32), (4, 1))
    indices = np.array([0, 2, 1, 0, 3, 2], dtype=np.uint16)  # counter-clockwise seen from +z
    shared = {"POSITION": add(position, 5126, "VEC3", 34962, True), "NORMAL": add(normal, 5126, "VEC3", 34962),
              "TANGENT": add(tangent, 5126, "VEC4", 34962)}
    index = add(indices, 5123, "SCALAR", 34963)
    u1, v1 = 1.0 / COLUMNS, 1.0 / rows  # view 0's part: the engine moves it to the view it shows
    uv = np.array([[0.0, 0.0], [u1, 0.0], [u1, v1], [0.0, v1]], dtype=np.float32)
    attributes = dict(shared)
    attributes["TEXCOORD_0"] = add(uv, 5126, "VEC2", 34962)
    meshes.append({"name": "Card", "primitives": [{"attributes": attributes, "indices": index, "material": 0}]})
    nodes.append({"name": "Card", "mesh": 0})
    gltf = {
        "asset": {"version": "2.0", "generator": "MoravaEngine bake_impostors.py"},
        "extras": {"views": VIEWS, "columns": COLUMNS, "rows": rows},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": nodes,
        "meshes": meshes,
        "materials": [{"name": os.path.splitext(os.path.basename(path))[0], "alphaMode": "MASK", "alphaCutoff": CUTOFF,
                       "pbrMetallicRoughness": {"baseColorTexture": {"index": 0}, "metallicFactor": 0.0, "roughnessFactor": 0.9},
                       "normalTexture": {"index": 1}}],
        "textures": [{"source": 0, "sampler": 0}, {"source": 1, "sampler": 0}],
        "samplers": [{"magFilter": 9729, "minFilter": 9987, "wrapS": 33071, "wrapT": 33071}],
        "images": [{"uri": albedo_name}, {"uri": normal_name}],
        "accessors": accessors,
        "bufferViews": views,
        "buffers": [{"byteLength": len(binary)}],
    }
    text = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
    text += b" " * ((4 - len(text) % 4) % 4)
    while len(binary) % 4:
        binary.append(0)
    with open(path, "wb") as f:
        f.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(text) + 8 + len(binary)))
        f.write(struct.pack("<II", len(text), 0x4E4F534A) + text)
        f.write(struct.pack("<II", len(binary), 0x004E4942) + bytes(binary))


def bake(model):
    positions, normals, colors = triangles(model)
    radius = float(np.sqrt(positions[..., 0] ** 2 + positions[..., 2] ** 2).max())
    bottom, top = float(positions[..., 1].min()), float(positions[..., 1].max())
    square = max(2.0 * radius, top - bottom) * 1.02  # a little room at the edges
    rows = (VIEWS + COLUMNS - 1) // COLUMNS
    albedo_atlas = np.zeros((rows * CELL, COLUMNS * CELL, 4), dtype=np.uint8)
    normal_atlas = np.zeros((rows * CELL, COLUMNS * CELL, 3), dtype=np.uint8)
    for k in range(VIEWS):
        angle = 2.0 * math.pi * k / VIEWS
        albedo, normal, covered = render_view(positions, normals, colors, angle, square, bottom, CELL * SAMPLES)
        color, n, alpha = downsample(albedo, normal, covered, SAMPLES)
        color, n = dilate(color, n, alpha, DILATE)
        row, column = k // COLUMNS, k % COLUMNS
        cell = (slice(row * CELL, (row + 1) * CELL), slice(column * CELL, (column + 1) * CELL))
        albedo_atlas[cell + (slice(0, 3),)] = np.round(to_srgb(color) * 255.0).astype(np.uint8)
        albedo_atlas[cell + (3,)] = np.round(np.clip(alpha, 0.0, 1.0) * 255.0).astype(np.uint8)
        normal_atlas[cell] = np.round((n * 0.5 + 0.5) * 255.0).astype(np.uint8)
    stem = os.path.splitext(model)[0] + "_Impostor"
    albedo_name, normal_name = os.path.basename(stem) + "_Albedo.png", os.path.basename(stem) + "_Normal.png"
    write_png(stem + "_Albedo.png", albedo_atlas)
    write_png(stem + "_Normal.png", normal_atlas)
    write_cards(stem + ".glb", square, bottom, rows, albedo_name, normal_name)
    print("bake_impostors: wrote %s.glb (%d views, a %.1f m card from %.2f m; the model %d triangles)" % (stem, VIEWS, square, bottom, len(positions)))


def main():
    if len(sys.argv) < 2:
        raise SystemExit("usage: python bake_impostors.py <model.glb> [<model.glb> ...]")
    for model in sys.argv[1:]:
        bake(model)


main()
