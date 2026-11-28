"""Generates the shooter's game sounds besides the weapons' (docs/games/Shooter_Plan.html, Phase 3.1: health, pickups and
the hazards; Phase 3.2: the objectives; Phase 4.1: the doors), synthesized here, no recordings, with GenerateWeaponSounds.py's tools:

    Pickup_Health.wav           a health pack taken: the case's two plastic latches snapping open, a short rustle
    Pickup_Armor.wav            a vest taken: a velcro strap torn open and pressed shut, the buckle's click
    Pickup_Ammo.wav             an ammo box taken: the steel lid's clank and its ring, rounds and magazines rattling
    Electric_Spark_1.wav .. _3  a live wire sparking: a burst of sharp crackles with a buzzing zap under them
    Electric_Hum.wav            the electric floor's hum (looped by its AudioSource): the mains' 50 Hz and its harmonics,
                                a transformer's buzz, faint crackle; whole cycles, so the loop has no seam
    Player_Hurt.wav             the player hit: a body's dull thump and a short breathy grunt
    Player_Death.wav            the player dying: a longer grunt falling away, the body and its gear hitting the ground
    Objective_Complete.wav      Phase 3.2, an objective done: a radio's squelch, then two soft rising tones through the
                                radio's narrow band (a field radio's confirmation, not a fanfare)
    Door_Open.wav               Phase 4.1, a door opened: the handle's latch drawn back, a short creak of its hinges,
                                the leaf's air
    Door_Close.wav              a door shut: the leaf's air, a hollow wooden thud in the frame, the latch snapping in
    Door_Locked.wav             a locked door tried: the handle pressed down twice, its latch knocking on the bolt, the
                                leaf rattling a little in the frame (nothing gives)

Usage: python GenerateGameSounds.py [output folder]   (needs numpy; the default is the shooter project's Assets/Audio)

Mono, 16-bit, 44.1 kHz. The same on every run (each sound has its own seed).
"""
import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from GenerateWeaponSounds import (RATE, biquad, click, decay, fade_out, mix, noise, normalize, samples, save,  # noqa: E402
                                  saturate, scrape, t_of)


def snap(rng, freq, seconds=0.02):
    """A plastic snap: a click lower and drier than metal's, with a tick of noise ahead of it"""
    tick = biquad(noise(rng, 0.004), "high", 3000.0) * decay(samples(0.004), 0.001)
    body = click(rng, freq, seconds, 2.5)
    return mix(seconds + 0.004, (0.0, tick, 0.4), (0.002, body, 1.0))


def rustle(rng, seconds, low=300.0, high=3500.0):
    """Cloth or a bag moving: band-limited noise, swelling and fading, with a slow uneven grain"""
    n = samples(seconds)
    shape = np.sin(np.linspace(0.0, math.pi, n)) ** 1.5
    grain = 0.6 + 0.4 * biquad(rng.standard_normal(n), "low", 25.0) * 8.0
    return biquad(biquad(noise(rng, seconds), "high", low), "low", high) * shape * np.clip(grain, 0.2, 1.4)


def pickup_health():
    rng = np.random.default_rng(600)
    out = mix(0.45,
              (0.0, snap(rng, 1900.0), 1.0),
              (0.09, snap(rng, 1700.0), 0.9),
              (0.12, rustle(rng, 0.25, 500.0, 4000.0), 0.35))
    return fade_out(normalize(out, 0.55), 0.03)


def velcro(rng, seconds):
    """Hooks tearing from loops: a dense run of tiny crackles, rough, bright"""
    n = samples(seconds)
    crackles = (rng.random(n) < 0.06).astype(float) * rng.standard_normal(n) * 3.0
    hiss = noise(rng, seconds) * 0.5
    body = biquad(biquad(crackles + hiss, "high", 900.0), "low", 7000.0)
    shape = np.minimum(1.0, np.linspace(0.0, 1.0, n) * 8.0) * np.linspace(1.0, 0.6, n)
    return body * shape


def pickup_armor():
    rng = np.random.default_rng(610)
    press = biquad(noise(rng, 0.06), "low", 1200.0) * decay(samples(0.06), 0.02)
    out = mix(0.6,
              (0.0, velcro(rng, 0.22), 1.0),
              (0.18, rustle(rng, 0.2, 200.0, 2500.0), 0.5),
              (0.36, press, 0.6),
              (0.44, click(rng, 2600.0, 0.012, 5.0), 0.8))
    return fade_out(normalize(out, 0.55), 0.03)


