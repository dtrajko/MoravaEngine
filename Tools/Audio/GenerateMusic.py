"""Generates the sample project's Music.wav: a bright, light adventure theme for a collecting game, in G major at 112 BPM.
Mallets, a soft flute-like lead, warm pads, a round bass and light drums. Everything is synthesized here, with no samples,
and the tune is original.

It is made to sit under a game: the lead leaves gaps, stays out of the range where the coin and jump sounds are loudest for
long, and nothing in it demands attention. Three sections of eight bars:

    A   the theme over a light groove
    A'  the same with a second voice and busier drums
    B   a calmer turn to E minor with half-time drums, leading back to the top

Usage: python GenerateMusic.py [output.wav]     (needs numpy; the default output is the sample project's Music.wav)

The whole track is rendered in a ring: note tails and the reverb wrap from the end back to the start, so the file loops
without a seam. The result is the same on every run (fixed random seed).
"""
import os
import sys
import wave

import numpy as np

RATE = 44100
BPM = 112.0
BARS = 24
BEAT = 60.0 / BPM
LENGTH = int(round(BARS * 4 * BEAT * RATE))
TARGET_RMS = 0.17               # A little under the previous track: music under a game, not over it.

rng = np.random.default_rng(2026)

CHORDS = {
    'G':  [55, 59, 62],
    'D':  [50, 54, 57],
    'Em': [52, 55, 59],
    'C':  [48, 52, 55],
    'Am': [45, 48, 52],
}
ROOT = {'G': 31, 'D': 38, 'Em': 40, 'C': 36, 'Am': 33}  # The bass's notes.

A = ['G', 'D', 'Em', 'C', 'G', 'D', 'C', 'D']
B = ['Em', 'C', 'G', 'D', 'Em', 'C', 'Am', 'D']
SONG = A + A + B

# The tune: (beat within the section, MIDI note, length in beats).
THEME = [
    (0, 71, 1), (1, 74, .5), (1.5, 79, 1.5), (3, 78, .5), (3.5, 76, .5),
    (4, 74, 1.5), (5.5, 69, .5), (6, 71, 1),
    (8, 67, .5), (8.5, 71, .5), (9, 76, 1.5), (10.5, 74, .5), (11, 71, 1),
    (12, 72, 1), (13, 76, .5), (13.5, 74, .5), (14, 72, .5), (14.5, 71, .5), (15, 69, 1),
    (16, 71, 1), (17, 74, .5), (17.5, 79, 1.5), (19, 81, .5), (19.5, 83, .5),
    (20, 81, 1.5), (21.5, 78, .5), (22, 74, 1),
    (24, 76, .5), (24.5, 79, .5), (25, 76, .5), (25.5, 72, .5), (26, 74, .5), (26.5, 76, .5), (27, 79, 1),
    (28, 78, 1), (29, 81, .5), (29.5, 78, .5), (30, 74, 1.5),
]
BRIDGE = [
    (0, 79, 2), (2, 78, 1), (3, 76, 1),
    (4, 76, 2), (6, 79, 1.5),
    (8, 74, 2), (10, 79, 1), (11, 83, 1),
    (12, 81, 3), (15, 78, 1),
    (16, 79, 1), (17, 83, 1), (18, 81, 1), (19, 79, 1),
    (20, 76, 2), (22, 79, 1), (23, 76, 1),
    (24, 72, 1), (25, 76, 1), (26, 81, 1), (27, 79, 1),
    (28, 78, 1), (29, 81, 1), (30, 78, .5), (30.5, 74, .5), (31, 69, 1),
]
# The second voice of A': a third or a sixth under the tune, on the notes of G major.
SCALE = [55, 57, 59, 60, 62, 64, 66, 67, 69, 71, 72, 74, 76, 78, 79, 81, 83]

# Which notes of the chord the mallets play, per eighth note of a bar.
ARPEGGIO = [0, 1, 2, 3, 1, 2, 3, 2]
ARPEGGIO_CALM = [0, 2, 3, 2, 0, 2, 3, 2]


def hz(note):
    return 440.0 * 2.0 ** ((note - 69) / 12.0)


def seconds(beat):
    return beat * BEAT


def add_ring(buffer, beat, signal, gain=1.0):
    """Adds a signal to the buffer starting at a beat, wrapping at the end."""
    index = (np.arange(len(signal)) + int(round(seconds(beat) * RATE))) % len(buffer)
    np.add.at(buffer, index, signal * gain)


def spectral(signal, gain_of_hz):
    """A zero-phase filter on the ring: multiplies the spectrum by gain_of_hz(frequencies)."""
    spectrum = np.fft.rfft(signal)
    spectrum *= gain_of_hz(np.fft.rfftfreq(len(signal), 1.0 / RATE))
    return np.fft.irfft(spectrum, len(signal))


def timeline(duration):
    return np.arange(int(duration * RATE)) / RATE


