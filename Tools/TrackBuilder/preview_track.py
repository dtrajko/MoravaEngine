# A track's preview without Blender or the engine (plain Python 3, no packages):
#
#   python Tools/TrackBuilder/preview_track.py Projects/Racing/Tracks/Circuit.track.json [out.svg]
#
# Prints the length, the heights, the grades and each turn (its tightest radius, bank, a rough speed), and a rough lap
# time; writes an SVG (beside the data file unless given): the top view colored by height, with the point numbers, and the
# height profile along the track. For laying out a track before building it; the test driver's laps are what count.
#
# The lap time is a point-mass estimate: the car's top speed, its grip in a turn (with the bank and downforce), power and
# brakes, in two passes (accelerating forward, braking backward). The numbers are the racing car's, rough; tuned so the
# test track's flying lap comes out near the test driver's (25.3 s against 25.9).

import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from track_spline import load_track, sample_track, turn_radius  # noqa: E402

G = 9.81
TOP_SPEED = 97.0        # m/s (350 km/h)
GRIP = 1.0              # lateral friction, flat (the arcade grip, with the test driver's caution)
DOWNFORCE = 0.00012     # extra grip per (m/s)^2
POWER_PER_MASS = 660.0  # W/kg (about 530 kW, 800 kg)
TRACTION = 14.0         # m/s^2 at low speed (0-100 km/h in about 2 s)
BRAKE = 14.0            # m/s^2


def corner_speed(radius, bank_deg):
    if radius == 0.0:
        return TOP_SPEED
    r = abs(radius)
    tb = math.tan(math.radians(abs(bank_deg)))
    # v^2 / r = g (mu + tan b) / (1 - mu tan b), mu growing with speed (downforce): solve by iteration
    v = TOP_SPEED
    for _ in range(20):
        mu = GRIP + DOWNFORCE * v * v
        a = G * (mu + tb) / max(1.0 - mu * tb, 0.6)
        v = min(TOP_SPEED, math.sqrt(a * r))
    return v


def lap_time(samples, total):
    count = len(samples)
    step = total / count
    limit = []
    for j, s in enumerate(samples):
        radius = turn_radius(samples, j, 6)
        # a banked turn helps only when it leans into the turn (a positive bank for a right turn, negative for a left)
        leans_in = (radius < 0.0) == (s["Bank"] > 0.0)
        limit.append(corner_speed(radius, abs(s["Bank"])) if leans_in else corner_speed(radius, 0.0) * 0.95)
    v = list(limit)
    # forward: accelerating from each turn, twice around so the start isn't a standstill
    for _ in range(2):
        for j in range(count):
            prev = v[j - 1]
            grade = samples[j]["Tangent"][1]
            accel = min(TRACTION, POWER_PER_MASS / max(prev, 1.0)) * (1.0 - (prev / TOP_SPEED) ** 3) - G * grade
            v[j] = min(limit[j], math.sqrt(max(prev * prev + 2.0 * accel * step, 1.0)))
    for _ in range(2):
        for j in range(count - 1, -1, -1):
            nxt = v[(j + 1) % count]
            v[j] = min(v[j], math.sqrt(nxt * nxt + 2.0 * BRAKE * step))
    time = sum(step / max(x, 1.0) for x in v)
    return time, v, limit


def turns(samples, speeds):
    # Stretches tighter than 400 m: the tightest radius in each, its bank, the slowest estimated speed
    result = []
    current = None
    for j, s in enumerate(samples):
        r = turn_radius(samples, j, 6)
        tight = r != 0.0 and abs(r) < 400.0
        if tight:
            if current is None or ((r < 0.0) != current["right"]):
                if current is not None:
                    result.append(current)
                current = {"start": s["s"], "end": s["s"], "right": r < 0.0, "radius": abs(r), "bank": s["Bank"],
                           "speed": speeds[j], "point": s["Point"]}
            else:
                current["end"] = s["s"]
                if abs(r) < current["radius"]:
                    current["radius"], current["bank"] = abs(r), s["Bank"]
                current["speed"] = min(current["speed"], speeds[j])
        elif current is not None:
            result.append(current)
            current = None
    if current is not None:
        result.append(current)
    return [t for t in result if t["end"] - t["start"] > 8.0]


def color_of_height(t):
    # low blue -> green -> high orange
    stops = [(0.0, (40, 90, 200)), (0.5, (60, 170, 90)), (1.0, (230, 120, 30))]
    for (t0, c0), (t1, c1) in zip(stops, stops[1:]):
        if t <= t1:
            f = (t - t0) / (t1 - t0)
            return "rgb(%d,%d,%d)" % tuple(int(a + (b - a) * f) for a, b in zip(c0, c1))
    return "rgb(%d,%d,%d)" % stops[-1][1]


