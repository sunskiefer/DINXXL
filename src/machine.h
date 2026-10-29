// Runs the BlastBeats firmware on the virtual hardware.
#pragma once
#include <stdint.h>
namespace machine {
struct Snap {   // the firmware's own variables, as the panel's screen shows them
  int mode = 0, function = -1, voice = 0, synth = 0, kit = 0, pattern = 0, tempo = 0, page = 0, song = 0, folder = 0;
  bool run = false, rec = false;
  int step = 0, ploc_step = 0;
  bool midi_slave = false, analog_slave = false;  // plocStep: the step key being held for a parameter lock (0 = none)                  // polyStep[0], the sequencer's position
};
Snap snapshot();
int drum_note(int i);              // the firmware's MIDI note for drum i (0 BD, 1 SD, 2 tom, 3 CH, 4 OH, 5 cymbal)
int midi_channel(int i);           // 1-based MIDI channel: i 0..3 = synth 1..4, 4 = drums
void init_blank_card();            // first start only: the pattern memory a factory card would give
void flush_to_sd();                // the firmware's save() and saveKit()
void power_on(uint32_t seed);      // hw reset + the firmware's setup() (SD card is kept)
void run(uint32_t us);             // advance the machine by this much virtual time
int fader_level(int fw_fader);      // stored 0..15 value of a fader for the current kit/synth/voice, -1 if none
void sync_faders();                // tell the firmware where the faders are now (no kit change)
}
