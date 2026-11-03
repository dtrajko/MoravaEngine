"""Generates the sample project's Music.wav: fast arpeggios over Am, E7, A7, Dm, Bdim7, in the style of a neoclassical
guitar duel. Everything is synthesized here, with no samples. Original phrasing, not a transcription of any recording.

Usage: python GenerateMusic.py [output.wav]     (needs numpy; the default output is the sample project's Music.wav)

The whole track is rendered in a ring: note tails, the filters and the reverb wrap from the end back to the start, so
the file loops without a seam. The result is the same on every run (fixed random seed).
"""
import os
import sys
import wave

import numpy as np

RATE = 44100
OVERSAMPLE = 2                  # The distortion runs at twice the rate, to keep its harmonics from folding back down.
BPM = 90.0
BARS = 16
BEAT = 60.0 / BPM
STEP = BEAT / 6.0               # Sextuplets: six notes to the beat.
SECONDS = BARS * 4 * BEAT
TARGET_RMS = 0.2                # About -14 dB, the level of the track this one replaced.

rng = np.random.default_rng(1986)

# MIDI note numbers. Each sweep is seven notes over two octaves, in the chord's own tones.
SWEEPS = {
    'Am':    [57, 60, 64, 69, 72, 76, 81],  # A C E
    'E7':    [52, 56, 59, 62, 64, 68, 71],  # E G# B D
    'A7':    [57, 61, 64, 67, 69, 73, 76],  # A C# E G
    'Dm':    [50, 53, 57, 62, 65, 69, 74],  # D F A
    'Bdim7': [59, 62, 65, 68, 71, 74, 77],  # B D F Ab
}
BASS = {'Am': 33, 'E7': 40, 'A7': 33, 'Dm': 38, 'Bdim7': 35}
# Root and fifth of the chugging rhythm guitar; Bdim7 has a flat fifth.
POWER = {'Am': (45, 52), 'E7': (40, 47), 'A7': (45, 52), 'Dm': (50, 57), 'Bdim7': (47, 53)}

# Two chords to the bar, a chord lasting two beats. The last one, E7, leads back to Am at the top of the next section.
SECTION = ['Am', 'Am', 'E7', 'E7', 'A7', 'A7', 'Dm', 'Dm',
           'Bdim7', 'Bdim7', 'Am', 'Am', 'Dm', 'E7', 'Am', 'E7']
SONG = [('low', chord) for chord in SECTION] + [('high', chord) for chord in SECTION]

UP_THEN_DOWN = [0, 1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1]
DOWN_THEN_UP = [6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5]


def hz(note):
    return 440.0 * 2.0 ** ((note - 69) / 12.0)


def add_ring(buffer, start, signal):
    """Adds signal to the buffer starting at a time in seconds, wrapping at the end."""
    rate_per_second = len(buffer) / SECONDS
    index = np.arange(len(signal)) + int(round(start * rate_per_second))
    np.add.at(buffer, index % len(buffer), signal)


def spectral(signal, rate, gain_of_hz):
    """A zero-phase filter on the ring: multiplies the spectrum by gain_of_hz(frequencies)."""
    spectrum = np.fft.rfft(signal)
    return np.fft.irfft(spectrum * gain_of_hz(np.fft.rfftfreq(len(signal), 1.0 / rate)), len(signal))


def highpass(f, cutoff, order=2):
    return 1.0 / np.sqrt(1.0 + (cutoff / np.maximum(f, 1e-3)) ** (2 * order))


def lowpass(f, cutoff, order=2):
    return 1.0 / np.sqrt(1.0 + (f / cutoff) ** (2 * order))


def bell(f, center, octaves, decibels):
    return 10.0 ** (decibels / 20.0 * np.exp(-(np.log2(np.maximum(f, 1e-3) / center) / octaves) ** 2))