def mallet(note, velocity=1.0):
    t = timeline(0.9)
    f = hz(note)
    tone = np.sin(2 * np.pi * f * t) * np.exp(-t * 6.0)
    tone += 0.5 * np.sin(2 * np.pi * f * 4.0 * t) * np.exp(-t * 22.0)  # The bar's bright, quickly gone overtone.
    tone += 0.16 * np.sin(2 * np.pi * f * 9.2 * t) * np.exp(-t * 60.0)  # And the strike.
    return tone * (1.0 - np.exp(-t * 900.0)) * velocity


def lead_note(note, beats, velocity=1.0):
    duration = seconds(beats) * 0.94 + 0.16
    t = timeline(duration)
    f = hz(note)
    # The vibrato comes in late, as a player's does: a short note stays straight.
    depth = 0.0035 * np.clip((t - 0.22) / 0.25, 0.0, 1.0)
    phase = 2 * np.pi * np.cumsum(f * (1.0 + depth * np.sin(2 * np.pi * 5.2 * t))) / RATE
    tone = np.sin(phase) + 0.28 * np.sin(2 * phase) + 0.09 * np.sin(3 * phase) + 0.03 * np.sin(4 * phase)
    breath = spectral(rng.standard_normal(len(t)), lambda hertz: np.exp(-((hertz - f * 2) / 900.0) ** 2))
    tone += 0.05 * breath / np.max(np.abs(breath))
    envelope = np.minimum(t / 0.018, 1.0) * np.minimum((duration - t) / 0.13, 1.0) * (0.78 + 0.22 * np.exp(-t * 7.0))
    return tone * envelope * velocity


def pad_chord(chord, beats):
    duration = seconds(beats) + 0.5
    t = timeline(duration)
    tone = np.zeros(len(t))
    for note in CHORDS[chord]:
        f = hz(note + 12)
        for detune in (-0.0022, 0.0, 0.0022):
            # Random phases: the same pad never adds up to the same peak.
            phase = rng.uniform(0, 2 * np.pi, 4)
            for harmonic, level in enumerate((1.0, 0.42, 0.16, 0.06), start=1):
                tone += level * np.sin(2 * np.pi * f * (1.0 + detune) * harmonic * t + phase[harmonic - 1])
    envelope = np.minimum(t / 0.3, 1.0) * np.minimum((duration - t) / 0.5, 1.0)
    return tone * envelope


def bass_note(note, beats, velocity=1.0):
    duration = seconds(beats) + 0.05
    t = timeline(duration)
    f = hz(note)
    tone = np.sin(2 * np.pi * f * t) + 0.35 * np.sin(4 * np.pi * f * t) * np.exp(-t * 7.0) + 0.1 * np.sin(6 * np.pi * f * t) * np.exp(-t * 14.0)
    envelope = np.minimum(t / 0.006, 1.0) * np.minimum((duration - t) / 0.05, 1.0) * (0.72 + 0.28 * np.exp(-t * 5.0))
    return tone * envelope * velocity


def kick():
    t = timeline(0.28)
    phase = 2 * np.pi * np.cumsum(46.0 + 95.0 * np.exp(-t * 34.0)) / RATE
    return np.sin(phase) * np.exp(-t * 15.0)


def snare():
    t = timeline(0.2)
    noise = spectral(rng.standard_normal(len(t)), lambda hertz: np.clip((hertz - 900.0) / 1500.0, 0.0, 1.0) * np.exp(-hertz / 9000.0))
    noise /= np.max(np.abs(noise))
    return 0.75 * noise * np.exp(-t * 24.0) + 0.45 * np.sin(2 * np.pi * 196.0 * t) * np.exp(-t * 30.0)


def hat(open_hat=False):
    t = timeline(0.16 if open_hat else 0.05)
    noise = spectral(rng.standard_normal(len(t)), lambda hertz: np.clip((hertz - 6500.0) / 2500.0, 0.0, 1.0))
    noise /= np.max(np.abs(noise))
    return noise * np.exp(-t * (28.0 if open_hat else 95.0))


def harmony_below(note):
    """A third under the tune, on the scale; a note under the scale's range stays out."""
    if note not in SCALE:
        return None
    index = SCALE.index(note) - 2
    return SCALE[index] if index >= 0 else None


def melody():
    lead = np.zeros(LENGTH)
    second = np.zeros(LENGTH)
    for section_start, tune, harmonized in ((0, THEME, False), (32, THEME, True), (64, BRIDGE, False)):
        for beat, note, length in tune:
            # A little louder on the beat than off it, and the second time through a little fuller.
            velocity = (1.0 if beat == int(beat) else 0.86) * (1.0 if section_start == 0 else 1.06 if harmonized else 0.9)
            add_ring(lead, section_start + beat, lead_note(note, length, velocity))
            if harmonized:
                below = harmony_below(note)
                if below is not None:
                    add_ring(second, section_start + beat, lead_note(below, length, 0.8 * velocity))
    return lead, second


def mallets():
    out = np.zeros(LENGTH)
    for bar, chord in enumerate(SONG):
        notes = CHORDS[chord] + [CHORDS[chord][0] + 12]
        calm = bar >= 16
        pattern = ARPEGGIO_CALM if calm else ARPEGGIO
        for eighth, choice in enumerate(pattern):
            note = notes[choice] + 12
            accent = 1.0 if eighth % 4 == 0 else 0.84 if eighth % 2 == 0 else 0.68
            add_ring(out, bar * 4 + eighth * 0.5, mallet(note, accent * (0.82 if calm else 1.0)))
    return out


