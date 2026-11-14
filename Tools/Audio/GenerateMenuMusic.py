"""Generates the racing game's MenuMusic.wav: a relaxed, warm loop for the main menu, in D major at 76 BPM.
An electric piano, a soft lead, a warm pad, a round bass and quiet, swung lo-fi drums. Everything is synthesized here,
with no samples, and the tune is original.

It is made to sit under the menu while the camera drives along the track: slow chords with a lot of air, a tune that
leaves gaps, nothing loud or busy. Four sections of eight bars:

    Intro   the piano, pad and bass alone; soft hats come in halfway
    A       the tune over a light, swung groove
    B       a turn to G with a higher, held tune and a second voice
    A'      the tune again, a little quieter; the drums thin out at the end, back to the intro

Usage: python GenerateMenuMusic.py [output.wav]     (needs numpy; the default output is the racing project's MenuMusic.wav)

The whole track is rendered in a ring: note tails and the reverb wrap from the end back to the start, so the file loops
without a seam. The result is the same on every run (fixed random seed).
"""
import os
import sys
import wave

import numpy as np

RATE = 44100
BPM = 76.0
BARS = 32
BEAT = 60.0 / BPM
LENGTH = int(round(BARS * 4 * BEAT * RATE))
TARGET_RMS = 0.14               # Quieter than the sample's theme: a menu, nothing to compete with.
SWING = 0.58                    # Where the off-beat eighth falls within a beat (0.5 is straight).

rng = np.random.default_rng(1976)

# The piano's voicings (no roots: the bass has them) and the bass's notes.
CHORDS = {
    'Dmaj9':  [54, 57, 61, 64],
    'Bm9':    [57, 61, 62, 66],
    'Gmaj9':  [54, 57, 59, 62],
    'A13sus': [55, 59, 62, 66],
    'F#m7':   [57, 61, 64, 66],
    'Em9':    [55, 59, 62, 66],
}
ROOT = {'Dmaj9': 38, 'Bm9': 35, 'Gmaj9': 43, 'A13sus': 45, 'F#m7': 42, 'Em9': 40}

A = ['Dmaj9', 'Bm9', 'Gmaj9', 'A13sus', 'Dmaj9', 'F#m7', 'Em9', 'A13sus']
B = ['Gmaj9', 'F#m7', 'Em9', 'Dmaj9', 'Gmaj9', 'F#m7', 'Em9', 'A13sus']
SONG = A + A + B + A
INTRO, SECTION_A, SECTION_B, SECTION_A2 = 0, 8, 16, 24

# The tunes: (beat within the section, MIDI note, length in beats).
THEME = [
    (0.5, 69, 1), (1.5, 71, .5), (2, 74, 2),
    (4.5, 73, 1), (5.5, 71, .5), (6, 66, 2),
    (8.5, 69, 1), (9.5, 71, .5), (10, 74, 1), (11, 76, 1),
    (12, 74, 3),
    (16.5, 78, 1), (17.5, 76, .5), (18, 74, 1.5), (19.5, 73, .5),
    (20, 69, 3),
    (24.5, 71, 1), (25.5, 74, .5), (26, 78, 1.5), (27.5, 76, .5),
    (28, 74, 1.5), (29.5, 76, .5), (30, 71, 2),
]
BRIDGE = [
    (0, 78, 2), (2, 76, 1), (3, 74, 1),
    (4, 73, 3), (7, 69, 1),
    (8, 71, 1.5), (9.5, 74, .5), (10, 78, 2),
    (12, 76, 4),
    (16, 74, 1), (17, 78, 1), (18, 83, 2),
    (20, 81, 2), (22, 78, 1), (23, 76, 1),
    (24, 74, 2), (26, 71, 1), (27, 69, 1),
    (28, 71, 2), (30, 74, 2),
]
# The bridge's second voice: a third or so under the tune, on the notes of D major.
SCALE = [62, 64, 66, 67, 69, 71, 73, 74, 76, 78, 79, 81, 83]


def hz(note):
    return 440.0 * 2.0 ** ((note - 69) / 12.0)


def seconds(beat):
    return beat * BEAT


def swung(beat):
    """Moves an off-beat eighth later, for the lazy feel."""
    whole = np.floor(beat)
    return whole + SWING if abs(beat - whole - 0.5) < 1e-6 else beat


def add_ring(buffer, beat, signal, gain=1.0, jitter=0.0):
    """Adds a signal to the buffer starting at a beat (plus a little human timing, in seconds), wrapping at the end."""
    start = int(round((seconds(beat) + jitter) * RATE))
    index = (np.arange(len(signal)) + start) % len(buffer)
    np.add.at(buffer, index, signal * gain)


def human():
    return float(rng.normal(0.0, 0.006))