def pluck(note, duration, rate, brightness=1.0, damping=1.0):
    """A plucked string: harmonics with the comb of a pick position, the higher ones dying sooner, and a little stiffness."""
    f0 = hz(note)
    t = np.arange(int(duration * rate)) / rate
    out = np.zeros_like(t)
    for k in range(1, 25):
        fk = k * f0 * np.sqrt(1.0 + 1.2e-4 * k * k)
        if fk > 9000.0:
            break
        amplitude = abs(np.sin(np.pi * k * 0.17)) / k ** (1.15 / brightness)
        out += amplitude * np.exp(-t * damping * (2.5 + 0.9 * k)) * np.sin(2.0 * np.pi * fk * t + 0.7 * k)
    attack = np.minimum(t / 0.001, 1.0)
    release = np.minimum((duration - t) / 0.006, 1.0)
    return out * attack * release


def distort(signal, drive):
    """An amp: tightened lows into a slightly asymmetric clipper, then a speaker cabinet."""
    rate = RATE * OVERSAMPLE
    tight = spectral(signal, rate, lambda f: highpass(f, 280.0) * bell(f, 1000.0, 1.2, 4.0))
    clipped = np.tanh(drive * tight + 0.12) - np.tanh(0.12)
    return spectral(clipped, rate, lambda f: highpass(f, 85.0) * lowpass(f, 4800.0, 3) * bell(f, 2400.0, 0.8, 4.0) * bell(f, 400.0, 0.7, -3.0))


