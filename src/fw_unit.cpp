// The BlastBeats firmware (unchanged, vendored under fw/) built as one unit, as PlatformIO does.
// Target: the STM32F4 (R2) variant. setup()/loop() are renamed so the plugin can call them.
#define STM32F4 1
#define ARDUINO_ARCH_STM32 1
#define SDFAT_FILE_TYPE 1
#define setup bb_setup
#define loop bb_loop
#include "fw/src/main.cpp"
#undef setup
#undef loop
#include "fw/src/pattern_visualizer.cpp"
#include "fw/src/scenes.cpp"
#include "fw/src/midi_sysex.cpp"
#include "fw/src/vgmStream.cpp"
#include "fw/src/sd_compat.cpp"

// ---- read-only views of the firmware's state for the plugin's screen, and the SD flush (same unit as the globals)
#include "machine.h"
namespace machine {
int drum_note(int i) { return (i >= 0 && i < 6) ? drumNotes[i] : 0; }
int midi_channel(int i) { switch (i) { case 0: return ch1; case 1: return ch2; case 2: return ch3; case 3: return ch4; default: return ch10; } }

// The value (0..15, what movedFader() stores) the firmware holds for fader number n of the current kit, synth and drum voice;
// -1 where a fader has no stored value. Mirrors the switch in fader.h so the panel can show a loaded kit's settings.
int fader_level(int n) {
  int k = currentKit;
  int off = selectedSynth == 1 ? 28 : selectedSynth == 2 ? 56 : selectedSynth == 3 ? 84 : 0;
  int v = selectedVoice;
  int idx = -1;
  switch (n) {
    case 0: idx = 1; break; case 1: idx = 2; break; case 2: idx = 3; break; case 3: idx = 4; break;
    case 4: idx = 5; break; case 5: idx = 6; break; case 6: idx = 7; break; case 14: idx = 8; break;
    case 15: idx = 9; break; case 16: idx = 10; break; case 17: idx = 11; break; case 18: idx = 12; break;
    case 19: idx = 13; break; case 20: idx = 14; break;
    case 28: idx = 15; break; case 29: idx = 16; break; case 30: idx = 19; break; case 31: idx = 17; break;
    case 32: idx = 18; break; case 33: idx = 20; break; case 34: idx = 21; break;
    case 42: idx = v == 5 ? 22 : v == 2 ? 23 : 24; break;
    case 43: idx = v == 2 ? 25 : 26; break;
    case 44: idx = v == 2 ? 29 : 148; break;
    case 45: idx = v == 2 ? 27 : 149; break;
    case 46: idx = v == 2 ? 28 : 150; break;
    case 47: idx = v == 5 ? 30 : v == 2 ? 31 : 32; break;
    case 48: idx = v == 5 ? 33 : v == 2 ? 34 : 35; break;
    case 7: idx = off + 36; break; case 8: idx = off + 37; break; case 9: idx = off + 38; break; case 10: idx = off + 39; break;
    case 11: idx = off + 40; break; case 12: idx = off + 41; break; case 13: idx = off + 42; break; case 21: idx = off + 43; break;
    case 22: idx = off + 44; break; case 23: idx = off + 45; break; case 24: idx = off + 46; break; case 25: idx = off + 47; break;
    case 26: idx = off + 48; break; case 27: idx = off + 49; break; case 35: idx = off + 50; break; case 36: idx = off + 51; break;
    case 37: idx = off + 52; break; case 38: idx = off + 53; break; case 39: idx = off + 54; break; case 40: idx = off + 55; break;
    case 41: idx = off + 56; break; case 49: idx = off + 57; break; case 50: idx = off + 58; break; case 51: idx = off + 59; break;
    case 52: idx = off + 60; break; case 53: idx = off + 61; break; case 54: idx = off + 62; break; case 55: idx = off + 63; break;
  }
  if (idx < 0) return -1;
  int x = opl[k][idx];
  return x < 0 ? 0 : x > 15 ? 15 : x;
}
Snap snapshot() {
  Snap s;
  s.mode = mode;
  s.run = ::run;        // the firmware global, not machine::run()
  s.rec = rec;
  s.function = function;
  s.voice = selectedVoice;
  s.synth = selectedSynth;
  s.kit = currentKit;
  s.pattern = currentPattern;
  s.tempo = tempo;
  s.page = selectedPage;
  s.song = currentSong;
  s.folder = currentFolder;
  s.step = polyStep[0];
  s.ploc_step = plocStep;
  s.midi_slave = midiSlave;
  s.analog_slave = analogSlave;
  return s;
}
// A new machine has an empty card. The firmware's own load() leaves every pattern one step long when there are no
// song files, so a first start does what a factory card would: every pattern cleared (clearPattern(), the firmware's
// own routine) with its 16 steps, tempo 120.
void init_blank_card() {
  for (int p = 0; p < 16; p++) {
    currentPattern = p;
    clearPattern();
    for (int v = 0; v < 10; v++) lastStepPoly[p][v] = 15;   // (clearPattern() sets only one voice's: see fw/VENDORED.md)
  }
  currentPattern = 0;
  markMaxStepCacheDirty();
  tempo = 120;
  uClock.setTempo(tempo);
  EEPROMupdate(26, 120);
  EEPROMupdate(27, 0);
}
// What pressing SAVE does for the working song and every kit: so a project keeps unsaved edits (see engine.cc).
void flush_to_sd() {
  save();
  saveKit();
}
}
