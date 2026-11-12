# Makes the racing game's trees and bushes from Poly Haven's CC0 models, in Blender without its window
# (docs/games/Racing_Plan.html, Phase 8.3):
#
#   blender --background --factory-startup --python Tools/TrackBuilder/prepare_tree.py -- Projects/Racing/Foliage/Trees.json [Name ...]
#
# The data file (Trees.json, see README.md) is the source: which Poly Haven model, which of its materials are leaves, how
# many triangles the wood keeps, how many cards the leaves become. Without names, every tree in it is made.
#
# Poly Haven's trees are scans with every leaf modeled: up to two million triangles, far too many for a game that draws
# thousands of them. A game's tree is a few thousand: the trunk and the larger branches, simplified, and the crown as cards
# (quads with a picture of leaves and alpha). This script:
#   1. downloads the model once (its glTF at 1k, and the alpha map where the data asks for it) into SourceDirectory,
#   2. reads its meshes (numpy, the glTF itself: the engine's space, meters, Y up), the node the data names or all,
#   3. stands it on the origin (its foot at 0, 0, 0), at the height the data gives,
#   4. the wood: drops the smallest islands (twigs the leaves hide), then Blender's Decimate to the data's triangles,
#   5. the leaves: groups them into clusters (k-means of the leaves' centers), gives each cluster a card in its main plane
#      (the two directions it spreads most), and draws its leaves' triangles into the card's tile of an atlas: points spread
#      over each triangle, the nearest to the card's front kept, colored from the leaf texture; alpha where a leaf is. The
#      cards' normals point out from the crown's center, so a crown is lit as a whole (sunny on one side) from either face,
#   6. writes a .glb (its own writer: one node, the wood and the leaves as two primitives of one mesh) and its maps in
#      Textures/ beside it; the leaves alphaMode MASK and double-sided (the engine makes them a Cutout surface).
#
# Plain numpy for the heavy parts (Blender's Python has it). Each tree takes a few seconds to a minute.

import bpy
import bmesh
import json
import math
import os
import struct
import sys
import time
import urllib.request
import zlib

import numpy as np


# ------------------------------------------------------------------------------------------------------ the sources
# Poly Haven's API asks each client to name itself (its default Python one is refused)
HEADERS = {"User-Agent": "MoravaEngine-prepare_tree/1.0"}


def fetch_json(url):
    with urllib.request.urlopen(urllib.request.Request(url, headers=HEADERS)) as response:
        return json.load(response)


def fetch_file(url, target):
    with urllib.request.urlopen(urllib.request.Request(url, headers=HEADERS)) as response, open(target, "wb") as f:
        f.write(response.read())


def download_source(source_dir, asset, alpha_maps):
    """The model's glTF (1k) and its files, once; the alpha maps (Poly Haven's 'Alpha' map at 1k) the data asks for."""
    folder = os.path.join(source_dir, asset)
    gltf = os.path.join(folder, asset + ".gltf")
    if not os.path.exists(gltf):
        os.makedirs(os.path.join(folder, "textures"), exist_ok=True)
        files = fetch_json("https://api.polyhaven.com/files/" + asset)
        entry = files["gltf"]["1k"]["gltf"]
        for relative, info in [(asset + ".gltf", entry)] + list(entry["include"].items()):
            target = os.path.join(folder, relative.replace("/", os.sep))
            if not os.path.exists(target):
                fetch_file(info["url"], target)
        print("prepare_tree: downloaded %s" % asset)
    maps = {}
    for material, kind in alpha_maps.items():
        target = os.path.join(folder, "textures", "%s_%s_1k.jpg" % (asset, kind.lower()))
        if not os.path.exists(target):
            files = fetch_json("https://api.polyhaven.com/files/" + asset)
            fetch_file(files[kind]["1k"]["jpg"]["url"], target)
        maps[material] = target
    return gltf, maps


# ------------------------------------------------------------------------------------------------------ reading glTF
COMPONENTS = {5120: np.int8, 5121: np.uint8, 5122: np.int16, 5123: np.uint16, 5125: np.uint32, 5126: np.float32}
WIDTHS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}


