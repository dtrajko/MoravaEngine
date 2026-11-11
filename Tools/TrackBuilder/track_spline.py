# The track data and its spline, shared by build_track.py (in Blender) and preview_track.py (plain Python): no Blender here.
#
# A track's data file (.track.json, see README.md) is a closed list of control points. sample_track() runs a centripetal
# Catmull-Rom spline through them and gives evenly spaced cross sections (samples), each with its frame (tangent, right,
# up, turned by the bank), its width, wall heights and flags.
#
# Coordinates are the engine's: meters, Y up, a car looks along -Z.

import json
import math


# ---------------------------------------------------------------------------------------------------------------- vectors
def add(a, b): return (a[0] + b[0], a[1] + b[1], a[2] + b[2])
def sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def mul(a, s): return (a[0] * s, a[1] * s, a[2] * s)
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
def length(a): return math.sqrt(dot(a, a))
def normalize(a):
    l = length(a)
    return mul(a, 1.0 / l) if l > 1e-9 else (0.0, 0.0, 0.0)
def lerp(a, b, t): return a + (b - a) * t
def smoothstep(t): return t * t * (3.0 - 2.0 * t)

WORLD_UP = (0.0, 1.0, 0.0)
CURB_TURN_WINDOW = 20.0  # meters before and after a sample over which its turn's direction is measured


# ------------------------------------------------------------------------------------------------------------- the data
DEFAULTS = {
    "Width": 16.0,          # meters, curb to curb
    "Bank": 0.0,            # degrees
    "Walls": True,
    "Curbs": False,
    "Supports": True,
    # "WallHeight": meters, one number for both sides or [left, right]; Settings' WallHeight where a point has none
}
SETTINGS = {
    "SampleSpacing": 1.0,   # meters between two cross sections
    "ChunkLength": 100.0,   # meters of track per chunk (culled one by one)
    "CurbWidth": 1.5,
    "CurbStripe": 3.0,      # meters per red or white stripe
    "WallHeight": 1.0,
    "WallThickness": 0.5,
    "SlabThickness": 0.8,
    "SupportSpacing": 24.0,
    "SupportSize": 1.4,     # meters, a square pillar
    "GroundHeight": 0.0,    # the engine's y of the ground below the track: supports go down to here (and a meter more)
    "StartLineWidth": 2.0,  # meters along the track, at distance 0 (the first point)
    "CenterlineSpacing": 2.0,
    "Gantry": False,        # a start and finish gantry over the start line (two pillars and a beam)
    "GantryHeight": 7.0,    # meters from the road to the beam's underside
    "MeshTolerance": 0.05,  # meters: the models keep a cross section only where leaving it out would move the surface more
    "MeshMaxLength": 50.0,  # meters: the longest a model's quad gets along the track (a long straight: a few of them)
    "MeshMaxAngle": 0.1,    # degrees: how far a quad's slope along the track may be from the surface's it replaces
    "MeshMaxFold": 0.25,    # degrees: the largest fold between two of the road's triangles where the bank changes (road_twist)
}


def road_twist(s0, s1):
    """Meters the road's quad from sample s0 to s1 is twisted (where the bank changes): its last corner's height off the
    plane of the other three. A twisted quad is two triangles with a fold along their diagonal; for a quad Width wide
    and L long the fold is about twist / L along the track (the car pitches crossing it) and twist / Width across it
    (it rolls). Before MeshMaxFold the road was one quad wall to wall: up to 5.6 degrees where the circuit's turn 1
    banks, which the car felt as a bumpy road."""
    def edge(sample, side):
        return add(sample["Position"], mul(sample["Right"], side * 0.5 * sample["Width"]))
    a, b, c, d = edge(s0, -1.0), edge(s0, 1.0), edge(s1, 1.0), edge(s1, -1.0)
    return dot(sub(sub(c, b), sub(d, a)), normalize(add(s0["Up"], s1["Up"])))


def wall_heights(value):
    # A point's WallHeight: one number for both sides, or [left, right]
    if isinstance(value, (list, tuple)):
        return float(value[0]), float(value[1])
    return float(value), float(value)


