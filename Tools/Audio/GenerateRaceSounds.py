"""Generates the racing game's short sounds (docs/games/Racing_Plan.html, Phase 6), all synthesized here, no samples:

    LightRed.wav, LightYellow.wav   the start lights' beeps: a short tone, the same pitch for both
    LightGreen.wav                  green, go: the same tone an octave up, longer
    Checkpoint.wav                  a split: a two-note bell chime
    CheckpointFaster.wav            a split faster than the best lap's: the chime a fourth higher
    Finish.wav                      the finish: a rising major arpeggio ending on a held chord
    WallScrape.wav                  scraping a wall: grinding noise, a loop (the race scales its volume with the speed)
    TireSqueal.wav                  sliding tires: narrow wobbling noise bands with chatter, a loop (volume from the slide)

The engine's loops are recordings (CC0), not made here.

Usage: python GenerateRaceSounds.py [output folder]     (needs numpy; the default is the racing project's Assets/Audio)

16-bit mono at 44.1 kHz. The result is the same on every run (fixed random seed).
"""
import os
import sys
import wave

import numpy as np

RATE = 44100
rng = np.random.default_rng(2026)


def t_of(seconds):
    return np.arange(int(round(seconds * RATE))) / RATE


def envelope(n, attack, release, hold=None):
    """A linear attack, then an exponential release over the rest (or after hold seconds)"""
    e = np.ones(n)
    a = max(1, int(attack * RATE))
    e[:a] = np.linspace(0.0, 1.0, a)
    start = a if hold is None else max(a, int(hold * RATE))
    rest = n - start
    if rest > 0:
        e[start:] = np.exp(-np.arange(rest) / RATE / max(release, 1e-4))
    e[-64:] *= np.linspace(1.0, 0.0, 64)  # no click at the end
    return e


def tone(freq, seconds, harmonics=((1, 1.0),), attack=0.004, release=0.2, hold=None):
    t = t_of(seconds)
    out = np.zeros_like(t)
    for k, amp in harmonics:
        out += amp * np.sin(2 * np.pi * freq * k * t)
    return out * envelope(len(t), attack, release, hold)


def bell(freq, seconds, release=0.6):
    """A bell-like note: inharmonic partials that die away at their own pace"""
    t = t_of(seconds)
    out = np.zeros_like(t)
    for ratio, amp, decay in ((1.0, 1.0, release), (2.0, 0.5, release * 0.6), (2.76, 0.35, release * 0.4),
                              (5.4, 0.15, release * 0.25)):
        out += amp * np.sin(2 * np.pi * freq * ratio * t) * np.exp(-t / decay)
    a = int(0.003 * RATE)
    out[:a] *= np.linspace(0.0, 1.0, a)
    out[-64:] *= np.linspace(1.0, 0.0, 64)
    return out


def mix(length, *parts):
    """Parts (start seconds, samples) into one buffer of length seconds"""
    out = np.zeros(int(round(length * RATE)))
    for start, samples in parts:
        i = int(round(start * RATE))
        n = min(len(samples), len(out) - i)
        out[i:i + n] += samples[:n]
    return out


def normalize(samples, peak=0.8):
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


def wall_scrape(seconds=2.0):
    """Grinding: noise through a few resonances, a fast irregular flutter on top; its ends cross-faded into a loop"""
    n = int(seconds * RATE)
    fade = int(0.1 * RATE)
    noise = rng.standard_normal(n + fade)
    spectrum = np.fft.rfft(noise)
    freqs = np.fft.rfftfreq(len(noise), 1.0 / RATE)
    shape = np.zeros_like(freqs)
    for center, width, gain in ((900, 500, 1.0), (2300, 700, 0.7), (4200, 1200, 0.35), (250, 150, 0.4)):
        shape += gain * np.exp(-0.5 * ((freqs - center) / width) ** 2)
    filtered = np.fft.irfft(spectrum * shape, len(noise))
    flutter = 0.65 + 0.35 * np.abs(np.convolve(rng.standard_normal(len(noise)), np.ones(300) / 300, mode='same')) * 8
    s = filtered * np.clip(flutter, 0.3, 1.4)
    # The loop: the extra tail faded in over the start
    ramp = np.linspace(0.0, 1.0, fade)
    s[:fade] = s[:fade] * ramp + s[n:n + fade] * (1.0 - ramp)
    return normalize(s[:n], 0.7)