def clank(rng, base, seconds=0.35):
    """A steel box's lid: a knock and its ring (a few partials not in tune, each dying at its own rate)"""
    t = t_of(seconds)
    ring = np.zeros(len(t))
    for ratio, gain, tau in ((1.0, 1.0, 0.09), (2.31, 0.6, 0.06), (3.87, 0.35, 0.04), (5.4, 0.2, 0.025)):
        ring += gain * np.sin(2 * np.pi * base * ratio * t + rng.random() * 6.0) * np.exp(-t / tau)
    knock = biquad(noise(rng, 0.02), "low", 1500.0) * decay(samples(0.02), 0.005)
    return mix(seconds, (0.0, knock, 1.0), (0.0, ring, 0.35))


def pickup_ammo():
    rng = np.random.default_rng(620)
    rattle = [(0.08 + 0.035 * k + 0.01 * rng.random(), click(rng, 2800.0 + 450.0 * rng.random(), 0.01, 6.0), 0.55 - 0.06 * k)
              for k in range(6)]
    out = mix(0.55, (0.0, clank(rng, 520.0), 1.0), *rattle, (0.3, clank(rng, 610.0, 0.2), 0.45))
    return fade_out(normalize(saturate(out, 1.3), 0.55), 0.03)


def electric_spark(variant):
    rng = np.random.default_rng(700 + variant)
    length = 0.28 + 0.06 * variant
    n = samples(length)
    out = np.zeros(n)
    # Sharp crackles, thick at the start and thinning
    count = 14 + 4 * variant
    for k in range(count):
        at = (rng.random() ** 1.8) * (length - 0.02)
        size = 0.0015 + 0.004 * rng.random()
        pop = noise(rng, size) * decay(samples(size), size / 3.0, attack=0.0001)
        i = int(at * RATE)
        out[i:i + len(pop)] += pop * (0.4 + rng.random()) * (1.0 - 0.6 * at / length)
    out = biquad(out, "high", 1200.0)
    # The zap under them: the mains' harmonics buzzing, hard-clipped, dying out
    t = t_of(length)
    zap = np.sign(np.sin(2 * np.pi * 100.0 * t)) * 0.5 + np.sin(2 * np.pi * 300.0 * t) * 0.5
    zap = biquad(zap, "band", 1800.0 + 400.0 * variant, 1.2) * np.exp(-t / (0.05 + 0.02 * variant))
    hiss = biquad(noise(rng, length), "high", 4000.0) * np.exp(-t / 0.06)
    out = out + zap * 0.6 + hiss * 0.25
    return fade_out(normalize(saturate(out, 2.0), 0.7), 0.02)


def electric_hum():
    rng = np.random.default_rng(710)
    length = 2.0  # whole cycles of 50 Hz and the slow wobble: the loop has no seam
    t = t_of(length)
    hum = (0.5 * np.sin(2 * np.pi * 50.0 * t) + 1.0 * np.sin(2 * np.pi * 100.0 * t) + 0.45 * np.sin(2 * np.pi * 200.0 * t)
           + 0.25 * np.sin(2 * np.pi * 300.0 * t) + 0.12 * np.sin(2 * np.pi * 400.0 * t))
    buzz = np.clip(np.sin(2 * np.pi * 100.0 * t) * 3.0, -1.0, 1.0)  # the transformer's laminations
    buzz = buzz - np.mean(buzz)
    wobble = 1.0 + 0.15 * np.sin(2 * np.pi * 1.5 * t)  # 3 wobbles in the 2 s
    # Faint crackle (a loop of noise: its end meets its start as any noise does)
    crackle = (rng.random(len(t)) < 0.002).astype(float) * rng.standard_normal(len(t))
    crackle = np.convolve(crackle, np.exp(-np.arange(40) / 8.0), mode="same")
    out = (hum * 0.6 + buzz * 0.18) * wobble + crackle * 0.15
    # (no fade: it loops; the peak kept low, the AudioSource sets the level)
    return normalize(out, 0.5)