def load_track(path):
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)
    settings = dict(SETTINGS)
    settings.update(data.get("Settings", {}))
    defaults = dict(DEFAULTS)
    defaults["WallHeight"] = settings["WallHeight"]
    defaults.update(data.get("Defaults", {}))
    points = []
    for p in data["Points"]:
        point = dict(defaults)
        point.update(p)
        point["Position"] = tuple(float(v) for v in p["Position"])
        point["WallLeft"], point["WallRight"] = wall_heights(point["WallHeight"])
        points.append(point)
    if len(points) < 4:
        raise ValueError("a track needs at least 4 points")
    return data, settings, points


# ------------------------------------------------------------------------------------------------- the spline and samples
def catmull_rom(p0, p1, p2, p3, t, alpha=0.5):
    # Centripetal Catmull-Rom (alpha 0.5): no loops or cusps where points are unevenly spaced
    def tj(ti, a, b):
        return ti + max(length(sub(b, a)), 1e-6) ** alpha
    t0 = 0.0
    t1 = tj(t0, p0, p1)
    t2 = tj(t1, p1, p2)
    t3 = tj(t2, p2, p3)
    u = lerp(t1, t2, t)
    def blend(a, b, ta, tb):
        return add(mul(a, (tb - u) / (tb - ta)), mul(b, (u - ta) / (tb - ta)))
    a1 = blend(p0, p1, t0, t1)
    a2 = blend(p1, p2, t1, t2)
    a3 = blend(p2, p3, t2, t3)
    b1 = blend(a1, a2, t0, t2)
    b2 = blend(a2, a3, t1, t3)
    return blend(b1, b2, t1, t2)


def smooth_values(knots, values, total):
    """A value along a closed track through its points' values: a monotone cubic (PCHIP, Fritsch-Carlson) over the
    distance. It runs through each point without stopping there and never overshoots: flat where the values hold, a
    single change between two held values a smoothstep, as before. Before, each point's value went to the next by a
    smoothstep of its own, which stops at every point: a bank changing over several points rose in steps 12 to 15 m apart
    on the circuit, and 10 m out from the center line each step was a dip and a rise of the road (1.2 degrees in 2 m,
    before and after turn 1). Returns a function of the distance s (0 to total). Points at the same distance (a point
    given twice) count once."""
    xs, ys = [], []
    for x, y in zip(knots, values):
        if xs and x - xs[-1] < 1e-6:
            continue
        xs.append(x)
        ys.append(float(y))
    n = len(xs)
    if n < 2:
        value = ys[0] if ys else 0.0
        return lambda s: value
    # Closed: the interval after the last point goes to the first, one lap on
    h = [(xs[(i + 1) % n] + (total if i == n - 1 else 0.0)) - xs[i] for i in range(n)]
    d = [(ys[(i + 1) % n] - ys[i]) / h[i] for i in range(n)]
    slopes = []
    for i in range(n):
        d0, d1, h0, h1 = d[i - 1], d[i], h[i - 1], h[i]
        if d0 * d1 <= 0.0:
            slopes.append(0.0)   # a peak, a dip or where a value starts or stops changing: held there
        else:
            w0, w1 = 2.0 * h1 + h0, h1 + 2.0 * h0
            slopes.append((w0 + w1) / (w0 / d0 + w1 / d1))

    def at(s):
        s = s % total
        lo, hi = 0, n          # the last point at or before s (before the first: the last point's interval)
        while hi - lo > 1:
            mid = (lo + hi) // 2
            if xs[mid] <= s:
                lo = mid
            else:
                hi = mid
        i = lo if s >= xs[0] else n - 1
        x0 = xs[i] if s >= xs[0] else xs[i] - total
        t = (s - x0) / h[i]
        t2, t3 = t * t, t * t * t
        return ((2 * t3 - 3 * t2 + 1) * ys[i] + (t3 - 2 * t2 + t) * h[i] * slopes[i] +
                (-2 * t3 + 3 * t2) * ys[(i + 1) % n] + (t3 - t2) * h[i] * slopes[(i + 1) % n])
    return at


