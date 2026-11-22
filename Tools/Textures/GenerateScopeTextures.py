"""Generates the sniper scope's overlay (docs/games/Shooter_Plan.html, Phase 2.5: the scope), made here, no images:

    Scope.png   the view through the scope as a game UI overlay, square (the HUD shows it as high as the screen, black bars
                beside it): black outside the lens, a soft vignette toward the lens's edge, and the reticle: thin
                crosshairs, thick posts from halfway out, mil dots along the lines and range marks below the middle
                (shorter bars further down: where a far target's drop is held)

Usage: python GenerateScopeTextures.py [output folder]   (needs numpy; the default is the shooter project's
Assets/Textures/UI)

RGBA, black with the alpha saying how much it covers: the UI image draws it over the scene. Lines are antialiased by their
coverage of each texel. The result is the same on every run.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from GenerateTireTextures import write_png  # noqa: E402  (the same PNG writer)

SIZE = 1024
LENS = 0.965          # the lens's edge (of the half size): black beyond
VIGNETTE = 0.72       # the vignette starts here and darkens toward the edge
VIGNETTE_DARK = 0.6   # how dark it is at the edge (alpha)
THIN = 1.6            # the crosshairs' width in texels (about two pixels on a 1080 high screen)
POST = 9.0            # the thick posts' width in texels
POST_START = 0.52     # where the posts start (of the half size)
DOT_STEP = 0.075      # mil dots along the lines this far apart (of the half size)
DOTS = 5              # on each side
DOT_RADIUS = 3.2      # texels


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def coverage(distance, half_width):
    """How much of a texel a shape covers whose edge is this far (texels; inside negative) from it: one texel's ramp."""
    return np.clip(half_width - distance + 0.5, 0.0, 1.0)


def scope():
    texel = 2.0 / SIZE  # one texel in the half size's units
    c = (np.arange(SIZE) + 0.5) / SIZE * 2.0 - 1.0
    x, y = np.meshgrid(c, c)  # y down, as the canvas's
    r = np.sqrt(x * x + y * y)
    ax, ay = np.abs(x) / texel, np.abs(y) / texel  # texels from the lines

    alpha = np.zeros((SIZE, SIZE))
    # The vignette, then black outside the lens (its edge a few texels soft)
    alpha = np.maximum(alpha, VIGNETTE_DARK * smoothstep(VIGNETTE, LENS, r) ** 1.6)
    alpha = np.maximum(alpha, smoothstep(LENS - 3 * texel, LENS + 1 * texel, r))

    # The crosshairs, thin across the whole lens
    lines = np.maximum(coverage(ax, THIN * 0.5), coverage(ay, THIN * 0.5))
    # The posts, thick from POST_START out (left, right and below; none above: the target's view stays open)
    along_x = np.abs(x) >= POST_START
    along_y = (y >= POST_START)
    posts = np.maximum(np.where(along_x, coverage(ay, POST * 0.5), 0.0), np.where(along_y, coverage(ax, POST * 0.5), 0.0))
    # Their ends tapered a little (a point toward the middle)
    alpha = np.maximum(alpha, np.where(r < LENS, np.maximum(lines, posts), 0.0))

    # Mil dots along the four lines
    for i in range(1, DOTS + 1):
        d = i * DOT_STEP
        for cx, cy in ((d, 0.0), (-d, 0.0), (0.0, d), (0.0, -d)):
            dist = np.sqrt((x - cx) ** 2 + (y - cy) ** 2) / texel
            alpha = np.maximum(alpha, coverage(dist, DOT_RADIUS))

    # Range marks below the middle: short bars between the dots, shorter further down (100 m steps of drop)
    for i, half in enumerate((0.055, 0.045, 0.036, 0.028)):
        cy = (i + 0.5) * DOT_STEP
        bar = np.maximum(coverage(np.abs(y - cy) / texel, THIN * 0.5), 0.0) * (np.abs(x) <= half)
        alpha = np.maximum(alpha, bar)

    rgba = np.zeros((SIZE, SIZE, 4), dtype=np.uint8)
    rgba[..., 3] = (np.clip(alpha, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8)
    return rgba


def main():
    root = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "Projects", "Outpost", "Assets", "Textures", "UI")
    os.makedirs(out, exist_ok=True)
    path = os.path.join(out, "Scope.png")
    write_png(path, scope())
    print(f"wrote {path}")


if __name__ == "__main__":
    main()