def read_accessor(gltf, buffers, index):
    accessor = gltf["accessors"][index]
    view = gltf["bufferViews"][accessor["bufferView"]]
    dtype = np.dtype(COMPONENTS[accessor["componentType"]])
    width = WIDTHS[accessor["type"]]
    count = accessor["count"]
    data = buffers[view["buffer"]]
    offset = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
    stride = view.get("byteStride", dtype.itemsize * width)
    if stride == dtype.itemsize * width:
        array = np.frombuffer(data, dtype=dtype, count=count * width, offset=offset).reshape(count, width)
    else:
        rows = np.frombuffer(data, dtype=np.uint8, count=stride * count, offset=offset).reshape(count, stride)
        array = rows[:, :dtype.itemsize * width].copy().view(dtype).reshape(count, width)
    return array.astype(np.float64) if dtype == np.float32 else array.astype(np.int64)


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


def load_model(path, only_node):
    """The model's triangles in its space: a list of primitives (material name, positions, normals, uvs, triangles)."""
    with open(path, "r", encoding="utf-8") as f:
        gltf = json.load(f)
    folder = os.path.dirname(path)
    buffers = [open(os.path.join(folder, b["uri"]), "rb").read() for b in gltf["buffers"]]
    materials = gltf.get("materials", [])
    primitives = []

    def visit(index, parent, inside):
        node = gltf["nodes"][index]
        world = parent @ node_matrix(node)
        inside = inside or only_node is None or node.get("name") == only_node
        if inside and "mesh" in node:
            normal_matrix = np.linalg.inv(world[:3, :3]).T
            for primitive in gltf["meshes"][node["mesh"]]["primitives"]:
                attributes = primitive["attributes"]
                positions = read_accessor(gltf, buffers, attributes["POSITION"])
                positions = positions @ world[:3, :3].T + world[:3, 3]
                normals = read_accessor(gltf, buffers, attributes["NORMAL"]) @ normal_matrix.T
                normals /= np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-12)
                uvs = read_accessor(gltf, buffers, attributes["TEXCOORD_0"])
                triangles = read_accessor(gltf, buffers, primitive["indices"]).reshape(-1, 3)
                name = materials[primitive["material"]]["name"] if "material" in primitive else "default"
                primitives.append({"Material": name, "Positions": positions, "Normals": normals, "UVs": uvs, "Triangles": triangles})
        for child in node.get("children", []):
            visit(child, world, inside)

    scene = gltf["scenes"][gltf.get("scene", 0)]
    for root in scene["nodes"]:
        visit(root, np.eye(4), False)
    return gltf, primitives


def material_maps(gltf, folder):
    """Each material's maps by kind (albedo, normal, arm), as files."""
    images = gltf.get("images", [])
    textures = gltf.get("textures", [])

    def file_of(info):
        if info is None:
            return None
        return os.path.join(folder, images[textures[info["index"]]["source"]]["uri"].replace("/", os.sep))

    maps = {}
    for material in gltf.get("materials", []):
        pbr = material.get("pbrMetallicRoughness", {})
        maps[material["name"]] = {"Albedo": file_of(pbr.get("baseColorTexture")), "Normal": file_of(material.get("normalTexture")),
                                  "ARM": file_of(pbr.get("metallicRoughnessTexture"))}
    return maps


def load_image(path):
    """An image's pixels, rows from the top (as glTF's texture coordinates: v down), float RGBA."""
    image = bpy.data.images.load(path, check_existing=False)
    width, height = image.size
    pixels = np.empty(width * height * 4, dtype=np.float32)
    image.pixels.foreach_get(pixels)
    bpy.data.images.remove(image)
    return pixels.reshape(height, width, 4)[::-1].copy()


# ------------------------------------------------------------------------------------------------------ islands
def islands(triangles, positions):
    """The connected parts of a mesh (a leaf, a twig): each triangle's island, by shared positions (welded: the file
    splits vertices along texture seams)."""
    keys = np.round(positions / 1e-5).astype(np.int64)
    _, welded = np.unique(keys, axis=0, return_inverse=True)
    welded = welded.reshape(-1)
    parent = np.arange(welded.max() + 1)

    def find(a):
        root = a
        while parent[root] != root:
            root = parent[root]
        while parent[a] != root:
            parent[a], a = root, parent[a]
        return root

    t = welded[triangles]
    for a, b, c in t:
        ra, rb, rc = find(a), find(b), find(c)
        if rb != ra:
            parent[rb] = ra
        if rc != ra and rc != rb:
            parent[rc] = ra
    roots = np.array([find(v) for v in range(len(parent))])
    _, island = np.unique(roots[t[:, 0]], return_inverse=True)
    return island.reshape(-1)


