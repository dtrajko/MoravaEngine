"""Generates the racing game's engine sound bank (docs/games/Racing_Plan.html, Phase 6), synthesized, no samples:

    Engine_On_<rpm>.wav, Engine_Off_<rpm>.wav   loops every 500 rpm from 1000 to 7000, on and off the throttle
    ShiftClick.wav                              the click of an upshift (the race also dips the engine for a moment)

The model was measured from a reference the user gave (a W16 hypercar from 0 to 350 km/h, 2026-11-09): its sound is
a harmonic series whose fundamental is the rpm / 30 (the engine's second order: 222 Hz at a 6700 rpm redline) with
the same balance of harmonics at every speed (HARMONICS_DB, the 4th the strongest), over a soft noise about 44 dB
under it, strongest from 1 to 3 kHz and gone above 12 kHz (NOISE_BANDS_DB). Only measurements were taken from it; no
part of the recording is in these files.

Off the throttle (the reference has none): 8 dB softer, darker (the harmonics above the 4th weaker the higher they
are), and 6 dB less noise. The noise also fades toward idle (24 dB lower at 1000 rpm), so the idle is clean.

Each loop is about a second, a whole number of periods of its fundamental: the loop's rpm is adjusted by under 0.1 %
so that its length is a whole number of samples too, and the noise is filtered around the loop (circular), so the
loop has no seam. The race plays the two loops nearest the engine's rpm, cross-faded, each pitched by rpm / its rpm
(a few percent at most) - see Projects/Racing/Scripts/RaceAudio.cs. The rpm each file was made at is in its name, and
exactly in EngineLoops.txt beside them.

Usage: python GenerateEngineSounds.py [output folder] [--preview folder]   (needs numpy)
--preview also writes Run.wav there: the bank played along a 0-350 km/h run as the game would mix it.

16-bit mono at 44.1 kHz. The result is the same on every run (fixed random seed).
"""
import os
import sys
import wave

import numpy as np

RATE = 44100
ORDER = 2.0                     # the fundamental is this many times the crank's rotation: rpm / 60 * ORDER = rpm / 30
RPMS = list(range(1000, 7001, 500))
LOOP_SECONDS = 1.0
# Harmonics 1 to 19 in dB against the strongest, measured (the medians over 1000 frames, at every speed nearly alike)
HARMONICS_DB = [-10, -4.5, -6, 0, -11, -17, -22.5, -18, -28, -24, -24, -25, -24, -32, -35, -35.5, -37, -35, -37]
# (9 to 13 are 2 dB over the medians measured: so the result, measured the same way, matches the reference)
HIGHER_DB = -40                 # harmonics 20 and up: at the noise floor's level, falling 1 dB per harmonic
NOISE_1K_3K_DB = -41            # the noise's median from 1 to 3 kHz against the strongest harmonic (measured)
NOISE_FULL_RPM = 5000.0         # the noise at the reference's level from here up
IDLE_NOISE_CUT_DB = 24.0        # the noise this much lower at 1000 rpm (the user: a clean idle, 2026-11-09)
OFF_NOISE_CUT_DB = 6.0          # and this much lower off the throttle
# The noise between the harmonics, dB against the strongest harmonic, per band (measured)
NOISE_BANDS_DB = [(0, 100, -62), (100, 300, -56), (300, 1000, -48), (1000, 3000, -44), (3000, 6000, -46),
                  (6000, 12000, -51), (12000, 22050, -80)]