def sample_track(points, spacing):
    # A dense polyline first (segment, t, position), then evenly spaced samples along its length
    n = len(points)
    dense = []
    steps = 64
    for i in range(n):
        p0 = points[(i - 1) % n]["Position"]
        p1 = points[i]["Position"]
        p2 = points[(i + 1) % n]["Position"]
        p3 = points[(i + 2) % n]["Position"]
        for k in range(steps):
            t = k / steps
            dense.append((i, t, catmull_rom(p0, p1, p2, p3, t)))
    distances = [0.0]
    for k in range(1, len(dense) + 1):
        distances.append(distances[-1] + length(sub(dense[k % len(dense)][2], dense[k - 1][2])))
    total = distances[-1]
    count = max(8, int(round(total / spacing)))
    step = total / count
    # Width and bank through all the points at once, by the distance along the track (smooth_values)
    point_s = [distances[i * steps] for i in range(n)]
    width_at = smooth_values(point_s, [p["Width"] for p in points], total)
    bank_at = smooth_values(point_s, [p["Bank"] for p in points], total)

    samples = []
    k = 0
    for j in range(count):
        s = j * step
        while distances[k + 1] < s:
            k += 1
        f = (s - distances[k]) / max(distances[k + 1] - distances[k], 1e-9)
        a, b = dense[k], dense[(k + 1) % len(dense)]
        position = add(a[2], mul(sub(b[2], a[2]), f))
        segment, t = a[0], a[1] + (1.0 / steps) * f
        pa, pb = points[segment], points[(segment + 1) % n]
        e = smoothstep(min(max(t, 0.0), 1.0))
        samples.append({
            "s": s,
            "Point": segment,
            "Position": position,
            "Width": width_at(s),
            "Bank": bank_at(s),
            # wall heights blend to the next point's like the width (where the next point has walls)
            "WallLeft": lerp(pa["WallLeft"], pb["WallLeft"], e) if pb["Walls"] else pa["WallLeft"],
            "WallRight": lerp(pa["WallRight"], pb["WallRight"], e) if pb["Walls"] else pa["WallRight"],
            # flags hold for the segment from a point to the next
            "Walls": bool(pa["Walls"]),
            "Curbs": bool(pa["Curbs"]),
            "Supports": bool(pa["Supports"]),
        })

    # Frames: tangent from the neighbors, right from the world's up, both turned around the tangent by the bank
    for j, sample in enumerate(samples):
        prev = samples[(j - 1) % count]["Position"]
        nxt = samples[(j + 1) % count]["Position"]
        tangent = normalize(sub(nxt, prev))
        right = normalize(cross(tangent, WORLD_UP))
        up = cross(right, tangent)
        bank = math.radians(sample["Bank"])
        c, s = math.cos(bank), math.sin(bank)
        sample["Tangent"] = tangent
        sample["Right"] = sub(mul(right, c), mul(up, s))   # positive bank: the right edge goes down
        sample["Up"] = add(mul(up, c), mul(right, s))

    # The inner side of the turn at each sample (-1: left, 1: right), where its curb goes: the way the track turns over
    # CurbTurnWindow meters around it (a wide window: a curb doesn't jump sides in a nearly straight stretch)
    window = max(1, int(round(CURB_TURN_WINDOW / spacing)))
    for j, sample in enumerate(samples):
        before = samples[(j - window) % count]["Tangent"]
        after = samples[(j + window) % count]["Tangent"]
        turn = cross(before, after)[1]  # positive: turning left (seen from above)
        sample["Inner"] = -1.0 if turn > 0.0 else 1.0
    return samples, total


def turn_radius(samples, j, window=5):
    # Meters, seen from above, over window samples before and after j (a circle through the three points); positive turns
    # left, negative right, 0 for a straight
    count = len(samples)
    a = samples[(j - window) % count]["Position"]
    b = samples[j]["Position"]
    c = samples[(j + window) % count]["Position"]
    ax, az, bx, bz, cx, cz = a[0], a[2], b[0], b[2], c[0], c[2]
    twice_area = (bx - ax) * (cz - az) - (bz - az) * (cx - ax)
    if abs(twice_area) < 1e-6:
        return 0.0
    ab = math.hypot(bx - ax, bz - az)
    bc = math.hypot(cx - bx, cz - bz)
    ca = math.hypot(ax - cx, az - cz)
    r = ab * bc * ca / (2.0 * abs(twice_area))
    # twice_area > 0 is a clockwise turn in x/z seen from above (+Y), which is a right turn here (x right, -z ahead)
    return -r if twice_area > 0.0 else r


