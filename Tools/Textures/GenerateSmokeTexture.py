"""Generates the racing game's tire smoke texture (docs/games/Racing_Plan.html, Phase 11.4), made here, no images:

    TireSmoke.png   a puff of smoke: a cloudy round shape in its alpha, a little light and shade in its color

Usage: python GenerateSmokeTexture.py [output folder]     (needs numpy; the default is the racing project's Assets/Textures)

256 x 256 RGBA. The particle multiplies it with its own color (ParticleEmitter's ColorStart/ColorEnd), so the color here
is near white: the emitter makes it gray. The result is the same on every run (fixed random seed).
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


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    default = os.path.join(here, "..", "..", "Projects", "Racing", "Assets", "Textures")
    folder = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else default)
    os.makedirs(folder, exist_ok=True)
    path = os.path.join(folder, "TireSmoke.png")
    write_png(path, smoke(np.random.default_rng(11)))
    print(path)


if __name__ == "__main__":
    main()