# ------------------------------------------------------------------------------------------------------ the wood
def simplify_wood(primitive, target, keep_factor=12):
    """The trunk and branches at about target triangles: the smallest islands left out first (twigs), then decimated."""
    positions, uvs, triangles = primitive["Positions"], primitive["UVs"], primitive["Triangles"]
    if len(triangles) > target * keep_factor:
        island = islands(triangles, positions)
        counts = np.bincount(island)
        # Islands by size (their box's diagonal), the largest kept until the triangles reach keep_factor times the target
        tri_points = positions[triangles]
        size = np.zeros(len(counts))
        for axis in range(3):
            lo = np.full(len(counts), np.inf)
            hi = np.full(len(counts), -np.inf)
            np.minimum.at(lo, island, tri_points[:, :, axis].min(axis=1))
            np.maximum.at(hi, island, tri_points[:, :, axis].max(axis=1))
            size += (hi - lo) ** 2
        order = np.argsort(-size)
        kept = order[: max(1, int(np.searchsorted(np.cumsum(counts[order]), target * keep_factor)) + 1)]
        mask = np.isin(island, kept)
        triangles = triangles[mask]

    # Blender's Decimate (collapse): a mesh with the texture coordinates per corner
    mesh = bpy.data.meshes.new("wood")
    used, remap = np.unique(triangles.reshape(-1), return_inverse=True)
    faces = remap.reshape(-1, 3)
    mesh.from_pydata(positions[used].tolist(), [], faces.tolist())
    layer = mesh.uv_layers.new(name="UV")
    corner_uvs = uvs[triangles.reshape(-1)]
    layer.data.foreach_set("uv", corner_uvs.astype(np.float32).reshape(-1))
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new("wood", mesh)
    bpy.context.scene.collection.objects.link(obj)
    current = len(mesh.polygons)
    if current > target:
        modifier = obj.modifiers.new("decimate", "DECIMATE")
        modifier.decimate_type = "COLLAPSE"
        modifier.ratio = target / current
        modifier.use_collapse_triangulate = True
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated = obj.evaluated_get(depsgraph)
    result = evaluated.to_mesh()
    result.calc_loop_triangles()
    n = len(result.loop_triangles)
    loops = np.empty(n * 3, dtype=np.int64)
    result.loop_triangles.foreach_get("loops", loops)
    corner_vertex = np.empty(len(result.loops), dtype=np.int64)
    result.loops.foreach_get("vertex_index", corner_vertex)
    co = np.empty(len(result.vertices) * 3, dtype=np.float64)
    result.vertices.foreach_get("co", co)
    co = co.reshape(-1, 3)
    uv = np.empty(len(result.loops) * 2, dtype=np.float64)
    result.uv_layers.active.data.foreach_get("uv", uv)
    uv = uv.reshape(-1, 2)
    normals = np.empty(len(result.corner_normals) * 3, dtype=np.float64)
    result.corner_normals.foreach_get("vector", normals)
    normals = normals.reshape(-1, 3)
    # Corners to vertices: the same position, normal and texture coordinate are one vertex
    corner_positions = co[corner_vertex[loops]]
    corner_normals = normals[loops]
    corner_uv = uv[loops]
    keys = np.concatenate([np.round(corner_positions / 1e-5), np.round(corner_normals * 1e3), np.round(corner_uv * 1e5)], axis=1)
    _, first, inverse = np.unique(keys, axis=0, return_index=True, return_inverse=True)
    evaluated.to_mesh_clear()
    bpy.data.objects.remove(obj)
    bpy.data.meshes.remove(mesh)
    return {"Material": primitive["Material"], "Positions": corner_positions[first], "Normals": corner_normals[first],
            "UVs": corner_uv[first], "Triangles": inverse.reshape(-1, 3)}


# ------------------------------------------------------------------------------------------------------ the leaves
def kmeans(points, weights, k, seed, iterations=12):
    rng = np.random.default_rng(seed)
    k = min(k, len(points))
    centers = points[rng.choice(len(points), size=k, replace=False, p=weights / weights.sum())]
    for _ in range(iterations):
        labels = np.empty(len(points), dtype=np.int64)
        for start in range(0, len(points), 20000):
            chunk = points[start:start + 20000]
            distance = ((chunk[:, None, :] - centers[None, :, :]) ** 2).sum(axis=2)
            labels[start:start + 20000] = distance.argmin(axis=1)
        for c in range(k):
            member = labels == c
            if member.any():
                centers[c] = np.average(points[member], axis=0, weights=weights[member])
    return labels, centers