def spectral(signal, gain_of_hz):
    """A zero-phase filter on the ring: multiplies the spectrum by gain_of_hz(frequencies)."""
    spectrum = np.fft.rfft(signal)
    spectrum *= gain_of_hz(np.fft.rfftfreq(len(signal), 1.0 / RATE))
    return np.fft.irfft(spectrum, len(signal))


def timeline(duration):
    return np.arange(int(duration * RATE)) / RATE


def piano_note(note, beats, velocity=1.0):
    """An electric piano: a soft FM tone whose brightness fades fast, with the tine's small bell on top."""
    duration = seconds(beats) + 0.6
    t = timeline(duration)
    f = hz(note)
    index = (0.6 + 1.4 * velocity) * np.exp(-t * 3.5)
    tone = np.sin(2 * np.pi * f * t + index * np.sin(2 * np.pi * f * t))
    tone += 0.12 * velocity * np.sin(2 * np.pi * f * 14.0 * t) * np.exp(-t * 40.0)
    tone *= 1.0 + 0.05 * np.sin(2 * np.pi * 4.2 * t)             # A slow tremolo, barely there.
    held = seconds(beats)
    release = np.clip(1.0 - (t - held) / 0.45, 0.0, 1.0)
    envelope = np.minimum(t / 0.004, 1.0) * np.exp(-t * 1.1) * release
    return tone * envelope * velocity


def lead_note(note, beats, velocity=1.0):
    """A mellow, breathy lead: a rounded sine with a slow start and a late vibrato."""
    duration = seconds(beats) * 0.96 + 0.3
    t = timeline(duration)
    f = hz(note)
    depth = 0.003 * np.clip((t - 0.35) / 0.4, 0.0, 1.0)
    phase = 2 * np.pi * np.cumsum(f * (1.0 + depth * np.sin(2 * np.pi * 4.8 * t))) / RATE
    tone = np.sin(phase) + 0.18 * np.sin(2 * phase) + 0.05 * np.sin(3 * phase)
    breath = spectral(rng.standard_normal(len(t)), lambda hertz: np.exp(-((hertz - f * 2) / 700.0) ** 2))
    tone += 0.06 * breath / np.max(np.abs(breath))
    envelope = np.minimum(t / 0.07, 1.0) * np.minimum((duration - t) / 0.28, 1.0)
    return tone * envelope * velocity


def pad_chord(chord, beats):
    duration = seconds(beats) + 1.2
    t = timeline(duration)
    tone = np.zeros(len(t))
    for note in CHORDS[chord]:
        f = hz(note)
        for detune in (-0.003, 0.0, 0.003):
            phase = rng.uniform(0, 2 * np.pi, 3)
            for harmonic, level in enumerate((1.0, 0.3, 0.1), start=1):
                tone += level * np.sin(2 * np.pi * f * (1.0 + detune) * harmonic * t + phase[harmonic - 1])
    envelope = np.minimum(t / 1.0, 1.0) * np.minimum((duration - t) / 1.2, 1.0)
    return tone * envelope


def bass_note(note, beats, velocity=1.0):
    duration = seconds(beats) + 0.08
    t = timeline(duration)
    f = hz(note)
    tone = np.sin(2 * np.pi * f * t) + 0.22 * np.sin(4 * np.pi * f * t) * np.exp(-t * 5.0)
    envelope = np.minimum(t / 0.012, 1.0) * np.minimum((duration - t) / 0.08, 1.0) * (0.75 + 0.25 * np.exp(-t * 3.0))
    return tone * envelope * velocity


def kick():
    t = timeline(0.35)
    phase = 2 * np.pi * np.cumsum(48.0 + 60.0 * np.exp(-t * 30.0)) / RATE
    return np.sin(phase) * np.exp(-t * 11.0)


def rim():
    """A soft, dark snare: brushed noise and a little body."""
    t = timeline(0.25)
    noise = spectral(rng.standard_normal(len(t)), lambda hertz: np.clip((hertz - 600.0) / 1200.0, 0.0, 1.0) * np.exp(-hertz / 4500.0))
    noise /= np.max(np.abs(noise))
    return 0.6 * noise * np.exp(-t * 16.0) + 0.35 * np.sin(2 * np.pi * 180.0 * t) * np.exp(-t * 28.0)


def hat():
    t = timeline(0.07)
    noise = spectral(rng.standard_normal(len(t)), lambda hertz: np.clip((hertz - 5000.0) / 3000.0, 0.0, 1.0) * np.exp(-hertz / 14000.0))
    noise /= np.max(np.abs(noise))
    return noise * np.exp(-t * 70.0)


def harmony_below(note):
    if note not in SCALE:
        return None
    index = SCALE.index(note) - 2
    return SCALE[index] if index >= 0 else None


