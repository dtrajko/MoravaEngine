"""Generates the shooter's weapon sounds (docs/games/Shooter_Plan.html, Phases 2.2 and 2.3), all synthesized here, no samples:

    Rifle_Shot_1.wav .. _3.wav  the assault rifle's shot: a sharp crack, the muzzle's blast, a low thump, the action's
                                click and an outdoor tail (two slap echoes off buildings), a touch of tape saturation;
                                three variants (the blast's color, the echoes) so automatic fire doesn't repeat
    Rifle_Empty.wav             the trigger on an empty magazine: a dry double click
    Rifle_MagOut.wav            the magazine out: the catch's click, the magazine sliding out
    Rifle_MagIn.wav             a new magazine in: a slide, a firm clack, then the charging handle (back and forward)
    Rifle_FireMode.wav          the selector: a small detent click
    Impact_Concrete_1.wav .. _3 a bullet into concrete: a hard snap, grit and chips falling, a dull knock
    Impact_Metal_1.wav .. _3    a bullet into sheet metal: a clank and the plate's ring (bell-like partials, not in tune)
    Impact_Dirt_1.wav, _2       a bullet into the ground: a soft thud and earth scattering
    Impact_Flesh_1.wav, _2      a bullet into a body: a dull, wet-less thump (the training targets, the guards)
    HitMarker.wav               the hit marker's tick: a short, bright click the player hears on a hit

Usage: python GenerateWeaponSounds.py [output folder]     (needs numpy; the default is the shooter's Assets/Audio)

16-bit mono at 44.1 kHz. The result is the same on every run (fixed random seeds).
"""
import math
import os
import sys
import wave

import numpy as np

RATE = 44100


def samples(seconds):
    return int(round(seconds * RATE))


def t_of(seconds):
    return np.arange(samples(seconds)) / RATE


def noise(rng, seconds):
    return rng.standard_normal(int(round(seconds * RATE)))


def decay(n, seconds, attack=0.0005):
    """A short attack, then an exponential decay with this time constant"""
    e = np.exp(-np.arange(n) / RATE / max(seconds, 1e-5))
    a = max(1, int(attack * RATE))
    e[:a] *= np.linspace(0.0, 1.0, a)
    return e


def biquad(x, kind, freq, q=0.707):
    """An RBJ biquad: "low", "high" or "band" (constant peak gain)"""
    w = 2.0 * math.pi * freq / RATE
    alpha = math.sin(w) / (2.0 * q)
    c = math.cos(w)
    if kind == "low":
        b = [(1 - c) / 2, 1 - c, (1 - c) / 2]
    elif kind == "high":
        b = [(1 + c) / 2, -(1 + c), (1 + c) / 2]
    else:
        b = [alpha, 0.0, -alpha]
    a = [1 + alpha, -2 * c, 1 - alpha]
    b = [v / a[0] for v in b]
    a1, a2 = a[1] / a[0], a[2] / a[0]
    y = np.zeros_like(x)
    x1 = x2 = y1 = y2 = 0.0
    for i, v in enumerate(x):
        out = b[0] * v + b[1] * x1 + b[2] * x2 - a1 * y1 - a2 * y2
        x2, x1 = x1, v
        y2, y1 = y1, out
        y[i] = out
    return y


def mix(length, *parts):
    """Parts (start seconds, samples, gain) into one buffer of length seconds"""
    out = np.zeros(int(round(length * RATE)))
    for start, samples, gain in parts:
        i = int(round(start * RATE))
        n = min(len(samples), len(out) - i)
        out[i:i + n] += samples[:n] * gain
    return out


def saturate(x, drive=1.6):
    """Tape-like soft clipping (tanh), the level kept"""
    return np.tanh(x * drive) / math.tanh(drive)


def fade_out(x, seconds=0.01):
    n = min(len(x), int(seconds * RATE))
    x[-n:] *= np.linspace(1.0, 0.0, n)
    return x


def normalize(samples, peak):
    return samples * (peak / max(np.max(np.abs(samples)), 1e-9))


