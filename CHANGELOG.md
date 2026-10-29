# Changelog

## 1.0.0 (2026-10-29)
First release. The BlastBeats v7.1 firmware (Twisted Electrons, commit 9b562d0) running unmodified on a software model of
its board, with the ymfm YMF262 core as the sound chip, and BlastBeats' factory card (kits, patterns, songs) inside.
- Three pages: KEYS (mode, voice and step keys with the firmware's lamps, HOLD, PADS = VOICES and the screen), DRUMS (28
  faders), SYNTH (28 faders for the selected synth). The faders follow the loaded kit, synth and drum voice.
- Screen: Home (kit, bank, pattern, tempo, mode, voice), fader pages with waveform pictures and algorithm diagrams, key
  names, held keys; returns Home by itself.
- Keys: every key is a 12 ms tap; HOLD keeps the next voice or step key down for combinations (p-locks).
- Force integration: follows the transport and tempo clock, MIDI ch 10 drums / ch 1-4 synths, 16 pads = 16 voices.
- State: the card is saved with the project as a difference from the factory card.
- `card/dinxxl.log` diagnostics (CPU, worst block, key and MIDI counts).
- Force (MPC OS 3.x, armv7) only. Not yet: PC editor, SysEx, voice keys on the fader pages.