def bandpass(f, fc, q):
    """A second-order band-pass's complex response (peak 1 at fc)"""
    f = np.maximum(f, 1e-3)
    return 1.0 / (1.0 + 1j * q * (f / fc - fc / f))


def smooth_periodic(r, n, width):
    """Slow random values, periodic over n (a circular moving average), about -1..1"""
    x = r.standard_normal(n)
    k = np.zeros(n)
    k[:width] = 1.0 / width
    y = np.fft.irfft(np.fft.rfft(x) * np.fft.rfft(k), n)
    return y / max(np.max(np.abs(y)), 1e-9)


def tire_squeal(seconds=2.0):
    """A squeal's few partials (about 1.05, 2.15 and 3.3 kHz): noise through narrow resonances whose centers wobble,
    with a chatter in the loudness. Blocks overlap-added around the loop (circular), so the loop is seamless. Its own
    seed: the sound the user chose from the previews (2026-11-09, "Tire A")"""
    r = np.random.default_rng(7)
    n = int(seconds * RATE)
    out = np.zeros(n)
    hop = 1024
    win = np.hanning(2 * hop)
    f = np.fft.rfftfreq(2 * hop, 1.0 / RATE)
    for base, gain in ((1050, 1.0), (2150, 0.55), (3300, 0.25)):
        wobble = smooth_periodic(r, n, int(0.2 * RATE)) * 0.08 + smooth_periodic(r, n, int(0.02 * RATE)) * 0.02
        source = r.standard_normal(n)
        part = np.zeros(n)
        for start in range(0, n, hop):
            idx = (np.arange(2 * hop) + start) % n
            y = np.fft.irfft(np.fft.rfft(source[idx] * win) * np.abs(bandpass(f, base * (1 + wobble[start]), 35)), 2 * hop)
            np.add.at(part, idx, y)
        out += gain * part / max(np.std(part), 1e-9)
    chatter = 0.75 + 0.25 * smooth_periodic(r, n, int(0.015 * RATE))
    return normalize(out * chatter, 0.7)


def main():
    folder = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..',
                                                                'Projects', 'Racing', 'Assets', 'Audio')
    folder = os.path.normpath(folder)
    os.makedirs(folder, exist_ok=True)

    beep_harmonics = ((1, 1.0), (2, 0.25), (3, 0.12))
    low = tone(587.33, 0.32, beep_harmonics, release=0.06, hold=0.22)       # D5
    save(folder, 'LightRed.wav', normalize(low, 0.7))
    save(folder, 'LightYellow.wav', normalize(low, 0.7))
    high = tone(1174.66, 0.9, beep_harmonics, release=0.25, hold=0.45)      # D6
    save(folder, 'LightGreen.wav', normalize(high, 0.75))

    # The chime: two bell notes a major third apart (E6, then G#6); faster: a fourth higher (A6, C#7)
    for name, f1 in (('Checkpoint.wav', 1318.51), ('CheckpointFaster.wav', 1760.0)):
        chime = mix(1.2, (0.0, bell(f1, 1.2, 0.35)), (0.11, bell(f1 * 1.26, 1.1, 0.45)))
        save(folder, name, normalize(chime, 0.6))

    # The finish: C major arpeggio up two octaves, then the chord held (soft square-ish tones and a bell on top)
    notes = [523.25, 659.25, 783.99, 1046.5, 1318.51]
    voice = ((1, 1.0), (3, 0.22), (5, 0.08))
    parts = [(0.11 * i, tone(f, 0.5, voice, release=0.18)) for i, f in enumerate(notes[:4])]
    chord_start = 0.48
    for f in (523.25, 659.25, 783.99, 1046.5):
        parts.append((chord_start, 0.55 * tone(f, 1.8, voice, attack=0.01, release=0.6, hold=0.5)))
    parts.append((chord_start, 0.6 * bell(notes[4], 1.6, 0.7)))
    save(folder, 'Finish.wav', normalize(mix(2.4, *parts), 0.75))

    save(folder, 'WallScrape.wav', wall_scrape())
    save(folder, 'TireSqueal.wav', tire_squeal())


if __name__ == '__main__':
    main()