# ------------------------------------------------------------------------------------------- fewer cross sections
def simplify_samples(samples, total, settings):
    """The samples the models are made of: where the track is straight, level and of one width and bank, a quad can run
    from one sample to one much further on with the same shape, so the samples between are left out. Each sample kept
    is the furthest one from the last kept whose left-out samples all stay within MeshTolerance of the quad's straight
    edges (the road's edges, the walls' tops and outer feet), at most MeshMaxLength on, and whose surface between them
    runs within MeshMaxAngle of the quad's direction: a quad that is close but tilted makes a fold at its ends, which a
    fast car feels as a bump (with 0.05 m alone, folds of up to 2.8 degrees on the circuit's crests). Where the bank
    changes, the quad's twist (road_twist) keeps its fold across the track within MeshMaxFold. A sample where
    something changes is always kept: a chunk's start, a curb stripe's, a change of walls, curbs or the curbs' side.
    Returns indices."""
    tolerance = settings["MeshTolerance"]
    max_angle = math.radians(settings["MeshMaxAngle"])
    max_fold = math.radians(settings["MeshMaxFold"])
    max_length = settings["MeshMaxLength"]
    chunk_length = settings["ChunkLength"]
    stripe = settings["CurbStripe"]
    curb_w = settings["CurbWidth"]
    wall_t = settings["WallThickness"]
    slab = settings["SlabThickness"]
    count = len(samples)

    def profile(sample):
        p, r, u = sample["Position"], sample["Right"], sample["Up"]
        hw = 0.5 * sample["Width"]
        def at(x, y): return add(p, add(mul(r, x), mul(u, y)))
        points = []
        for side, wall in ((-1.0, sample["WallLeft"]), (1.0, sample["WallRight"])):
            points += [at(side * hw, 0.0), at(side * (hw + wall_t), wall), at(side * (hw + wall_t), -slab)]
            if sample["Curbs"]:
                points.append(at(side * (hw - curb_w), 0.0))
        return points
    profiles = [profile(sm) for sm in samples]
    distance = [sm["s"] for sm in samples] + [total]   # index count: the first sample again, at the end of the loop

    def must_keep(k):
        a, b = samples[k - 1], samples[k]
        return (int(a["s"] / chunk_length) != int(b["s"] / chunk_length) or a["Walls"] != b["Walls"] or
                a["Curbs"] != b["Curbs"] or a["Supports"] != b["Supports"] or
                (b["Curbs"] and (a["Inner"] != b["Inner"] or int(a["s"] / stripe) != int(b["s"] / stripe))))

    def fits(i, j):
        first, last = profiles[i], profiles[j % count]
        span = distance[j] - distance[i]
        # Where the bank changes, a long quad is twisted: its fold across the track (the car rolls) within MeshMaxFold.
        # The fold along it is the road's strips' (build_track.py)
        width = 0.5 * (samples[i]["Width"] + samples[j % count]["Width"])
        if abs(road_twist(samples[i], samples[j % count])) / width > max_fold:
            return False
        for k in range(i + 1, j):
            t = (distance[k] - distance[i]) / span
            for a, b, c in zip(first, last, profiles[k]):
                if length(sub(c, add(a, mul(sub(b, a), t)))) > tolerance:
                    return False
        # The surface's slope along the track, sample to sample, against the quad's: measured against each sample's up, so
        # a curve's turn doesn't count (the wheels feel a fold in the slope, not the facets of a bend)
        chords = [normalize(sub(b, a)) for a, b in zip(first, last)]
        for k in range(i, j):
            up = samples[k]["Up"]
            for chord, c, d in zip(chords, profiles[k], profiles[(k + 1) % count]):
                if abs(math.asin(max(-1.0, min(1.0, dot(chord, up)))) - math.asin(max(-1.0, min(1.0, dot(normalize(sub(d, c)), up))))) > max_angle:
                    return False
        return True

    kept = [0]
    i = 0
    while i < count:
        j = i + 1
        while j < count and not must_keep(j) and distance[j + 1] - distance[i] <= max_length and fits(i, j + 1):
            j += 1
        if j < count:
            kept.append(j)
        i = j
    return kept
