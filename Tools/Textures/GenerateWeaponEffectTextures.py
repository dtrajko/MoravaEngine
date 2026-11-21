"""Generates the shooter's weapon effect textures (docs/games/Shooter_Plan.html, Phase 2.3), made here, no images:

    MuzzleFlash.png          a muzzle's flash seen from the side or the front: a flame star, white at its heart, yellow and
                             orange out to its ragged points (drawn added to the scene: its color times its alpha)
    BulletHole_Concrete.png  a bullet's hole in concrete or plaster: a dark hole, a chipped crater around it with a few
                             cracks, a faint ring of soot (a decal: it darkens the wall, white leaves it as it is)
    BulletHole_Metal.png     a bullet's hole in sheet metal: a smaller hole, the dent's dark ring, scorch

Usage: python GenerateWeaponEffectTextures.py [output folder]   (needs numpy; the default is the shooter project's
Assets/Textures/Effects)

RGBA. The decals' color is how dark each place gets and the alpha how much it covers; the engine multiplies them into the
lit surface (Decals.glsl), so a decal can only darken. The results are the same on every run (fixed random seeds).
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from GenerateTireTextures import fbm, write_png  # noqa: E402  (the same noise and PNG writer)

SIZE = 128


def polar(size):
    """Each texel's distance from the middle (1 at the edge's middle) and its angle."""
    c = (np.arange(size) + 0.5) / size * 2.0 - 1.0
    x, y = np.meshgrid(c, -c)
    return np.sqrt(x * x + y * y), np.arctan2(y, x)


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def to_rgba(color, alpha):
    rgba = np.concatenate([np.clip(color, 0.0, 1.0), np.clip(alpha, 0.0, 1.0)[..., None]], axis=-1)
    return (rgba * 255.0 + 0.5).astype(np.uint8)


def muzzle_flash(rng):
    r, a = polar(SIZE)
    noise = fbm(SIZE, rng, octaves=4, base=4)
    # The star: five to seven points of different lengths, their edges ragged with the noise
    reach = np.zeros_like(r)
    points = 6
    for i in range(points):
        angle = i * 2.0 * np.pi / points + rng.uniform(-0.25, 0.25)
        length = rng.uniform(0.65, 0.95)
        width = rng.uniform(0.18, 0.3)
        d = np.angle(np.exp(1j * (a - angle)))  # the angle from the point's middle, -pi..pi
        reach = np.maximum(reach, length * np.exp(-(d / width) ** 2))
    reach = np.maximum(reach, 0.32)  # the round heart
    edge = reach * (0.8 + 0.4 * noise)
    alpha = 1.0 - smoothstep(edge * 0.55, edge, r)
    # White at the heart, yellow, then orange at the points
    heat = 1.0 - smoothstep(0.0, 0.85, r / np.maximum(edge, 1e-3))
    white = np.array([1.0, 0.98, 0.9])
    yellow = np.array([1.0, 0.78, 0.32])
    orange = np.array([1.0, 0.42, 0.1])
    t = heat[..., None]
    color = np.where(t > 0.5, yellow + (white - yellow) * ((t - 0.5) * 2.0), orange + (yellow - orange) * (t * 2.0))
    alpha *= 0.55 + 0.45 * heat
    return to_rgba(color, alpha)


def bullet_hole(rng, hole, crater, soot, cracks, crater_dark, dent):
    """A hole of radius 'hole' (of the half size), a crater out to 'crater', soot fading out to 'soot'."""
    r, a = polar(SIZE)
    noise = fbm(SIZE, rng, octaves=5, base=4)
    fine = fbm(SIZE, rng, octaves=3, base=16)
    ragged = 1.0 + 0.35 * (noise - 0.5) * 2.0

    darkness = np.zeros_like(r)  # 0: as the wall, 1: black
    # The soot (or the scorch), faint and soft
    darkness = np.maximum(darkness, 0.28 * (1.0 - smoothstep(crater * 0.8, soot, r * ragged)) * (0.7 + 0.6 * fine))
    # The crater: chipped, lighter toward its rim
    in_crater = 1.0 - smoothstep(crater * 0.85, crater, r * ragged)
    crater_shade = crater_dark * (0.65 + 0.35 * (1.0 - r / crater)) * (0.75 + 0.5 * fine)
    darkness = np.maximum(darkness, in_crater * crater_shade)
    # A dent's ring (metal): darker at the rim of the crater
    if dent > 0.0:
        ring = np.exp(-((r - crater * 0.8) / (crater * 0.12)) ** 2)
        darkness = np.maximum(darkness, dent * ring)
    # Cracks: a few thin dark lines out from the crater
    for _ in range(cracks):
        angle = rng.uniform(-np.pi, np.pi)
        length = rng.uniform(crater * 1.1, soot * 0.95)
        d = np.angle(np.exp(1j * (a - angle))) + 0.15 * (noise - 0.5)
        width = 0.012 / np.maximum(r, 0.05)
        line = np.exp(-(d / width) ** 2) * (1.0 - smoothstep(length * 0.7, length, r)) * smoothstep(crater * 0.6, crater, r)
        darkness = np.maximum(darkness, 0.55 * line)
    # The hole itself: nearly black
    in_hole = 1.0 - smoothstep(hole * 0.75, hole, r * (1.0 + 0.15 * (noise - 0.5)))
    darkness = np.maximum(darkness, 0.95 * in_hole)

    # The decal's color is the darkness (a little warm: soot and dust), its alpha the coverage
    alpha = np.clip(darkness * 2.2, 0.0, 1.0) * (1.0 - smoothstep(soot * 0.9, 1.0, r))
    shade = 1.0 - darkness
    color = np.stack([shade * 1.0, shade * 0.97, shade * 0.93], axis=-1)
    return to_rgba(color, alpha)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "..", "Projects", "Outpost", "Assets", "Textures", "Effects")
    os.makedirs(out, exist_ok=True)
    textures = {
        "MuzzleFlash.png": muzzle_flash(np.random.default_rng(23)),
        "BulletHole_Concrete.png": bullet_hole(np.random.default_rng(7), hole=0.16, crater=0.45, soot=0.85, cracks=4, crater_dark=0.7, dent=0.0),
        "BulletHole_Metal.png": bullet_hole(np.random.default_rng(11), hole=0.17, crater=0.36, soot=0.75, cracks=0, crater_dark=0.4, dent=0.7),
    }
    for name, rgba in textures.items():
        path = os.path.join(out, name)
        write_png(path, rgba)
        print(f"{path}: {rgba.shape[1]} x {rgba.shape[0]}")


if __name__ == "__main__":
    main()
