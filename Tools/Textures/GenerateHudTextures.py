"""Generates the shooter's HUD textures (docs/games/Shooter_Plan.html, Phase 3.1: health and the death screen), made
here, no images:

    DamageVignette.png  the red at the screen's edges after a hit (the HUD tints it and fades it): white, its alpha 0 in
                        the middle and rising toward the edges and more into the corners, with a soft, uneven inner edge
                        (a few slow waves round it) so it reads as a glow, not a frame. It is stretched over the screen.

Usage: python GenerateHudTextures.py [output folder]   (needs numpy; the default is the shooter project's
Assets/Textures/UI)

The result is the same on every run.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from GenerateTireTextures import write_png  # noqa: E402  (the same PNG writer)

SIZE = 512
CLEAR = 0.55      # the middle stays clear out to this (of the half size, along the axes)
EDGE = 1.12       # full at this distance (beyond the edge's middle: the corners are fuller)
SQUARENESS = 3.0  # how far the rounded rectangle's distance leans toward a square's (2: an ellipse)
WAVES = 0.06      # how much the inner edge wavers (of the half size)
MOST = 0.9        # the alpha at the edge


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def damage_vignette():
    c = (np.arange(SIZE) + 0.5) / SIZE * 2.0 - 1.0
    x, y = np.meshgrid(c, c)
    # A rounded rectangle's distance: fuller in the corners than an ellipse's
    r = (np.abs(x) ** SQUARENESS + np.abs(y) ** SQUARENESS) ** (1.0 / SQUARENESS)
    angle = np.arctan2(y, x)
    wave = WAVES * (0.6 * np.sin(5.0 * angle + 0.7) + 0.4 * np.sin(9.0 * angle + 2.1))
    a = smoothstep(CLEAR + wave, EDGE, r) ** 1.5 * MOST
    rgba = np.zeros((SIZE, SIZE, 4), dtype=np.uint8)
    rgba[..., :3] = 255
    rgba[..., 3] = (np.clip(a, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)
    return rgba


def main():
    root = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "Projects", "Outpost", "Assets", "Textures", "UI")
    os.makedirs(out, exist_ok=True)
    path = os.path.join(out, "DamageVignette.png")
    write_png(path, damage_vignette())
    print(f"wrote {path}")


if __name__ == "__main__":
    main()