def bake_cards(leaves, textures, alphas, cards, tile, seed):
    """The leaves as cards: their quads, and the atlas they show (RGBA, rows from the top)."""
    positions = np.concatenate([p["Positions"][p["Triangles"]] for p in leaves])  # (T, 3, 3)
    uvs = np.concatenate([p["UVs"][p["Triangles"]] for p in leaves])
    source = np.concatenate([np.full(len(p["Triangles"]), i) for i, p in enumerate(leaves)])
    centroids = positions.mean(axis=1)
    areas = 0.5 * np.linalg.norm(np.cross(positions[:, 1] - positions[:, 0], positions[:, 2] - positions[:, 0]), axis=1)

    # The leaves (islands of each primitive), their centers and areas
    leaf = np.empty(len(positions), dtype=np.int64)
    offset = 0
    start = 0
    for p in leaves:
        island = islands(p["Triangles"], p["Positions"])
        leaf[start:start + len(island)] = island + offset
        offset += island.max() + 1
        start += len(island)
    leaf_area = np.bincount(leaf, weights=areas)
    leaf_center = np.stack([np.bincount(leaf, weights=centroids[:, a] * areas) for a in range(3)], axis=1) / np.maximum(leaf_area, 1e-12)[:, None]
    crown = np.average(leaf_center, axis=0, weights=leaf_area)

    labels, _ = kmeans(leaf_center, leaf_area + 1e-9, cards, seed)
    cluster = labels[leaf]
    k = labels.max() + 1

    columns = int(math.ceil(math.sqrt(k)))
    rows = int(math.ceil(k / columns))
    width = 1 << int(math.ceil(math.log2(columns * tile)))
    height = 1 << int(math.ceil(math.log2(rows * tile)))
    atlas = np.zeros((height, width, 4), dtype=np.float32)
    depth = np.full((height, width), -np.inf, dtype=np.float32)

    # Each cluster's card: its main plane (the two directions its leaves spread most), facing out from the crown
    frames = []
    for c in range(k):
        member = cluster == c
        points = positions[member].reshape(-1, 3)
        center = centroids[member].mean(axis=0) if member.any() else crown
        if len(points) < 3:
            frames.append(None)
            continue
        _, _, axes = np.linalg.svd(points - center, full_matrices=False)
        e1, e2, e3 = axes[0], axes[1], axes[2]
        if np.dot(e3, center - crown) < 0:
            e3 = -e3
        e2 = np.cross(e3, e1)
        local = points - center
        u, v = local @ e1, local @ e2
        margin = 0.02
        frames.append((center, e1, e2, e3, u.min() - margin, u.max() + margin, v.min() - margin, v.max() + margin))

    # The leaves' triangles drawn into the tiles: points spread over each (more on larger ones), the nearest to the card's
    # front kept for each pixel
    rng = np.random.default_rng(seed + 1)
    images = [load_image(t) for t in textures]
    alpha_images = [load_image(a)[:, :, 0] if a else None for a in alphas]
    inner = tile - 2  # a pixel of empty border around each tile: no bleeding into the next one
    tri_frame = np.array([frames[c] is not None for c in range(k)])[cluster]
    for chunk_start in range(0, len(positions), 200000):
        sl = slice(chunk_start, chunk_start + 200000)
        tri = np.arange(len(positions))[sl]
        tri = tri[tri_frame[tri]]
        if len(tri) == 0:
            continue
        cl = cluster[tri]
        center = np.array([frames[c][0] for c in cl])
        e1 = np.array([frames[c][1] for c in cl])
        e2 = np.array([frames[c][2] for c in cl])
        e3 = np.array([frames[c][3] for c in cl])
        u0 = np.array([frames[c][4] for c in cl]); u1 = np.array([frames[c][5] for c in cl])
        v0 = np.array([frames[c][6] for c in cl]); v1 = np.array([frames[c][7] for c in cl])
        local = positions[tri] - center[:, None, :]
        pu = ((local * e1[:, None, :]).sum(axis=2) - u0[:, None]) / (u1 - u0)[:, None] * inner + 1
        pv = ((local * e2[:, None, :]).sum(axis=2) - v0[:, None]) / (v1 - v0)[:, None] * inner + 1
        pd = (local * e3[:, None, :]).sum(axis=2)
        area_px = 0.5 * np.abs((pu[:, 1] - pu[:, 0]) * (pv[:, 2] - pv[:, 0]) - (pu[:, 2] - pu[:, 0]) * (pv[:, 1] - pv[:, 0]))
        count = np.clip(np.ceil(area_px * 2.0).astype(np.int64) + 1, 1, 400)
        which = np.repeat(np.arange(len(tri)), count)
        r1 = np.sqrt(rng.random(len(which)))
        r2 = rng.random(len(which))
        b0, b1, b2 = 1 - r1, r1 * (1 - r2), r1 * r2

        def mix(values):
            return values[which, 0] * b0 + values[which, 1] * b1 + values[which, 2] * b2

        x = mix(pu); y = mix(pv); d = mix(pd)
        tu = mix(uvs[tri][:, :, 0]); tv = mix(uvs[tri][:, :, 1])
        src = source[tri][which]
        color = np.zeros((len(which), 4), dtype=np.float32)
        keep = np.ones(len(which), dtype=bool)
        for i, image in enumerate(images):
            m = src == i
            if not m.any():
                continue
            h, w = image.shape[:2]
            px = (np.floor(np.mod(tu[m], 1.0) * w).astype(np.int64)) % w
            py = (np.floor(np.mod(tv[m], 1.0) * h).astype(np.int64)) % h
            color[m] = image[py, px]
            if alpha_images[i] is not None:
                keep[np.where(m)[0]] = alpha_images[i][py, px] >= 0.5
            else:
                keep[np.where(m)[0]] = image[py, px, :3].max(axis=1) > 0.04  # the leaf texture's black background
        tile_of = cl[which]
        gx = (tile_of % columns) * tile + np.clip(x, 0, tile - 1).astype(np.int64)
        gy = (tile_of // columns) * tile + np.clip(y, 0, tile - 1).astype(np.int64)
        keep &= (x >= 0) & (x < tile) & (y >= 0) & (y < tile)
        pixel = (gy * width + gx)[keep]
        d = d[keep].astype(np.float32)
        color = color[keep]
        # The nearest to the card's front per pixel (largest depth along its normal)
        order = np.lexsort((-d, pixel))
        pixel, d, color = pixel[order], d[order], color[order]
        unique, first = np.unique(pixel, return_index=True)
        flat_depth = depth.reshape(-1)
        flat_atlas = atlas.reshape(-1, 4)
        better = d[first] > flat_depth[unique]
        flat_depth[unique[better]] = d[first][better]
        flat_atlas[unique[better], :3] = color[first][better][:, :3]
        flat_atlas[unique[better], 3] = 1.0

    # Pinholes between the points closed (a pixel with leaves on most sides), then the colors bled into the empty pixels
    # (mipmaps average them in: no dark fringe)
    covered = atlas[:, :, 3] > 0
    neighbours = sum(np.roll(np.roll(covered, dy, axis=0), dx, axis=1).astype(np.int32)
                     for dy in (-1, 0, 1) for dx in (-1, 0, 1) if dy or dx)
    fill = (~covered) & (neighbours >= 5)
    if fill.any():
        rgb_sum = sum(np.roll(np.roll(atlas[:, :, :3] * covered[:, :, None], dy, axis=0), dx, axis=1)
                      for dy in (-1, 0, 1) for dx in (-1, 0, 1) if dy or dx)
        atlas[fill, :3] = rgb_sum[fill] / neighbours[fill][:, None]
        atlas[fill, 3] = 1.0
        covered |= fill
    color_known = covered.copy()
    for _ in range(8):
        known = color_known.astype(np.float32)
        total = sum(np.roll(np.roll(known, dy, axis=0), dx, axis=1) for dy in (-1, 0, 1) for dx in (-1, 0, 1))
        rgb = sum(np.roll(np.roll(atlas[:, :, :3] * known[:, :, None], dy, axis=0), dx, axis=1)
                  for dy in (-1, 0, 1) for dx in (-1, 0, 1))
        grow = (~color_known) & (total > 0)
        atlas[grow, :3] = rgb[grow] / total[grow][:, None]
        color_known |= grow

    # The cards: a quad each, its corners' normals out from the crown (flattened a little: a crown is wider than tall)
    quad_positions, quad_normals, quad_uvs, quad_triangles = [], [], [], []
    for c in range(k):
        if frames[c] is None:
            continue
        center, e1, e2, e3, u0, u1, v0, v1 = frames[c]
        tx, ty = (c % columns) * tile, (c // columns) * tile
        base = len(quad_positions)
        for (u, v, px, py) in ((u0, v0, 1, 1), (u1, v0, tile - 1, 1), (u1, v1, tile - 1, tile - 1), (u0, v1, 1, tile - 1)):
            p = center + e1 * u + e2 * v
            n = (p - crown) * np.array([1.0, 0.7, 1.0]) + np.array([0.0, 0.25, 0.0]) * np.linalg.norm(p - crown)
            quad_positions.append(p)
            quad_normals.append(n / max(np.linalg.norm(n), 1e-9))
            quad_uvs.append(((tx + px) / width, (ty + py) / height))
        quad_triangles += [(base, base + 1, base + 2), (base, base + 2, base + 3)]
    coverage = covered.sum() / float(k * tile * tile)
    return ({"Material": "Leaves", "Positions": np.array(quad_positions), "Normals": np.array(quad_normals),
             "UVs": np.array(quad_uvs), "Triangles": np.array(quad_triangles, dtype=np.int64)}, atlas, k, coverage)


# ------------------------------------------------------------------------------------------------------ writing
def png_bytes(rgba):
    """RGBA float (rows from the top) as PNG."""
    data = (np.clip(rgba, 0, 1) * 255 + 0.5).astype(np.uint8)
    height, width = data.shape[:2]
    raw = b"".join(b"\x00" + data[y].tobytes() for y in range(height))

    def chunk(kind, payload):
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)

    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def write_glb(path, primitives):
    """primitives: dicts with Positions, Normals, UVs, Triangles and a Material dict (glTF material + its images)."""
    binary = bytearray()
    views, accessors, images, textures, materials, mesh_primitives = [], [], [], [], [], []

    def add_view(data, target=None):
        while len(binary) % 4:
            binary.append(0)
        view = {"buffer": 0, "byteOffset": len(binary), "byteLength": len(data)}
        if target:
            view["target"] = target
        binary.extend(data)
        views.append(view)
        return len(views) - 1

    def add_accessor(array, kind, component, target, minmax=False):
        data = np.ascontiguousarray(array).tobytes()
        accessor = {"bufferView": add_view(data, target), "componentType": component, "count": len(array), "type": kind}
        if minmax:
            accessor["min"] = array.min(axis=0).tolist()
            accessor["max"] = array.max(axis=0).tolist()
        accessors.append(accessor)
        return len(accessors) - 1

    # The maps as files beside the model, in Textures/ (the engine loads a model's maps from files, not from the .glb)
    folder = os.path.dirname(path)
    stem = os.path.splitext(os.path.basename(path))[0]
    written = {}

    def add_image(slot_name, data, mime):
        key = hash(data)
        if key not in written:
            file = "Textures/%s_%s.%s" % (stem, slot_name, "png" if mime == "image/png" else "jpg")
            os.makedirs(os.path.join(folder, "Textures"), exist_ok=True)
            with open(os.path.join(folder, file.replace("/", os.sep)), "wb") as f:
                f.write(data)
            images.append({"uri": file, "mimeType": mime})
            textures.append({"source": len(images) - 1})
            written[key] = len(textures) - 1
        return written[key]

    for primitive in primitives:
        material = dict(primitive["Material"]["glTF"])
        names = {"baseColorTexture": "Albedo", "normalTexture": "Normal", "metallicRoughnessTexture": "ARM", "occlusionTexture": "ARM"}
        for slot, (data, mime) in primitive["Material"]["Images"].items():
            index = add_image(primitive["Material"]["glTF"]["name"].replace(stem + "_", "") + "_" + names[slot], data, mime)
            if slot == "baseColorTexture" or slot == "metallicRoughnessTexture":
                material.setdefault("pbrMetallicRoughness", {})[slot] = {"index": index}
            else:
                material[slot] = {"index": index}
        materials.append(material)
        positions = primitive["Positions"].astype(np.float32)
        attributes = {"POSITION": add_accessor(positions, "VEC3", 5126, 34962, True),
                      "NORMAL": add_accessor(primitive["Normals"].astype(np.float32), "VEC3", 5126, 34962),
                      "TEXCOORD_0": add_accessor(primitive["UVs"].astype(np.float32), "VEC2", 5126, 34962)}
        indices = add_accessor(primitive["Triangles"].astype(np.uint32).reshape(-1), "SCALAR", 5125, 34963)
        mesh_primitives.append({"attributes": attributes, "indices": indices, "material": len(materials) - 1})

    gltf = {"asset": {"version": "2.0", "generator": "MoravaEngine prepare_tree.py"}, "scene": 0, "scenes": [{"nodes": [0]}],
            "nodes": [{"name": os.path.splitext(os.path.basename(path))[0], "mesh": 0}],
            "meshes": [{"name": os.path.splitext(os.path.basename(path))[0], "primitives": mesh_primitives}],
            "materials": materials, "textures": textures, "images": images, "accessors": accessors, "bufferViews": views,
            "buffers": [{"byteLength": len(binary)}]}
    while len(binary) % 4:
        binary.append(0)
    text = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
    text += b" " * ((4 - len(text) % 4) % 4)
    with open(path, "wb") as f:
        f.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(text) + 8 + len(binary)))
        f.write(struct.pack("<II", len(text), 0x4E4F534A) + text)
        f.write(struct.pack("<II", len(binary), 0x004E4942) + bytes(binary))