def decimate(signal):
    """Back to the output rate, by cutting the spectrum off below the new Nyquist frequency."""
    spectrum = np.fft.rfft(signal)
    count = len(signal) // OVERSAMPLE
    return np.fft.irfft(spectrum[:count // 2 + 1], count) / OVERSAMPLE


def lead_guitar():
    rate = RATE * OVERSAMPLE
    bus = np.zeros(int(SECONDS * rate))
    chord_length = 2 * BEAT
    for cycle, (register, chord) in enumerate(SONG):
        sweep = SWEEPS[chord]
        high = register == 'high'
        order = DOWN_THEN_UP if high else UP_THEN_DOWN
        for i, position in enumerate(order):
            note = sweep[position] + (12 if high else 0)
            velocity = 0.62 + rng.uniform(-0.05, 0.05)
            if position == 6:
                velocity += 0.3   # The top of the sweep sings out.
            elif i % 6 == 0:
                velocity += 0.16  # The beat.
            start = cycle * chord_length + i * STEP + rng.uniform(-0.0015, 0.0015)
            add_ring(bus, start, velocity * pluck(note, STEP * 2.0, rate, brightness=1.15))
    return decimate(distort(bus, 6.0))


def rhythm_guitar():
    """Muted power chords on the eighth notes, in the second section only, to lift it."""
    rate = RATE * OVERSAMPLE
    bus = np.zeros(int(SECONDS * rate))
    half = len(SONG) // 2
    for cycle, (register, chord) in enumerate(SONG):
        if cycle < half:
            continue
        for eighth in range(4):
            start = (cycle * 2 + eighth * 0.5) * BEAT + rng.uniform(-0.002, 0.002)
            hit = 0.9 if eighth == 0 else 0.65
            for note in POWER[chord]:
                add_ring(bus, start, hit * pluck(note, BEAT * 0.42, rate, damping=3.2))
    return decimate(distort(bus, 4.0))


def bass():
    bus = np.zeros(int(SECONDS * RATE))
    for cycle, (register, chord) in enumerate(SONG):
        for eighth in range(4):
            start = (cycle * 2 + eighth * 0.5) * BEAT
            t = np.arange(int(BEAT * 0.48 * RATE)) / RATE
            f0 = hz(BASS[chord])
            wave_ = sum(np.sin(2.0 * np.pi * k * f0 * t) / k ** 1.4 for k in range(1, 9))
            envelope = np.exp(-t * 3.0) * np.minimum(t / 0.003, 1.0) * np.minimum((t[-1] - t + 1e-6) / 0.008, 1.0)
            add_ring(bus, start, (1.0 if eighth == 0 else 0.8) * wave_ * envelope)
    return spectral(np.tanh(1.3 * bus), RATE, lambda f: lowpass(f, 900.0) * highpass(f, 35.0))


def drums():
    bus = np.zeros(int(SECONDS * RATE))
    kick_t = np.arange(int(0.22 * RATE)) / RATE
    phase = 2.0 * np.pi * (52.0 * kick_t + (170.0 - 52.0) * (1.0 - np.exp(-kick_t * 38.0)) / 38.0)
    kick = np.sin(phase) * np.exp(-kick_t * 15.0) + 0.5 * np.exp(-kick_t * 600.0) * rng.normal(0.0, 1.0, len(kick_t))

    snare_t = np.arange(int(0.2 * RATE)) / RATE
    rattle = spectral(rng.normal(0.0, 1.0, len(snare_t)), RATE, lambda f: highpass(f, 1600.0) * lowpass(f, 9000.0))
    snare = 2.2 * rattle * np.exp(-snare_t * 20.0) + 0.7 * np.sin(2.0 * np.pi * 190.0 * snare_t) * np.exp(-snare_t * 28.0)

    hat_t = np.arange(int(0.05 * RATE)) / RATE
    hat = spectral(rng.normal(0.0, 1.0, len(hat_t)), RATE, lambda f: highpass(f, 7000.0, 3)) * np.exp(-hat_t * 110.0)

    for bar in range(BARS):
        origin = bar * 4 * BEAT
        for beat in (0.0, 1.5, 2.0, 3.5):
            add_ring(bus, origin + beat * BEAT, 0.95 * kick)
        for beat in (1.0, 3.0):
            add_ring(bus, origin + beat * BEAT, 0.72 * snare)
        for eighth in range(8):
            add_ring(bus, origin + eighth * 0.5 * BEAT, (0.5 if eighth % 2 == 0 else 0.3) * hat)
    return bus


def reverb(signal, seconds=1.8, predelay=0.03):
    """A large room: convolution, on the ring, with a decaying noise tail."""
    tail = int(seconds * RATE)
    t = np.arange(tail) / RATE
    impulse = np.zeros(len(signal))
    start = int(predelay * RATE)
    impulse[start:start + tail] = rng.normal(0.0, 1.0, tail) * np.exp(-t * 6.9 / seconds)
    impulse = spectral(impulse, RATE, lambda f: highpass(f, 300.0) * lowpass(f, 4500.0))
    impulse /= np.sqrt(np.sum(impulse * impulse))
    return np.fft.irfft(np.fft.rfft(signal) * np.fft.rfft(impulse), len(signal))


def unit(signal):
    """Scales to an RMS of 1, so the mix below reads as relative loudness."""
    return signal / np.sqrt(np.mean(signal * signal))


def main():
    default = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'Projects', 'Sample', 'Assets', 'Audio', 'Music.wav')
    path = sys.argv[1] if len(sys.argv) > 1 else default

    lead = unit(lead_guitar())
    rhythm = rhythm_guitar()
    rhythm /= np.sqrt(np.mean(rhythm[len(rhythm) // 2:] ** 2))  # It only plays in the second half.
    low = unit(bass())
    kit = unit(drums())

    guitars = 1.0 * lead + 0.34 * rhythm
    wet = reverb(guitars + 0.25 * kit)
    mix = guitars + 0.38 * wet + 0.5 * low + 0.2 * kit  # The kit is sparse: its RMS of 1 means tall peaks.
    mix = np.tanh(0.42 * mix)         # Gentle bus limiting: the drum peaks round off rather than clip.
    mix -= np.mean(mix)
    mix *= TARGET_RMS / np.sqrt(np.mean(mix * mix))
    print('peak', round(float(np.max(np.abs(mix))), 3))

    with wave.open(path, 'wb') as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(np.round(mix * 32767.0).astype('<i2').tobytes())
    print('wrote', os.path.normpath(path), round(len(mix) / RATE, 3), 's')


if __name__ == '__main__':
    main()