def grunt(rng, seconds, pitch_from, pitch_to, breath=0.5):
    """A short voiced grunt: a pulse train gliding down in pitch through the "uh" vowel's formants, with breath on it"""
    t = t_of(seconds)
    f0 = pitch_from + (pitch_to - pitch_from) * (t / seconds) ** 0.7
    f0 = f0 * (1.0 + 0.02 * np.sin(2 * np.pi * 7.0 * t))  # a little unsteady
    phase = np.cumsum(f0) / RATE
    pulses = (phase % 1.0) * 2.0 - 1.0  # a sawtooth: rich in harmonics, as a glottis's pulses
    voiced = np.zeros(len(t))
    for f, q, g in ((640.0, 5.0, 1.0), (1150.0, 6.0, 0.6), (2450.0, 8.0, 0.25)):
        voiced += biquad(pulses, "band", f, q) * g
    air = biquad(biquad(noise(rng, seconds), "band", 1200.0, 0.9), "high", 400.0)
    env = np.minimum(1.0, t / 0.015) * np.exp(-np.maximum(0.0, t - 0.03) / (seconds * 0.35))
    return (voiced * (1.0 - breath) + air * breath * 0.6) * env


def thump(rng, seconds=0.12, freq=70.0):
    """A body hit: a low knock falling in pitch, a muffled slap"""
    t = t_of(seconds)
    f = freq + 50.0 * np.exp(-t / 0.015)
    body = np.sin(2 * np.pi * np.cumsum(f) / RATE) * decay(len(t), 0.035, attack=0.001)
    slap = biquad(noise(rng, 0.03), "low", 800.0) * decay(samples(0.03), 0.008)
    return mix(seconds, (0.0, body, 1.0), (0.0, slap, 0.6))


def player_hurt():
    rng = np.random.default_rng(800)
    out = mix(0.35, (0.0, thump(rng), 0.9), (0.02, grunt(rng, 0.22, 150.0, 115.0, breath=0.45), 0.8))
    return fade_out(normalize(saturate(out, 1.3), 0.7), 0.03)


def player_death():
    rng = np.random.default_rng(810)
    gear = [(0.62 + 0.03 * k + 0.01 * rng.random(), click(rng, 2200.0 + 600.0 * rng.random(), 0.012, 5.0), 0.35)
            for k in range(4)]
    out = mix(1.2,
              (0.0, thump(rng), 0.8),
              (0.02, grunt(rng, 0.55, 140.0, 85.0, breath=0.5), 0.9),
              (0.55, thump(rng, 0.2, 55.0), 1.0),          # the body on the ground
              (0.6, rustle(rng, 0.3, 150.0, 2000.0), 0.4),
              (0.78, thump(rng, 0.15, 65.0), 0.5),         # an arm, the head
              *gear)
    return fade_out(normalize(saturate(out, 1.3), 0.75), 0.05)


def radio_tone(freq, seconds):
    """A soft tone: a sine with a little of its third (a hollow, square-ish edge), a quick swell and a smooth fade"""
    t = t_of(seconds)
    tone = np.sin(2.0 * math.pi * freq * t) + 0.18 * np.sin(2.0 * math.pi * 3.0 * freq * t)
    n = len(t)
    attack = min(n, samples(0.012))
    shape = np.ones(n)
    shape[:attack] = np.linspace(0.0, 1.0, attack)
    shape *= np.exp(-t / (seconds * 0.6))
    return tone * shape


def objective_complete():
    rng = np.random.default_rng(900)
    squelch = biquad(biquad(noise(rng, 0.07), "high", 500.0), "low", 3200.0) * decay(samples(0.07), 0.02)
    out = mix(0.75,
              (0.0, click(rng, 1800.0, 0.01, 3.0), 0.5),     # the key
              (0.005, squelch, 0.35),
              (0.09, radio_tone(784.0, 0.22), 0.8),           # G5
              (0.27, radio_tone(1175.0, 0.42), 0.8),          # D6, a fifth up
              (0.68, click(rng, 1500.0, 0.01, 3.0), 0.3))     # the key let go
    # The radio's narrow band and a little grit
    out = biquad(biquad(out, "high", 350.0), "low", 3800.0)
    out += biquad(noise(rng, 0.75), "band", 2500.0, 0.8) * 0.012
    return fade_out(normalize(saturate(out, 1.4), 0.6), 0.02)


def latch(rng, seconds=0.05):
    """A door's latch: a steel tongue sliding against its spring, a dry metal tick at each end"""
    slide = biquad(biquad(noise(rng, seconds), "band", 3200.0, 1.5), "high", 1500.0) * np.sin(np.linspace(0.0, math.pi, samples(seconds)))
    return mix(seconds + 0.02, (0.0, click(rng, 3400.0, 0.008, 6.0), 0.6), (0.004, slide, 0.25),
               (seconds, click(rng, 2900.0, 0.01, 6.0), 0.8))