def melody():
    lead = np.zeros(LENGTH)
    second = np.zeros(LENGTH)
    for section, tune, level, harmonized in ((SECTION_A, THEME, 1.0, False), (SECTION_B, BRIDGE, 1.0, True), (SECTION_A2, THEME, 0.82, False)):
        for beat, note, length in tune:
            velocity = (1.0 if beat == int(beat) else 0.88) * level
            jitter = human()
            add_ring(lead, section * 4 + swung(beat), lead_note(note, length, velocity), jitter=jitter)
            if harmonized:
                below = harmony_below(note)
                if below is not None:
                    add_ring(second, section * 4 + swung(beat), lead_note(below, length, 0.6 * velocity), jitter=jitter + 0.01)
    return lead, second


def piano():
    out = np.zeros(LENGTH)
    for bar, chord in enumerate(SONG):
        start = bar * 4
        # The chord on the one, rolled a little from the bottom, then a softer touch on the and of two.
        for hit, beats, velocity in ((0, 2.4, 0.85), (swung(2.5), 1.4, 0.55)):
            jitter = human()
            for order, note in enumerate(CHORDS[chord]):
                add_ring(out, start + hit, piano_note(note, beats, velocity * (0.9 + 0.1 * order / 3)), jitter=jitter + order * 0.011)
        if bar % 2 == 1:
            # A small answer at the end of every other bar: the top of the chord, an octave up.
            add_ring(out, start + swung(3.5), piano_note(CHORDS[chord][-1] + 12, 0.5, 0.35), jitter=human())
    return out


def pads():
    out = np.zeros(LENGTH)
    for bar, chord in enumerate(SONG):
        add_ring(out, bar * 4, pad_chord(chord, 4), 1.0 if SECTION_B <= bar < SECTION_A2 else 0.7)
    return spectral(out, lambda hertz: np.exp(-hertz / 1500.0))


def bass():
    out = np.zeros(LENGTH)
    for bar, chord in enumerate(SONG):
        root = ROOT[chord]
        start = bar * 4
        add_ring(out, start, bass_note(root, 2.3), jitter=human())
        add_ring(out, start + swung(2.5), bass_note(root, 0.9, 0.7), jitter=human())
        if bar % 4 == 3:
            add_ring(out, start + swung(3.5), bass_note(root + 7, 0.45, 0.6), jitter=human())
    return out


def drums():
    out = np.zeros(LENGTH)
    kick_sound, rim_sound, hat_sound = kick(), rim(), hat()
    for bar in range(BARS):
        start = bar * 4
        intro = bar < SECTION_A
        outro = bar >= BARS - 2
        if not intro and not outro:
            add_ring(out, start, kick_sound, jitter=human())
            add_ring(out, start + swung(2.5), kick_sound, 0.7, jitter=human())
            add_ring(out, start + 1, rim_sound, 0.55, jitter=human())
            add_ring(out, start + 3, rim_sound, 0.55, jitter=human())
        if not intro or bar >= SECTION_A // 2:
            for eighth in range(8):
                beat = eighth * 0.5
                level = 0.28 if eighth % 2 == 0 else 0.16
                if intro or outro:
                    level *= 0.7
                add_ring(out, start + swung(beat), hat_sound, level, jitter=human())
    # A darker kit than the sample's: the drums sit behind the piano.
    return spectral(out, lambda hertz: np.exp(-hertz / 7000.0))


def reverb(signal, length=2.6, decay=2.6):
    """A soft hall on the ring: the signal convolved with fading noise, wrapping at the end."""
    t = np.arange(int(length * RATE)) / RATE
    tail = rng.standard_normal(len(t)) * np.exp(-t * decay)
    tail[:int(0.02 * RATE)] = 0.0
    impulse = np.zeros(len(signal))
    impulse[:len(tail)] = tail
    wet = np.fft.irfft(np.fft.rfft(signal) * np.fft.rfft(impulse), len(signal))
    wet = spectral(wet, lambda hertz: np.clip(hertz / 300.0, 0.0, 1.0) * np.exp(-hertz / 3500.0))
    return wet / np.sqrt(np.mean(wet * wet))


def unit(signal):
    """Scales to an RMS of 1, so the mix below reads as relative loudness."""
    return signal / np.sqrt(np.mean(signal * signal))


def main():
    default = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'Projects', 'Racing', 'Assets', 'Audio', 'MenuMusic.wav')
    path = sys.argv[1] if len(sys.argv) > 1 else default

    lead, second = melody()
    tune = (lead + second) / np.sqrt(np.mean(lead * lead))
    keys = unit(piano())
    warm = unit(pads())
    low = unit(bass())
    kit = unit(drums())

    wet = reverb(0.8 * tune + 0.8 * keys + 0.3 * warm + 0.1 * kit)
    mix = 0.42 * tune + 0.62 * keys + 0.32 * warm + 0.5 * low + 0.2 * kit + 0.36 * wet
    mix = spectral(mix, lambda hertz: 1.0 / np.sqrt(1.0 + (hertz / 9000.0) ** 4))  # Rounds off the top: warm, not bright.
    mix = np.tanh(0.36 * mix)
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