def jpeg(path, size):
    """A map as JPEG, made smaller when it is larger than size (bark seen from a car needs no 1k map)."""
    image = bpy.data.images.load(path, check_existing=False)
    if image.size[0] <= size:
        bpy.data.images.remove(image)
        with open(path, "rb") as f:
            return (f.read(), "image/jpeg")
    image.scale(size, max(1, image.size[1] * size // image.size[0]))
    target = os.path.join(bpy.app.tempdir, "prepare_tree_map.jpg")
    scene = bpy.context.scene
    scene.render.image_settings.file_format = "JPEG"
    scene.render.image_settings.quality = 88
    image.save_render(target, scene=scene)
    bpy.data.images.remove(image)
    with open(target, "rb") as f:
        return (f.read(), "image/jpeg")


def clump(primitives, copies, radius, tilt, seed):
    """A bush from a sprig: copies of the model around its foot, each turned at random and tilted outwards."""
    rng = np.random.default_rng(seed)
    result = []
    for p in primitives:
        parts = {"Positions": [], "Normals": [], "UVs": [], "Triangles": []}
        base = 0
        for i in range(copies):
            yaw = 2 * math.pi * (i + rng.random() * 0.6) / copies
            lean = math.radians(tilt * (0.5 + rng.random() * 0.5))
            size = 0.8 + rng.random() * 0.4
            cy, sy, cl, sl = math.cos(yaw), math.sin(yaw), math.cos(lean), math.sin(lean)
            turn = np.array([[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]])
            lean_m = np.array([[cl, -sl, 0], [sl, cl, 0], [0, 0, 1]])  # around z: its +x end lifts
            rotation = turn @ lean_m
            offset = np.array([cy, 0.0, -sy]) * radius * (0.3 + 0.7 * rng.random())
            parts["Positions"].append(p["Positions"] @ (rotation * size).T + offset)
            parts["Normals"].append(p["Normals"] @ rotation.T)
            parts["UVs"].append(p["UVs"])
            parts["Triangles"].append(p["Triangles"] + base)
            base += len(p["Positions"])
        result.append({"Material": p["Material"], **{k: np.concatenate(v) for k, v in parts.items()}})
    return result


# ------------------------------------------------------------------------------------------------------ a tree
def make_tree(entry, data_dir, source_dir, output_dir, defaults):
    started = time.time()
    name = entry["Name"]
    asset = entry["Source"]
    gltf_path, alpha_maps = download_source(source_dir, asset, entry.get("AlphaMaps", {}))
    gltf, primitives = load_model(gltf_path, entry.get("Node"))
    maps = material_maps(gltf, os.path.dirname(gltf_path))
    if not primitives:
        raise RuntimeError("%s: no meshes (Node '%s'?)" % (name, entry.get("Node")))

    if entry.get("Clump"):
        c = entry["Clump"]
        primitives = clump(primitives, c.get("Copies", 8), c.get("Radius", 0.3), c.get("Tilt", 35.0), entry.get("Seed", 1))

    # Its foot at the origin: the lowest point at 0, the middle of what is near the ground at x = z = 0
    points = np.concatenate([p["Positions"] for p in primitives])
    low = points[:, 1].min()
    tall = points[:, 1].max() - low
    foot = points[points[:, 1] < low + 0.05 * tall]
    shift = np.array([foot[:, 0].mean(), low, foot[:, 2].mean()])
    scale = entry["Height"] / tall if entry.get("Height") else 1.0
    for p in primitives:
        p["Positions"] = (p["Positions"] - shift) * scale
    print("prepare_tree: %s (%s): %.2f m tall in the file%s, %d triangles" % (name, asset, tall, ", made %.2f m" % (tall * scale) if scale != 1.0 else "",
                                                                            sum(len(p["Triangles"]) for p in primitives)))

    leaf_names = set(entry["Leaves"])
    leaves = [p for p in primitives if p["Material"] in leaf_names]
    wood_by_material = {}
    for p in primitives:
        if p["Material"] not in leaf_names:
            wood_by_material.setdefault(p["Material"], []).append(p)

    out = []
    for material, parts in wood_by_material.items():
        # One primitive per material (the file may have several)
        base = 0
        merged = {"Material": material, "Positions": [], "Normals": [], "UVs": [], "Triangles": []}
        for p in parts:
            merged["Positions"].append(p["Positions"]); merged["Normals"].append(p["Normals"]); merged["UVs"].append(p["UVs"])
            merged["Triangles"].append(p["Triangles"] + base)
            base += len(p["Positions"])
        merged = {k: (np.concatenate(v) if k != "Material" else v) for k, v in merged.items()}
        target = entry.get("Wood", {}).get(material, defaults["WoodTriangles"])
        if target <= 0:
            continue
        before = len(merged["Triangles"])
        wood = simplify_wood(merged, target)
        print("prepare_tree:   wood '%s': %d -> %d triangles" % (material, before, len(wood["Triangles"])))
        m = maps.get(material, {})
        images = {}
        size = entry.get("WoodTextureSize", defaults["WoodTextureSize"])
        if m.get("Albedo"):
            images["baseColorTexture"] = jpeg(m["Albedo"], size)
        if m.get("Normal"):
            images["normalTexture"] = jpeg(m["Normal"], size)
        if m.get("ARM"):
            images["metallicRoughnessTexture"] = jpeg(m["ARM"], size)
            images["occlusionTexture"] = images["metallicRoughnessTexture"]
        wood["Material"] = {"glTF": {"name": name + "_" + ("Bark" if len(out) == 0 else "Wood%d" % len(out)),
                                     "pbrMetallicRoughness": {"metallicFactor": 0.0, "roughnessFactor": 1.0}},
                            "Images": images}
        out.append(wood)

    cards = entry.get("Cards", defaults["Cards"])
    tile = entry.get("TileSize", defaults["TileSize"])
    textures = [maps[p["Material"]]["Albedo"] for p in leaves]
    alphas = [alpha_maps.get(p["Material"]) for p in leaves]
    card_mesh, atlas, count, coverage = bake_cards(leaves, textures, alphas, cards, tile, entry.get("Seed", 1))
    card_mesh["Material"] = {"glTF": {"name": name + "_Leaves", "alphaMode": "MASK", "alphaCutoff": 0.5, "doubleSided": True,
                                      "pbrMetallicRoughness": {"metallicFactor": 0.0, "roughnessFactor": entry.get("LeafRoughness", 0.75)}},
                             "Images": {"baseColorTexture": (png_bytes(atlas), "image/png")}}
    out.append(card_mesh)
    print("prepare_tree:   leaves: %d triangles -> %d cards (%d triangles), atlas %dx%d, %.0f%% of the tiles covered" % (
        sum(len(p["Triangles"]) for p in leaves), count, len(card_mesh["Triangles"]), atlas.shape[1], atlas.shape[0], coverage * 100))

    os.makedirs(output_dir, exist_ok=True)
    path = os.path.join(output_dir, name + ".glb")
    write_glb(path, out)
    total = sum(len(p["Triangles"]) for p in out)
    print("prepare_tree:   wrote %s: %d triangles, %.1f MB, in %.0f s" % (path, total, os.path.getsize(path) / 1e6, time.time() - started))


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if not args:
        raise SystemExit("usage: blender --background --factory-startup --python prepare_tree.py -- <Trees.json> [Name ...]")
    data_path = os.path.abspath(args[0])
    names = set(args[1:])
    with open(data_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data_dir = os.path.dirname(data_path)
    source_dir = os.path.normpath(os.path.join(data_dir, data.get("SourceDirectory", "../Library/PolyHaven")))
    output_dir = os.path.normpath(os.path.join(data_dir, data.get("OutputDirectory", "../Assets/Models/Trees")))
    defaults = {"WoodTriangles": 600, "Cards": 256, "TileSize": 64, "WoodTextureSize": 512}
    defaults.update(data.get("Defaults", {}))
    for entry in data["Trees"]:
        if names and entry["Name"] not in names:
            continue
        make_tree(entry, data_dir, source_dir, output_dir, defaults)


main()