def creak(rng, seconds, pitch_from, pitch_to):
    """A hinge's creak: stick-slip, a train of tiny pulses whose rate wanders, through a wooden body's resonances"""
    t = t_of(seconds)
    rate = pitch_from + (pitch_to - pitch_from) * (t / seconds) + 12.0 * biquad(rng.standard_normal(len(t)), "low", 18.0) * 6.0
    phase = np.cumsum(np.maximum(rate, 40.0)) / RATE
    pulses = np.maximum(0.0, np.sin(2 * np.pi * phase)) ** 8
    body = np.zeros(len(t))
    for f, q, g in ((900.0, 6.0, 1.0), (1650.0, 7.0, 0.7), (2600.0, 8.0, 0.35)):
        body += biquad(pulses, "band", f, q) * g
    return body * np.sin(np.linspace(0.0, math.pi, len(t))) ** 0.8


def swish(rng, seconds):
    """A leaf pushing the air: soft low noise, swelling and fading"""
    return biquad(biquad(noise(rng, seconds), "low", 600.0), "high", 80.0) * np.sin(np.linspace(0.0, math.pi, samples(seconds))) ** 2


def door_thud(rng, seconds=0.3):
    """A wooden leaf hitting its frame: a hollow low knock (the panel's modes ringing briefly) and a slap"""
    t = t_of(seconds)
    ring = np.zeros(len(t))
    for freq, gain, tau in ((95.0, 1.0, 0.07), (180.0, 0.7, 0.05), (310.0, 0.4, 0.035), (520.0, 0.25, 0.02)):
        ring += gain * np.sin(2 * np.pi * freq * t + rng.random() * 6.0) * np.exp(-t / tau)
    slap = biquad(noise(rng, 0.03), "low", 1800.0) * decay(samples(0.03), 0.006)
    return mix(seconds, (0.0, ring, 1.0), (0.0, slap, 0.7))


def door_open():
    rng = np.random.default_rng(1000)
    out = mix(0.8,
              (0.0, latch(rng), 0.9),
              (0.1, creak(rng, 0.38, 260.0, 170.0), 0.45),
              (0.12, swish(rng, 0.55), 0.5))
    return fade_out(normalize(out, 0.6), 0.05)


def door_close():
    rng = np.random.default_rng(1010)
    out = mix(0.7,
              (0.0, swish(rng, 0.3), 0.5),
              (0.24, door_thud(rng), 1.0),
              (0.25, click(rng, 3100.0, 0.012, 6.0), 0.7),      # the latch snapping into the strike
              (0.27, rustle(rng, 0.12, 300.0, 2500.0), 0.15))   # the frame settling
    return fade_out(normalize(saturate(out, 1.3), 0.65), 0.05)


def door_locked():
    rng = np.random.default_rng(1020)
    parts = []
    for k, at in enumerate((0.0, 0.32)):
        parts.append((at, click(rng, 2300.0 + 200.0 * k, 0.012, 5.0), 0.7))     # the handle down
        parts.append((at + 0.05, door_thud(rng, 0.12), 0.35))                   # the latch on the bolt, the leaf a little
        parts.append((at + 0.055, click(rng, 1900.0, 0.015, 4.0), 0.6))
        parts.append((at + 0.16, click(rng, 2600.0, 0.01, 6.0), 0.35))          # the handle back up
    out = mix(0.6, *parts)
    return fade_out(normalize(saturate(out, 1.2), 0.6), 0.04)


def main():
    root = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    folder = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "Projects", "Outpost", "Assets", "Audio")
    os.makedirs(folder, exist_ok=True)
    save(folder, "Pickup_Health.wav", pickup_health())
    save(folder, "Pickup_Armor.wav", pickup_armor())
    save(folder, "Pickup_Ammo.wav", pickup_ammo())
    for variant in (1, 2, 3):
        save(folder, f"Electric_Spark_{variant}.wav", electric_spark(variant))
    save(folder, "Electric_Hum.wav", electric_hum())
    save(folder, "Player_Hurt.wav", player_hurt())
    save(folder, "Player_Death.wav", player_death())
    save(folder, "Objective_Complete.wav", objective_complete())
    save(folder, "Door_Open.wav", door_open())
    save(folder, "Door_Close.wav", door_close())
    save(folder, "Door_Locked.wav", door_locked())


if __name__ == "__main__":
    main()
