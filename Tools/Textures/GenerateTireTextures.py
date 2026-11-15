"""Generates the racing game's tire effect textures (docs/games/Racing_Plan.html, Phases 11.4 and 11.5), made here, no images:

    TireSmoke.png   a puff of smoke: a cloudy round shape in its alpha, a little light and shade in its color
    SkidMark.png    a tire's mark along the road: streaks of rubber along it (the tread), soft at its two edges

Usage: python GenerateTireTextures.py [output folder]     (needs numpy; the default is the racing project's Assets/Textures)

RGBA, near white: the particle (ParticleEmitter's ColorStart/ColorEnd) and the trail (Trail's Color) multiply it with their
own color. The results are the same on every run (fixed random seeds).
"""

import os
import struct
import sys
import zlib

import numpy as np

SIZE = 256


def value_noise(size, cells, rng):
    """Smooth noise of 'cells' cells across, tiling: random values on a grid, interpolated with a smoothstep."""
    grid = rng.random((cells, cells))
    t = np.arange(size) * cells / size
    i0 = np.floor(t).astype(int)
    f = t - i0
    f = f * f * (3.0 - 2.0 * f)
    i1 = (i0 + 1) % cells
    i0 %= cells
    # Rows first, then columns
    a = grid[i0][:, i0] * (1 - f)[None, :] + grid[i0][:, i1] * f[None, :]
    b = grid[i1][:, i0] * (1 - f)[None, :] + grid[i1][:, i1] * f[None, :]
    return a * (1 - f)[:, None] + b * f[:, None]


def fbm(size, rng, octaves=5, base=4):
    """Noise of several scales together, each half as strong as the one before: billows."""
    total = np.zeros((size, size))
    amplitude, weight = 1.0, 0.0
    for o in range(octaves):
        total += amplitude * value_noise(size, base << o, rng)
        weight += amplitude
        amplitude *= 0.5
    return total / weight


def write_png(path, rgba):
    """An 8-bit RGBA PNG, written with zlib only."""
    height, width, _ = rgba.shape
    raw = b"".join(b"\x00" + rgba[y].tobytes() for y in range(height))

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
        f.write(chunk(b"IEND", b""))


def smoke(rng):
    y, x = (np.mgrid[0:SIZE, 0:SIZE] + 0.5) / SIZE * 2.0 - 1.0
    # The edge pushed in and out by the noise, so the puff isn't a circle
    shape = fbm(SIZE, rng, octaves=3, base=3)
    r = np.sqrt(x * x + y * y) / 0.92 * (1.0 + 0.5 * (shape - 0.5))
    falloff = np.clip(1.0 - r, 0.0, 1.0)
    falloff = falloff * falloff * (3.0 - 2.0 * falloff)
    # Billows inside: thicker and thinner smoke
    billows = fbm(SIZE, rng, octaves=5, base=4)
    density = falloff * np.clip(0.75 + 1.4 * (billows - 0.5), 0.0, 1.0)
    # Never anything at the border (the quad's edge must not show)
    border = np.clip((1.0 - np.maximum(np.abs(x), np.abs(y))) * 12.0, 0.0, 1.0)
    alpha = np.clip(density * border, 0.0, 1.0)
    # Light from above: the upper side a little brighter, the thick parts a little darker
    shade = np.clip(0.88 - 0.06 * y - 0.08 * (billows - 0.5), 0.0, 1.0)
    rgba = np.zeros((SIZE, SIZE, 4))
    rgba[..., 0] = rgba[..., 1] = rgba[..., 2] = shade
    rgba[..., 3] = alpha
    return (rgba * 255.0 + 0.5).astype(np.uint8)


def skid(rng):
    """256 along the mark (x: a trail's TileLength) by 128 across it (y). Streaks run along: each row across has its own
    amount of rubber, the tread's grooves lighter; the amount wanders a little along, so the mark isn't ruled; it tiles."""
    along, across = 256, 128
    y = (np.arange(across) + 0.5) / across
    # The tread: four grooves with less rubber, and fine streaks of random strength
    grooves = 1.0 - 0.55 * np.exp(-((np.mod(y * 5.0, 1.0) - 0.5) ** 2) / (2 * 0.035 ** 2))
    grooves[(y < 0.1) | (y > 0.9)] = 1.0
    fine = np.repeat(rng.random(across // 2), 2)
    fine = 0.7 + 0.3 * np.convolve(np.r_[fine[-2:], fine, fine[:2]], np.ones(5) / 5, mode="same")[2:-2]
    rows = grooves * fine
    # Along: slow changes, as the slide pressed harder or lighter (tiling noise, one dimension stretched)
    wander = value_noise(along, 4, rng)[:across // 4 if across // 4 else 1].mean(axis=0)
    wander = 0.8 + 0.4 * (wander - wander.min()) / max(np.ptp(wander), 1e-6)
    grain = 0.9 + 0.1 * fbm(along, rng, octaves=4, base=8)[:across, :]
    alpha = rows[:, None] * wander[None, :] * grain
    # Soft edges: nothing at the two sides, full a fifth in
    edge = np.clip(np.minimum(y, 1.0 - y) / 0.2, 0.0, 1.0)
    edge = edge * edge * (3.0 - 2.0 * edge)
    alpha *= edge[:, None]
    rgba = np.ones((across, along, 4))
    rgba[..., 3] = np.clip(alpha, 0.0, 1.0)
    return (rgba * 255.0 + 0.5).astype(np.uint8)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    default = os.path.join(here, "..", "..", "Projects", "Racing", "Assets", "Textures")
    folder = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else default)
    os.makedirs(folder, exist_ok=True)
    for name, make, seed in (("TireSmoke.png", smoke, 11), ("SkidMark.png", skid, 12)):
        path = os.path.join(folder, name)
        write_png(path, make(np.random.default_rng(seed)))
        print(path)


if __name__ == "__main__":
    main()
