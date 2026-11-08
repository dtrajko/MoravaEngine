# Flattens the tread of a model's tires (docs/games/Racing_Plan.html, Phase 2), plain Python (no Blender):
#
#   python Tools/TrackBuilder/flatten_tires.py <in.glb> <out.glb> [edge]
#
# Kenney's Car Kit tires are full round only across the middle third of their width: from there they chamfer down to a
# smaller radius at both sides. Widened (a wheel scaled along its axle) they look as if only their middle touched the road.
# This moves the tread's inner rings of corners (those at the full radius, between the tire's two sides) out to `edge` of
# the width from each side (default 0.07: the tread flat across 86% of the width, a narrow chamfer at each side), and
# recomputes the normals of the faces it moved. Everything else of the file stays as it is: the other meshes, the
# materials, the texture's file name (beside the model), the nodes.
#
# Which meshes: those whose name starts with "wheel". A wheel turns around its X axis (its axle): its corners' distance
# from the axle is their radius, their X their place across the tire.

import json
import math
import struct
import sys

FLOAT = 5126
UNSIGNED_SHORT = 5123
UNSIGNED_INT = 5125


def read_glb(path):
    data = bytearray(open(path, 'rb').read())
    magic, version, length = struct.unpack_from('<III', data, 0)
    if magic != 0x46546C67 or version != 2:
        raise SystemExit("%s: not a glTF 2.0 binary (.glb)" % path)
    json_length, json_type = struct.unpack_from('<II', data, 12)
    gltf = json.loads(data[20:20 + json_length].decode('utf-8'))
    bin_header = 20 + json_length
    bin_length, bin_type = struct.unpack_from('<II', data, bin_header)
    if bin_type != 0x004E4942:
        raise SystemExit("%s: no binary chunk" % path)
    return data, gltf, bin_header + 8


def accessor_location(gltf, bin_start, index):
    accessor = gltf['accessors'][index]
    view = gltf['bufferViews'][accessor['bufferView']]
    offset = bin_start + view.get('byteOffset', 0) + accessor.get('byteOffset', 0)
    return accessor, view, offset


def read_vec3s(data, gltf, bin_start, index):
    accessor, view, offset = accessor_location(gltf, bin_start, index)
    if accessor['componentType'] != FLOAT or accessor['type'] != 'VEC3':
        raise SystemExit("accessor %d: not float vectors" % index)
    stride = view.get('byteStride', 12)
    return [list(struct.unpack_from('<3f', data, offset + stride * k)) for k in range(accessor['count'])], offset, stride


def write_vec3s(data, offset, stride, values):
    for k, v in enumerate(values):
        struct.pack_into('<3f', data, offset + stride * k, *v)


def read_indices(data, gltf, bin_start, index):
    accessor, view, offset = accessor_location(gltf, bin_start, index)
    fmt = {UNSIGNED_SHORT: '<H', UNSIGNED_INT: '<I'}.get(accessor['componentType'])
    if fmt is None:
        raise SystemExit("accessor %d: indices of an unexpected type" % index)
    size = struct.calcsize(fmt)
    return [struct.unpack_from(fmt, data, offset + size * k)[0] for k in range(accessor['count'])]


def sub(a, b): return [a[0] - b[0], a[1] - b[1], a[2] - b[2]]
def cross(a, b): return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]
def normalize(a):
    length = math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])
    return [a[0] / length, a[1] / length, a[2] / length] if length > 1e-12 else a


def flatten(data, gltf, bin_start, primitive, edge):
    positions, position_offset, position_stride = read_vec3s(data, gltf, bin_start, primitive['attributes']['POSITION'])
    normals, normal_offset, normal_stride = read_vec3s(data, gltf, bin_start, primitive['attributes']['NORMAL'])
    indices = read_indices(data, gltf, bin_start, primitive['indices'])

    radius = lambda p: math.hypot(p[1], p[2])
    x_min = min(p[0] for p in positions)
    x_max = max(p[0] for p in positions)
    width = x_max - x_min
    full = max(radius(p) for p in positions)
    middle = 0.5 * (x_min + x_max)
    tolerance = 1e-4 * max(width, full)

    # The tread's corners between the sides, out to `edge` of the width from the side they are nearer to
    moved = set()
    for k, p in enumerate(positions):
        if radius(p) >= full * 0.995 and x_min + tolerance < p[0] < x_max - tolerance:
            target = x_min + edge * width if p[0] < middle else x_max - edge * width
            if abs(target - p[0]) > tolerance:
                p[0] = target
                moved.add(k)

    # The faces that moved: their corners' normals again (each corner: its faces' normals, weighted by their areas)
    triangles = [indices[t:t + 3] for t in range(0, len(indices) - 2, 3)]
    touched = set()
    for tri in triangles:
        if moved.intersection(tri):
            touched.update(tri)
    sums = {k: [0.0, 0.0, 0.0] for k in touched}
    for a, b, c in triangles:
        if a in touched or b in touched or c in touched:
            n = cross(sub(positions[b], positions[a]), sub(positions[c], positions[a]))
            for k in (a, b, c):
                if k in sums:
                    sums[k] = [sums[k][0] + n[0], sums[k][1] + n[1], sums[k][2] + n[2]]
    for k, n in sums.items():
        # keep the side a corner's normal faced (a mesh's winding could be either way)
        new = normalize(n)
        if new[0] * normals[k][0] + new[1] * normals[k][1] + new[2] * normals[k][2] < 0.0:
            new = [-new[0], -new[1], -new[2]]
        normals[k] = new

    write_vec3s(data, position_offset, position_stride, positions)
    write_vec3s(data, normal_offset, normal_stride, normals)
    return len(moved), len(touched), x_min, x_max, full


def main():
    if len(sys.argv) < 3:
        raise SystemExit("flatten_tires.py <in.glb> <out.glb> [edge]")
    source, target = sys.argv[1], sys.argv[2]
    edge = float(sys.argv[3]) if len(sys.argv) > 3 else 0.07
    data, gltf, bin_start = read_glb(source)
    count = 0
    for mesh in gltf['meshes']:
        if not mesh.get('name', '').lower().startswith('wheel'):
            continue
        for primitive in mesh['primitives']:
            moved, touched, x_min, x_max, full = flatten(data, gltf, bin_start, primitive, edge)
            print("flatten_tires: %s: %d tread corners moved, %d normals again (width %.3f, radius %.3f)" %
                  (mesh['name'], moved, touched, x_max - x_min, full))
            count += 1
    if count == 0:
        raise SystemExit("flatten_tires: no meshes named wheel...")
    open(target, 'wb').write(data)
    print("flatten_tires: wrote %s" % target)


main()