def write_svg(path, name, points, samples, total, speeds):
    xs = [s["Position"][0] for s in samples]
    zs = [s["Position"][2] for s in samples]
    ys = [s["Position"][1] for s in samples]
    lo, hi = min(ys), max(ys)
    margin = 40.0
    min_x, max_x, min_z, max_z = min(xs) - margin, max(xs) + margin, min(zs) - margin, max(zs) + margin
    scale = 900.0 / max(max_x - min_x, max_z - min_z)
    w = (max_x - min_x) * scale
    h = (max_z - min_z) * scale
    prof_h = 220.0
    out = []
    out.append('<svg xmlns="http://www.w3.org/2000/svg" width="%.0f" height="%.0f" font-family="Segoe UI, sans-serif" font-size="12">'
               % (max(w, 900) + 20, h + prof_h + 90))
    out.append('<rect width="100%" height="100%" fill="#f4f4f0"/>')
    out.append('<text x="10" y="18" font-size="15" font-weight="bold">%s: %.0f m, %.1f m from the lowest to the highest point (top view, -Z up the page)</text>'
               % (name, total, hi - lo))

    def px(x, z):
        return 10 + (x - min_x) * scale, 30 + (z - min_z) * scale

    # 100 m grid
    for gx in range(int(min_x // 100) * 100, int(max_x) + 100, 100):
        a = px(gx, min_z)
        b = px(gx, max_z)
        out.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="#ddd"/>' % (a[0], a[1], b[0], b[1]))
    for gz in range(int(min_z // 100) * 100, int(max_z) + 100, 100):
        a = px(min_x, gz)
        b = px(max_x, gz)
        out.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="#ddd"/>' % (a[0], a[1], b[0], b[1]))
    # the road, its width to scale, colored by height
    count = len(samples)
    for j in range(0, count, 2):
        a = samples[j]
        b = samples[(j + 2) % count]
        p, q = px(a["Position"][0], a["Position"][2]), px(b["Position"][0], b["Position"][2])
        t = (a["Position"][1] - lo) / max(hi - lo, 1e-6)
        out.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" stroke-width="%.1f" stroke-linecap="round"/>'
                   % (p[0], p[1], q[0], q[1], color_of_height(t), max(a["Width"] * scale, 2.0)))
    # the start line and the direction
    s0 = samples[0]
    p = px(s0["Position"][0], s0["Position"][2])
    r = s0["Right"]
    hw = 0.5 * s0["Width"] + 4
    a = px(s0["Position"][0] - r[0] * hw, s0["Position"][2] - r[2] * hw)
    b = px(s0["Position"][0] + r[0] * hw, s0["Position"][2] + r[2] * hw)
    out.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="black" stroke-width="3"/>' % (a[0], a[1], b[0], b[1]))
    s20 = samples[min(40, count - 1)]
    q = px(s20["Position"][0], s20["Position"][2])
    out.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="black" stroke-width="1.5" marker-end="url(#arrow)"/>' % (p[0], p[1], q[0], q[1]))
    out.insert(1, '<defs><marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="8" markerHeight="8" orient="auto">'
                  '<path d="M0,0 L10,5 L0,10 z"/></marker></defs>')
    # the control points
    for i, point in enumerate(points):
        x, y, z = point["Position"]
        c = px(x, z)
        out.append('<circle cx="%.1f" cy="%.1f" r="3" fill="black"/><text x="%.1f" y="%.1f" fill="#333">%d</text>'
                   % (c[0], c[1], c[0] + 5, c[1] - 5, i))

    # the profile: height (and the estimated speed) along the track
    top = 30 + h + 40
    left, right = 50.0, max(w, 900) - 10
    def prof(sv, yv):
        return left + sv / total * (right - left), top + prof_h - (yv - lo) / max(hi - lo, 1e-6) * prof_h
    out.append('<text x="10" y="%.0f" font-size="14" font-weight="bold">Height along the track (black, %.0f to %.0f m) and the rough speed (red, 0 to 360 km/h)</text>' % (top - 12, lo, hi))
    out.append('<rect x="%.0f" y="%.0f" width="%.0f" height="%.0f" fill="white" stroke="#bbb"/>' % (left, top, right - left, prof_h))
    pts = " ".join("%.1f,%.1f" % prof(s["s"], s["Position"][1]) for s in samples[::2])
    out.append('<polyline points="%s" fill="none" stroke="black" stroke-width="2"/>' % pts)
    sp = " ".join("%.1f,%.1f" % (left + s["s"] / total * (right - left), top + prof_h - v * 3.6 / 360.0 * prof_h)
                  for s, v in list(zip(samples, speeds))[::2])
    out.append('<polyline points="%s" fill="none" stroke="#c33" stroke-width="1.2"/>' % sp)
    for d in range(0, int(total) + 1, 250):
        x = left + d / total * (right - left)
        out.append('<line x1="%.1f" y1="%.0f" x2="%.1f" y2="%.0f" stroke="#bbb"/><text x="%.1f" y="%.0f" text-anchor="middle">%d m</text>'
                   % (x, top + prof_h, x, top + prof_h + 5, x, top + prof_h + 18, d))
    out.append('</svg>')
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out))


def main():
    if len(sys.argv) < 2:
        print("preview_track.py: give the track's data file")
        sys.exit(1)
    track_path = os.path.abspath(sys.argv[1])
    data, settings, points = load_track(track_path)
    samples, total = sample_track(points, settings["SampleSpacing"])
    time, speeds, _ = lap_time(samples, total)
    ys = [s["Position"][1] for s in samples]
    grades = [s["Tangent"][1] / max(math.hypot(s["Tangent"][0], s["Tangent"][2]), 1e-6) * 100.0 for s in samples]
    print("%s: %.0f m, heights %.1f to %.1f (%.1f m apart), steepest climb %.1f %%, steepest drop %.1f %%"
          % (data["Name"], total, min(ys), max(ys), max(ys) - min(ys), max(grades), -min(grades)))
    print("rough lap %.1f s (average %.0f km/h, top %.0f km/h)" % (time, total / time * 3.6, max(speeds) * 3.6))
    for t in turns(samples, speeds):
        print("  %5.0f-%5.0f m (from point %2d): %-5s radius %4.0f m, bank %5.1f, about %3.0f km/h"
              % (t["start"], t["end"], t["point"], "right" if t["right"] else "left", t["radius"], t["bank"], t["speed"] * 3.6))
    svg = sys.argv[2] if len(sys.argv) > 2 else os.path.splitext(track_path)[0].replace(".track", "") + ".preview.svg"
    write_svg(svg, data["Name"], points, samples, total, speeds)
    print("wrote %s" % svg)


main()