def save(folder, name, samples):
    path = os.path.join(folder, name)
    data = (np.clip(samples, -1.0, 1.0) * 32767.0).astype('<i2')
    with wave.open(path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(data.tobytes())
    print(f'{path}: {len(samples) / RATE:.2f} s')


def click(rng, freq, seconds=0.012, q=4.0):
    """A metal click: a burst of noise rung through a narrow band"""
    burst = noise(rng, seconds) * decay(samples(seconds), seconds / 5.0)
    return biquad(burst, "band", freq, q)


def rifle_shot(variant):
    rng = np.random.default_rng(100 + variant)
    length = 1.4
    # The crack: the supersonic snap, very short and bright
    crack = biquad(noise(rng, 0.006), "high", 2500.0) * decay(samples(0.006), 0.0012)
    # The blast: broadband noise, darker as it dies (two bands mixed), its color a little different in each variant
    blast_n = noise(rng, 0.25)
    bright = biquad(blast_n, "low", 5200.0 + 500.0 * variant) * decay(len(blast_n), 0.018)
    dark = biquad(blast_n, "low", 900.0 + 120.0 * variant) * decay(len(blast_n), 0.055)
    # The thump: a sine falling from 150 to 48 Hz (the chest's punch)
    t = t_of(0.22)
    freq = 48.0 + 102.0 * np.exp(-t / 0.025)
    thump = np.sin(2 * np.pi * np.cumsum(freq) / RATE) * decay(len(t), 0.05, attack=0.001)
    # The action: the bolt carrier's click right after
    action = click(rng, 3400.0 + 200.0 * variant, 0.015, 5.0)
    # The tail: the shot heard back from the yard, low-passed noise dying slowly, and two slap echoes off walls
    tail_n = noise(rng, 1.2)
    tail = biquad(tail_n, "low", 1400.0) * decay(len(tail_n), 0.28, attack=0.02)
    body = mix(0.3, (0.0, crack, 0.9), (0.0, bright, 1.0), (0.0, dark, 0.9), (0.0, thump, 0.8), (0.012, action, 0.25))
    echo = biquad(body, "low", 2200.0)
    first, second = 0.11 + 0.02 * variant, 0.23 + 0.03 * variant
    out = mix(length, (0.0, body, 1.0), (0.03, tail, 0.22), (first, echo, 0.22), (second, echo, 0.12))
    return fade_out(normalize(saturate(normalize(out, 1.0), 1.8), 0.9), 0.05)


def rifle_empty():
    rng = np.random.default_rng(201)
    return normalize(mix(0.12, (0.0, click(rng, 2600.0, 0.01, 6.0), 1.0), (0.035, click(rng, 3900.0, 0.008, 7.0), 0.6)), 0.6)


def scrape(rng, seconds, low, high):
    """A metal part sliding against another: band-limited noise with a rough, uneven level"""
    n = noise(rng, seconds)
    band = biquad(biquad(n, "high", low), "low", high)
    rough = 0.6 + 0.4 * np.abs(biquad(noise(rng, seconds), "low", 40.0)) * 8.0
    env = np.sin(np.linspace(0.0, math.pi, len(n))) ** 0.7
    return band * np.clip(rough, 0.0, 1.5) * env


def rifle_mag_out():
    rng = np.random.default_rng(202)
    out = mix(0.4, (0.0, click(rng, 2300.0, 0.014, 5.0), 1.0), (0.03, scrape(rng, 0.12, 900.0, 3500.0), 0.5))
    return normalize(out, 0.6)


def rifle_mag_in():
    rng = np.random.default_rng(203)
    clack = mix(0.03, (0.0, click(rng, 1700.0, 0.03, 3.0), 1.0), (0.0, click(rng, 3000.0, 0.02, 4.0), 0.6))
    thud = biquad(noise(rng, 0.05), "low", 300.0) * decay(samples(0.05), 0.012)
    back = click(rng, 2800.0, 0.02, 4.0)
    forward = mix(0.04, (0.0, click(rng, 2000.0, 0.03, 3.0), 1.0),
                  (0.0, biquad(noise(rng, 0.04), "low", 500.0) * decay(samples(0.04), 0.01), 0.8))
    out = mix(0.75, (0.0, scrape(rng, 0.09, 700.0, 3000.0), 0.45), (0.09, clack, 1.0), (0.09, thud, 0.8),
              (0.38, scrape(rng, 0.07, 1200.0, 4000.0), 0.3), (0.45, back, 0.7), (0.56, forward, 1.0))
    return normalize(out, 0.7)


def rifle_fire_mode():
    rng = np.random.default_rng(204)
    return normalize(mix(0.06, (0.0, click(rng, 3200.0, 0.008, 8.0), 1.0)), 0.45)


def impact_concrete(variant):
    rng = np.random.default_rng(300 + variant)
    snap = biquad(noise(rng, 0.008), "band", 3800.0 + 400.0 * variant, 1.2) * decay(samples(0.008), 0.0018)
    knock_t = t_of(0.05)
    knock = np.sin(2 * np.pi * (180.0 + 25.0 * variant) * knock_t) * decay(len(knock_t), 0.012, attack=0.0008)
    grit = scrape(rng, 0.22, 1200.0, 6500.0) * decay(samples(0.22), 0.07)
    # A few chips landing a little after
    chips = [(0.05 + rng.uniform(0.0, 0.12), click(rng, rng.uniform(2500.0, 5000.0), 0.006, 3.0), rng.uniform(0.15, 0.35)) for _ in range(4)]
    out = mix(0.32, (0.0, snap, 1.0), (0.0, knock, 0.5), (0.004, grit, 0.55), *chips)
    return fade_out(normalize(saturate(out, 1.4), 0.8), 0.03)


def impact_metal(variant):
    rng = np.random.default_rng(310 + variant)
    clank = biquad(noise(rng, 0.01), "high", 1500.0) * decay(samples(0.01), 0.002)
    t = t_of(0.45)
    ring = np.zeros(len(t))
    base = 1650.0 + 230.0 * variant
    for ratio, gain, tau in ((1.0, 1.0, 0.11), (1.59, 0.6, 0.08), (2.31, 0.45, 0.06), (3.17, 0.25, 0.04)):
        ring += gain * np.sin(2 * np.pi * base * ratio * t + rng.uniform(0, 6.28)) * decay(len(t), tau, attack=0.0005)
    out = mix(0.45, (0.0, clank, 1.0), (0.0, ring, 0.45))
    return fade_out(normalize(out, 0.75), 0.04)


def impact_dirt(variant):
    rng = np.random.default_rng(320 + variant)
    thud = biquad(noise(rng, 0.08), "low", 380.0 + 60.0 * variant) * decay(samples(0.08), 0.018)
    scatter = biquad(biquad(noise(rng, 0.25), "high", 500.0), "low", 2400.0) * decay(samples(0.25), 0.06, attack=0.004)
    out = mix(0.3, (0.0, thud, 1.0), (0.006, scatter, 0.45))
    return fade_out(normalize(out, 0.7), 0.03)


def impact_flesh(variant):
    rng = np.random.default_rng(330 + variant)
    t = t_of(0.12)
    freq = 70.0 + 60.0 * np.exp(-t / 0.015) + 8.0 * variant
    body = np.sin(2 * np.pi * np.cumsum(freq) / RATE) * decay(len(t), 0.03, attack=0.001)
    slap = biquad(noise(rng, 0.03), "low", 900.0 + 150.0 * variant) * decay(samples(0.03), 0.007)
    cloth = biquad(noise(rng, 0.06), "band", 2200.0, 0.8) * decay(samples(0.06), 0.012)
    out = mix(0.16, (0.0, body, 1.0), (0.0, slap, 0.8), (0.0, cloth, 0.2))
    return fade_out(normalize(out, 0.75), 0.02)


def hit_marker():
    rng = np.random.default_rng(340)
    return fade_out(normalize(mix(0.07, (0.0, click(rng, 4400.0, 0.012, 9.0), 1.0), (0.004, click(rng, 6100.0, 0.008, 9.0), 0.5)), 0.5), 0.01)


def main():
    root = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    folder = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "Projects", "Outpost", "Assets", "Audio")
    os.makedirs(folder, exist_ok=True)
    for variant in (1, 2, 3):
        save(folder, f"Rifle_Shot_{variant}.wav", rifle_shot(variant))
    save(folder, "Rifle_Empty.wav", rifle_empty())
    save(folder, "Rifle_MagOut.wav", rifle_mag_out())
    save(folder, "Rifle_MagIn.wav", rifle_mag_in())
    save(folder, "Rifle_FireMode.wav", rifle_fire_mode())
    for variant in (1, 2, 3):
        save(folder, f"Impact_Concrete_{variant}.wav", impact_concrete(variant))
        save(folder, f"Impact_Metal_{variant}.wav", impact_metal(variant))
    for variant in (1, 2):
        save(folder, f"Impact_Dirt_{variant}.wav", impact_dirt(variant))
        save(folder, f"Impact_Flesh_{variant}.wav", impact_flesh(variant))
    save(folder, "HitMarker.wav", hit_marker())


if __name__ == "__main__":
    main()