def pads():
    out = np.zeros(LENGTH)
    for bar, chord in enumerate(SONG):
        add_ring(out, bar * 4, pad_chord(chord, 4), 1.0 if bar >= 16 else 0.62)
    return spectral(out, lambda hertz: np.exp(-hertz / 2600.0))


def bass():
    out = np.zeros(LENGTH)
    for bar, chord in enumerate(SONG):
        root = ROOT[chord]
        start = bar * 4
        if bar >= 16:
            add_ring(out, start, bass_note(root, 1.9))
            add_ring(out, start + 2.5, bass_note(root + 7, 0.9, 0.8))
            add_ring(out, start + 3.5, bass_note(root + 12, 0.45, 0.7))
        else:
            add_ring(out, start, bass_note(root, 1.3))
            add_ring(out, start + 1.5, bass_note(root, 0.45, 0.78))
            add_ring(out, start + 2, bass_note(root + 7, 0.9, 0.9))
            add_ring(out, start + 3, bass_note(root + 12, 0.45, 0.8))
            add_ring(out, start + 3.5, bass_note(root + 7, 0.45, 0.7))
    return out


def drums():
    out = np.zeros(LENGTH)
    kick_sound, snare_sound, closed, opened = kick(), snare(), hat(), hat(True)
    for bar in range(BARS):
        start = bar * 4
        if bar >= 16:
            # Half time: the bridge breathes.
            add_ring(out, start, kick_sound)
            add_ring(out, start + 2, snare_sound, 0.6)
            for eighth in range(8):
                add_ring(out, start + eighth * 0.5, closed, 0.2 if eighth % 2 else 0.3)
        else:
            add_ring(out, start, kick_sound)
            add_ring(out, start + 1.5, kick_sound, 0.55)
            add_ring(out, start + 2, kick_sound, 0.9)
            add_ring(out, start + 1, snare_sound, 0.72)
            add_ring(out, start + 3, snare_sound, 0.72)
            busy = bar >= 8
            for sixteenth in range(16 if busy else 8):
                step = 0.25 if busy else 0.5
                level = 0.36 if sixteenth % (4 if busy else 2) == 0 else 0.16
                add_ring(out, start + sixteenth * step, closed, level)
            add_ring(out, start + 3.5, opened, 0.2)
        if bar % 8 == 7:
            # A small fill into each section.
            for step in (3.0, 3.25, 3.5, 3.75):
                add_ring(out, start + step, snare_sound, 0.32 + 0.1 * (step - 3.0) * 4)
    return out


def reverb(signal):
    """A hall on the ring: the signal convolved with fading noise, wrapping at the end."""
    t = np.arange(int(1.6 * RATE)) / RATE
    tail = rng.standard_normal(len(t)) * np.exp(-t * 4.2)
    tail[:int(0.012 * RATE)] = 0.0
    impulse = np.zeros(len(signal))
    impulse[:len(tail)] = tail
    wet = np.fft.irfft(np.fft.rfft(signal) * np.fft.rfft(impulse), len(signal))
    wet = spectral(wet, lambda hertz: np.clip(hertz / 350.0, 0.0, 1.0) * np.exp(-hertz / 5000.0))
    return wet / np.sqrt(np.mean(wet * wet))


def unit(signal):
    """Scales to an RMS of 1, so the mix below reads as relative loudness."""
    return signal / np.sqrt(np.mean(signal * signal))


def main():
    default = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'Projects', 'Sample', 'Assets', 'Audio', 'Music.wav')
    path = sys.argv[1] if len(sys.argv) > 1 else default

    lead, second = melody()
    lead_level = np.sqrt(np.mean(lead * lead))
    tune = (lead + second) / lead_level
    keys = unit(mallets())
    warm = unit(pads())
    low = unit(bass())
    kit = unit(drums())

    # The pads and bass duck a little on each beat, so the groove moves without being any louder.
    beat_position = (np.arange(LENGTH) / RATE / BEAT) % 1.0
    duck = 1.0 - 0.22 * np.exp(-beat_position * 7.0)

    wet = reverb(0.9 * tune + 0.7 * keys + 0.3 * warm + 0.12 * kit)
    mix = 0.72 * tune + 0.5 * keys + 0.3 * warm * duck + 0.62 * low * duck + 0.28 * kit + 0.3 * wet
    mix = np.tanh(0.38 * mix)         # Gentle bus limiting: the peaks round off rather than clip.
    mix -= np.mean(mix)
    mix *= TARGET_RMS / np.sqrt(np.mean(mix * mix))
    print('peak', round(float(np.max(np.abs(mix))), 3))

    with wave.open(path, 'wb') as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(RATE)
        out.writeframes(np.round(np.clip(mix, -1.0, 1.0) * 32767.0).astype('<i2').tobytes())
    print('wrote', os.path.normpath(path), round(len(mix) / RATE, 3), 's')


if __name__ == '__main__':
    main()
