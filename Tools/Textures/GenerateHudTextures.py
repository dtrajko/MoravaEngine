"""Generates the shooter's HUD textures (docs/games/Shooter_Plan.html, Phases 3.1 and 3.2: health, the death screen, where
hits come from), made here, no images:

    DamageVignette.png  the red at the screen's edges after a hit (the HUD tints it and fades it): white, its alpha 0 in
                        the middle and rising toward the edges and more into the corners, with a soft, uneven inner edge
                        (a few slow waves round it) so it reads as a glow, not a frame. It is stretched over the screen.
    DamageArc.png       Phase 3.2: the direction a hit came from, a piece of a ring round the crosshair (ARC_RADIUS
                        pixels from its middle, ARC_HALF_ANGLE each way), white, brightest in its middle and fading to its
                        ends, soft across. Drawn at its own size (256 x 64 on the 1080 canvas), the ring's middle is
                        ARC_CENTER_BELOW pixels below the picture's middle: the HUD puts it that far from the crosshair,
                        turned toward the hit.

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


ARC_W, ARC_H = 256, 64
ARC_RADIUS = 220.0       # pixels from the ring's middle to the arc's middle line
ARC_THICK = 6.0          # half its thickness (full alpha), and the soft edge beyond it
ARC_SOFT = 4.0
ARC_HALF_ANGLE = 28.0    # degrees each way from its middle
ARC_TOP = 6.0            # the arc's outer edge at its middle, below the picture's top
ARC_CENTER_Y = ARC_TOP + ARC_THICK + ARC_SOFT + ARC_RADIUS  # the ring's middle, from the picture's top
ARC_CENTER_BELOW = ARC_CENTER_Y - ARC_H / 2.0              # ... from the picture's middle (the HUD's distance)


def damage_arc():
    x = np.arange(ARC_W) + 0.5 - ARC_W / 2.0
    y = np.arange(ARC_H) + 0.5
    x, y = np.meshgrid(x, y)
    dy = ARC_CENTER_Y - y  # up from the ring's middle
    r = np.sqrt(x * x + dy * dy)
    angle = np.degrees(np.arctan2(x, dy))
    across = 1.0 - smoothstep(ARC_THICK, ARC_THICK + ARC_SOFT, np.abs(r - ARC_RADIUS))
    along = 1.0 - smoothstep(0.35 * ARC_HALF_ANGLE, ARC_HALF_ANGLE, np.abs(angle))
    # A little thicker in the middle: the outer side grows toward it
    a = across * along ** 1.2
    rgba = np.zeros((ARC_H, ARC_W, 4), dtype=np.uint8)
    rgba[..., :3] = 255
    rgba[..., 3] = (np.clip(a, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)
    return rgba


def main():
    root = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "Projects", "Outpost", "Assets", "Textures", "UI")
    os.makedirs(out, exist_ok=True)
    for name, image in (("DamageVignette.png", damage_vignette()), ("DamageArc.png", damage_arc())):
        path = os.path.join(out, name)
        write_png(path, image)
        print(f"wrote {path}")
    print(f"DamageArc: the ring's middle {ARC_CENTER_BELOW:.1f} pixels below the picture's middle")


if __name__ == "__main__":
    main()