def save(path, samples):
    data = (np.clip(samples, -1.0, 1.0) * 32767.0).astype('<i2')
    with wave.open(path, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(data.tobytes())


def noise_shape(freqs):
    """The noise's level per frequency (linear), smooth between the measured bands"""
    centers = np.array([np.sqrt(max(lo, 20) * hi) for lo, hi, _ in NOISE_BANDS_DB])
    levels = np.array([db for _, _, db in NOISE_BANDS_DB])
    return 10 ** (np.interp(np.log(np.maximum(freqs, 20)), np.log(centers), levels) / 20)


def noise_db(rpm, on):
    """The noise's level against the strongest harmonic: the reference's at high rpm on the throttle (measured from
    2650 rpm up), fading out toward idle (a clean idle: IDLE_NOISE_CUT_DB under it at 1000 rpm), lower off the throttle"""
    t = min(max((rpm - 1000.0) / (NOISE_FULL_RPM - 1000.0), 0.0), 1.0)
    return NOISE_1K_3K_DB - IDLE_NOISE_CUT_DB * (1.0 - t) ** 1.5 - (0.0 if on else OFF_NOISE_CUT_DB)


def measured_noise(noise):
    """The median bin from 1 to 3 kHz of this (periodic) noise in 8192-sample Hann frames, as the analysis measured"""
    tiled = np.tile(noise, int(np.ceil(8 * 8192 / len(noise))) + 1)
    window = np.hanning(8192)
    freqs = np.fft.rfftfreq(8192, 1.0 / RATE)
    band = (freqs > 1000) & (freqs < 3000)
    bins = [np.abs(np.fft.rfft(tiled[i:i + 8192] * window))[band] for i in range(0, 8 * 8192, 4096)]
    return float(np.median(np.concatenate(bins)))


def engine_loop(rpm, on, rng):
    """A loop at about this rpm, on or off the throttle: (samples, its exact rpm)"""
    f0 = rpm / 60.0 * ORDER
    periods = max(1, int(round(LOOP_SECONDS * f0)))
    n = int(round(periods * RATE / f0))
    f0 = periods * RATE / n                     # exactly periodic in n samples
    exact_rpm = f0 * 60.0 / ORDER
    t = np.arange(n) / RATE
    x = np.zeros(n)
    phases = np.random.default_rng(11).uniform(0, 2 * np.pi, 64)   # the same phases in every loop
    k = 1
    while k * f0 < RATE * 0.45:
        db = HARMONICS_DB[k - 1] if k <= len(HARMONICS_DB) else HIGHER_DB - (k - len(HARMONICS_DB))
        if not on and k > 4:
            db -= 1.5 * (k - 4)                 # off the throttle: darker
        if db > -80:
            x += 10 ** (db / 20) * np.sin(2 * np.pi * k * f0 * t + phases[k % 64])
        k += 1
    # The noise, filtered around the loop (its spectrum shaped, then back): seamless. Its level is set as it was
    # measured: in 8192-sample Hann frames, the median bin from 1 to 3 kHz NOISE_1K_3K_DB under the strongest
    # harmonic's peak (a sine of amplitude 1 peaks at 8192 / 4 there)
    freqs = np.fft.rfftfreq(n, 1.0 / RATE)
    noise = np.fft.irfft(np.fft.rfft(rng.standard_normal(n)) * noise_shape(freqs), n)
    noise *= 10 ** (noise_db(rpm, on) / 20) * (8192 / 4) / measured_noise(noise)
    x += noise
    level = (0.8 + 0.2 * rpm / 7000.0) * (1.0 if on else 10 ** (-8 / 20))
    return x * level, exact_rpm


def shift_click():
    """An upshift's click: 25 ms of noise in the engine's noise colors, a fast decay"""
    n = int(0.06 * RATE)
    r = np.random.default_rng(5)
    freqs = np.fft.rfftfreq(n, 1.0 / RATE)
    burst = np.fft.irfft(np.fft.rfft(r.standard_normal(n)) * noise_shape(freqs) / 10 ** (-44 / 20), n)
    burst *= np.exp(-np.arange(n) / RATE / 0.008)
    return burst / np.max(np.abs(burst)) * 0.6


def play(bank, rpm_curve, load_curve):
    """The game's mix, offline: per sample the two loops around the rpm (equal-power cross-fade), each pitched by
    rpm / its rpm; the on and off banks cross-faded by the load"""
    out = np.zeros(len(rpm_curve))
    for on in (True, False):
        loops = bank[on]
        rpms = np.array([r for r, _ in loops])
        w_load = np.clip(load_curve if on else 1.0 - load_curve, 0, 1)
        for i, (r, loop) in enumerate(loops):
            lo = rpms[i - 1] if i > 0 else r - 1
            hi = rpms[i + 1] if i + 1 < len(rpms) else r + 1
            w = np.where(rpm_curve <= r, (rpm_curve - lo) / (r - lo), (hi - rpm_curve) / (hi - r))
            if i == 0:
                w = np.where(rpm_curve <= r, 1.0, w)
            if i == len(rpms) - 1:
                w = np.where(rpm_curve >= r, 1.0, w)
            w = np.sqrt(np.clip(w, 0, 1) * w_load)
            if np.max(w) <= 0:
                continue
            pos = np.cumsum(rpm_curve / r) % len(loop)
            i0 = pos.astype(int)
            frac = pos - i0
            out += w * (loop[i0] * (1 - frac) + loop[(i0 + 1) % len(loop)] * frac)
    return out


def run_curve():
    """Like the reference: launch, seven gears to 6700 rpm, each shift a third down (0.655), a dip at each shift"""
    seg, shifts = [], []
    def add(r0, r1, s, load):
        n = int(s * RATE)
        seg.append((np.linspace(r0, r1, n), np.full(n, load)))
    add(1000, 1000, 1.0, 0.0)
    add(1000, 2650, 0.3, 1.0)
    for duration in (2.3, 3.0, 3.0, 3.3, 3.4, 4.0, 5.5):
        add(seg[-1][0][-1], 6700, duration, 1.0)
        if duration != 5.5:
            shifts.append(sum(len(s[0]) for s in seg))
            add(6700, 6700 * 0.655, 0.12, 0.15)
    add(6700, 1000, 3.0, 0.0)
    add(1000, 1000, 1.0, 0.0)
    rpm = np.concatenate([s[0] for s in seg])
    load = np.concatenate([s[1] for s in seg])
    k = int(0.05 * RATE)
    load = np.convolve(load, np.ones(k) / k, mode='same')
    return rpm, load, shifts


def main():
    args = [a for a in sys.argv[1:]]
    preview = None
    if '--preview' in args:
        i = args.index('--preview')
        preview = args[i + 1]
        del args[i:i + 2]
    folder = args[0] if args else os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'Projects', 'Racing',
                                               'Assets', 'Audio')
    folder = os.path.normpath(folder)
    os.makedirs(folder, exist_ok=True)
    rng = np.random.default_rng(2026)
    bank = {True: [], False: []}
    for on in (True, False):
        for rpm in RPMS:
            loop, exact = engine_loop(rpm, on, rng)
            bank[on].append((exact, loop))
    peak = max(np.max(np.abs(l)) for loops in bank.values() for _, l in loops)
    lines = []
    for on in (True, False):
        for (exact, loop), rpm in zip(bank[on], RPMS):
            name = f'Engine_{"On" if on else "Off"}_{rpm}.wav'
            save(os.path.join(folder, name), loop * (0.8 / peak))
            lines.append(f'Assets/Audio/{name}@{exact:.2f}')
    for on in (True, False):
        bank[on] = [(e, l * (0.8 / peak)) for e, l in bank[on]]
    with open(os.path.join(folder, 'EngineLoops.txt'), 'w', encoding='utf-8', newline='\n') as f:
        f.write('# The engine loops and the exact rpm each was made at (GenerateEngineSounds.py): RaceAudio\'s EngineLoopsOn / EngineLoopsOff\n')
        f.write(', '.join(l for l in lines if '_On_' in l) + '\n')
        f.write(', '.join(l for l in lines if '_Off_' in l) + '\n')
    save(os.path.join(folder, 'ShiftClick.wav'), shift_click())
    print(f'{folder}: {len(lines)} engine loops, ShiftClick.wav, EngineLoops.txt')
    if preview:
        os.makedirs(preview, exist_ok=True)
        rpm, load, shifts = run_curve()
        out = play(bank, rpm, load)
        # The shift: the dip measured in the reference (about 7 dB for 0.15 s) and the click
        dip = np.ones(len(out))
        shape = np.concatenate([np.linspace(1, 0.45, int(0.08 * RATE)), np.full(int(0.06 * RATE), 0.45), np.linspace(0.45, 1, int(0.15 * RATE))])
        click = shift_click()
        for at in shifts:
            s = max(0, at - int(0.06 * RATE))
            dip[s:s + len(shape)] = np.minimum(dip[s:s + len(shape)], shape[:len(dip) - s])
        out *= dip
        for at in shifts:
            out[at:at + len(click)] += 0.25 * click[:len(out) - at]
        out *= 0.85 / np.max(np.abs(out))
        save(os.path.join(preview, 'Run.wav'), out)
        print(f'{preview}/Run.wav: {len(out) / RATE:.1f} s')


if __name__ == '__main__':
    main()
