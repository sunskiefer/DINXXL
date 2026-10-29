#pragma once

#include <stdint.h>

enum : uint8_t {
  SCENE_VOLUME_COUNT = 22,
  SCENE_MULT_COUNT = 17,
  SCENE_DECAY_COUNT = 1,
};

struct SceneState {
  uint8_t mixVol;
  uint8_t phonesVol;
  uint8_t volumeFaders[SCENE_VOLUME_COUNT];
  uint8_t multiplierFaders[SCENE_MULT_COUNT];
  uint8_t decayFaders[SCENE_DECAY_COUNT];
};

static const uint8_t SCENE_VOLUME_INDICES[SCENE_VOLUME_COUNT] = {
  1,   // kick
  9,   // snare
  15,  // clap/perc
  22,  // cymbal
  23,  // tom
  24,  // hats
  36,  43,  50,  57,  // synth voice 1 operators
  64,  71,  78,  85,  // synth voice 2 operators
  92,  99,  106, 113, // synth voice 3 operators
  120, 127, 134, 141  // synth voice 4 operators
};

static const uint8_t SCENE_MULT_INDICES[SCENE_MULT_COUNT] = {
  4,    // kick multiplier
  11,   // snare multiplier
  19,   // clap/perc multiplier
  29,   // tom/alt drum multiplier
  40,   // synth voice 1 operator multiplier
  54,   // synth voice 1 operator multiplier (carrier)
  59,   // synth voice 1 operator multiplier (modulated)
  68,   // synth voice 2 operator multiplier
  82,   // synth voice 2 operator multiplier (carrier)
  87,   // synth voice 2 operator multiplier (modulated)
  96,   // synth voice 3 operator multiplier
  110,  // synth voice 3 operator multiplier (carrier)
  115,  // synth voice 3 operator multiplier (modulated)
  124,  // synth voice 4 operator multiplier
  138,  // synth voice 4 operator multiplier (carrier)
  143,  // synth voice 4 operator multiplier (modulated)
  148   // alternate voice multiplier
};

static const uint8_t SCENE_DECAY_INDICES[SCENE_DECAY_COUNT] = {
  8,  // drum voice decay
};
