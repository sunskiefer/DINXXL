#include <Arduino.h>

#include "pattern_visualizer.h"
#include "midi_sysex.h"
#include "web_tool_protocol.h"

extern uint8_t currentPattern;
extern int16_t pages[16][10];
extern uint8_t pattern[16][2][64];
extern uint8_t synth[16][4][64];
extern bool patternVisualizerDirty;
extern uint32_t patternVisualizerLastSendMs;
extern uint32_t patternVisualizerLastActivityMs;
extern uint8_t selectedVoice;
extern uint8_t selectedPage;
extern int offsetOct[16][10];
extern bool saveAuto;
extern volatile bool run;

extern bool toolMode;
extern void showPattern();
extern void setStep(uint8_t voice, int16_t stepNumber, bool value);
extern void setNote(uint8_t voice, uint8_t stepNumber, uint8_t value);
extern void settingsSpit();
extern void wipe();
extern void applyToolModeSetting(uint8_t note, uint8_t value);

namespace {
constexpr uint8_t kDrumVoices = 6;
constexpr uint8_t kSynthVoices = 4;
constexpr uint8_t kSteps = 64;
constexpr uint32_t kMinSendIntervalMs = 30;
constexpr int8_t kOffsetBias = 8;

constexpr size_t kPayloadSize = 5                 // F0 + manufacturer + message type
                               + 1                // pattern number
                               + 2                // selected voice/page
                               + 10               // per-voice page counts
                               + kSteps           // drum column bitmasks
                               + kSteps           // synth step bitmasks
                               + (kSynthVoices * kSteps)  // synth note values
                               + kSynthVoices     // per-voice octave offsets
                               + 1;               // F7

uint8_t messageBuffer[kPayloadSize]{};

bool patternVisualizerSessionActive() {
  return patternVisualizerLastActivityMs != 0;
}

void touchPatternVisualizerSession() {
  patternVisualizerLastActivityMs = millis();
  if (!patternVisualizerLastActivityMs) {
    patternVisualizerLastActivityMs = 1;
  }
}

uint8_t clampStep(uint8_t step) {
  return static_cast<uint8_t>(step & 0x3F);
}

bool synthStepActive(uint8_t voice, uint8_t step) {
  step = clampStep(step);
  switch (voice) {
    case 0: return bitRead(pattern[currentPattern][0][step], 6);
    case 1: return bitRead(pattern[currentPattern][0][step], 7);
    case 2: return bitRead(pattern[currentPattern][1][step], 0);
    case 3: return bitRead(pattern[currentPattern][1][step], 1);
    default: return false;
  }
}

uint8_t synthNoteValue(uint8_t voice, uint8_t step) {
  step = clampStep(step);
  uint8_t value = 0;
  switch (voice) {
    case 0:
      for (uint8_t bit = 0; bit < 6; ++bit) {
        bitWrite(value, bit, bitRead(synth[currentPattern][0][step], bit));
      }
      break;
    case 1:
      bitWrite(value, 0, bitRead(synth[currentPattern][0][step], 6));
      bitWrite(value, 1, bitRead(synth[currentPattern][0][step], 7));
      for (uint8_t bit = 0; bit < 4; ++bit) {
        bitWrite(value, bit + 2, bitRead(synth[currentPattern][1][step], bit));
      }
      break;
    case 2:
      for (uint8_t bit = 0; bit < 4; ++bit) {
        bitWrite(value, bit, bitRead(synth[currentPattern][1][step], bit + 4));
      }
      bitWrite(value, 4, bitRead(synth[currentPattern][2][step], 0));
      bitWrite(value, 5, bitRead(synth[currentPattern][2][step], 1));
      break;
    case 3:
      for (uint8_t bit = 0; bit < 6; ++bit) {
        bitWrite(value, bit, bitRead(synth[currentPattern][2][step], bit + 2));
      }
      break;
    default:
      break;
  }
  return static_cast<uint8_t>(value & 0x3F);
}

void applyRemoteDrumStep(uint8_t voice, uint8_t step, bool active) {
  if (voice >= kDrumVoices) {
    return;
  }
  step = clampStep(step);

  const bool previous = bitRead(pattern[currentPattern][0][step], voice);
  if (previous == active) {
    return;
  }

  bitWrite(pattern[currentPattern][0][step], voice, active ? 1 : 0);
  saveAuto = true;
  ::markPatternVisualizerDirty();
  showPattern();
}

void applyRemoteSynthStep(uint8_t voice, uint8_t step, bool active, uint8_t rawNote) {
  if (voice >= kSynthVoices) {
    return;
  }
  step = clampStep(step);
  rawNote &= 0x3F;

  if (active) {
    setStep(voice, step, true);
    setNote(voice, step, rawNote);
  } else {
    setStep(voice, step, false);
  }

  saveAuto = true;
  ::markPatternVisualizerDirty();
  showPattern();
}
}  // namespace

void markPatternVisualizerDirty() {
  patternVisualizerDirty = true;
}

void sendPatternVisualizerSnapshot() {
  size_t index = 0;
  messageBuffer[index++] = 0xF0;
  messageBuffer[index++] = WebToolProtocol::kManufacturerId;
  messageBuffer[index++] = WebToolProtocol::kProductId0;
  messageBuffer[index++] = WebToolProtocol::kProductId1;
  messageBuffer[index++] = WebToolProtocol::kMsgPatternSnapshot;
  messageBuffer[index++] = currentPattern & 0x7F;
  messageBuffer[index++] = selectedVoice & 0x0F;
  messageBuffer[index++] = selectedPage & 0x07;

  for (uint8_t voice = 0; voice < 10; ++voice) {
    messageBuffer[index++] = pages[currentPattern][voice] & 0x7F;
  }

  for (uint8_t step = 0; step < kSteps; ++step) {
    uint8_t drumMask = 0;
    for (uint8_t voice = 0; voice < kDrumVoices; ++voice) {
      if (bitRead(pattern[currentPattern][0][step], voice)) {
        drumMask = static_cast<uint8_t>(drumMask | (1u << voice));
      }
    }
    messageBuffer[index++] = drumMask;
  }

  for (uint8_t step = 0; step < kSteps; ++step) {
    uint8_t synthMask = 0;
    for (uint8_t voice = 0; voice < kSynthVoices; ++voice) {
      if (synthStepActive(voice, step)) {
        synthMask = static_cast<uint8_t>(synthMask | (1u << voice));
      }
    }
    messageBuffer[index++] = synthMask;
  }

  for (uint8_t voice = 0; voice < kSynthVoices; ++voice) {
    for (uint8_t step = 0; step < kSteps; ++step) {
      const uint8_t note = synthNoteValue(voice, step);
      messageBuffer[index++] = note;
    }
  }

  for (uint8_t voice = 0; voice < kSynthVoices; ++voice) {
    int8_t offset = static_cast<int8_t>(offsetOct[currentPattern][kDrumVoices + voice]);
    if (offset < -kOffsetBias) { offset = -kOffsetBias; }
    if (offset > (0x7F - kOffsetBias)) { offset = static_cast<int8_t>(0x7F - kOffsetBias); }
    messageBuffer[index++] = static_cast<uint8_t>(offset + kOffsetBias);
  }

  messageBuffer[index++] = 0xF7;

  if (sendBlastBeatsSysex(messageBuffer, index)) {
    touchPatternVisualizerSession();
    patternVisualizerLastSendMs = millis();
    patternVisualizerDirty = false;
  }
}

void patternVisualizerSendStepPositions(const uint8_t* steps, size_t count) {
  if (!steps || count == 0) {
    return;
  }
  if (!patternVisualizerSessionActive()) {
    return;
  }

  const size_t voiceCount = kDrumVoices + kSynthVoices;
  size_t payloadCount = (count > voiceCount) ? voiceCount : count;

  size_t index = 0;
  messageBuffer[index++] = 0xF0;
  messageBuffer[index++] = WebToolProtocol::kManufacturerId;
  messageBuffer[index++] = WebToolProtocol::kProductId0;
  messageBuffer[index++] = WebToolProtocol::kProductId1;
  messageBuffer[index++] = WebToolProtocol::kMsgStepPositions;

  for (size_t i = 0; i < payloadCount; ++i) {
    messageBuffer[index++] = steps[i] & 0x7F;
  }

  messageBuffer[index++] = 0xF7;
  if (sendBlastBeatsSysex(messageBuffer, index)) {
    touchPatternVisualizerSession();
  }
}

bool handlePatternVisualizerSysEx(const uint8_t* data, size_t length) {
  if (!data || length < 6) {
    return false;
  }
  if (data[0] != 0xF0 || data[length - 1] != 0xF7) {
    return false;
  }
  if (data[1] != WebToolProtocol::kManufacturerId ||
      data[2] != WebToolProtocol::kProductId0 ||
      data[3] != WebToolProtocol::kProductId1) {
    return false;
  }

  const uint8_t type = data[4];
  switch (type) {
    case WebToolProtocol::kMsgPatternRequest:
      touchPatternVisualizerSession();
      markPatternVisualizerDirty();
      return true;
    case WebToolProtocol::kMsgStepPositions:
      // Currently unused; ignore gracefully.
      return true;
    case WebToolProtocol::kMsgSetDrumStep:
      touchPatternVisualizerSession();
      if (length >= 9) {
        const uint8_t voice = data[5] & 0x7F;
        const uint8_t step = data[6] & 0x7F;
        const bool active = (data[7] & 0x01) != 0;
        applyRemoteDrumStep(voice, step, active);
      }
      return true;
    case WebToolProtocol::kMsgSetSynthStep:
      touchPatternVisualizerSession();
      if (length >= 10) {
        const uint8_t voice = data[5] & 0x7F;
        const uint8_t step = data[6] & 0x7F;
        const bool active = (data[7] & 0x01) != 0;
        const uint8_t rawNote = data[8] & 0x7F;
        applyRemoteSynthStep(voice, step, active, rawNote);
      }
      return true;
    case WebToolProtocol::kMsgSettingsRequest:
      toolMode = true;
      settingsSpit();
      wipe();
      return true;
    case WebToolProtocol::kMsgSettingsUpdate: {
      toolMode = true;
      if (length >= 7) {
        const uint8_t pairCount = data[5] & 0x7F;
        size_t index = 6;
        for (uint8_t i = 0; i < pairCount; ++i) {
          if ((index + 1) >= (length - 1)) {
            break;
          }
          const uint8_t note = data[index++] & 0x7F;
          const uint8_t value = data[index++] & 0x7F;
          applyToolModeSetting(note, value);
        }
      }
      return true;
    }
    case WebToolProtocol::kMsgSettingsDump:
      // Host-only message; ignore on device.
      return true;
    default:
      return false;
  }
}

void patternVisualizerTick() {
  static uint8_t lastSelectedVoice = 0xFF;
  static uint8_t lastSelectedPage = 0xFF;

  if (selectedVoice != lastSelectedVoice || selectedPage != lastSelectedPage) {
    lastSelectedVoice = selectedVoice;
    lastSelectedPage = selectedPage;
    patternVisualizerDirty = true;
  }

  if (!patternVisualizerSessionActive()) {
    return;
  }
  if (!patternVisualizerDirty) {
    return;
  }
  if (run) {
    return;
  }
  const uint32_t now = millis();
  if ((now - patternVisualizerLastSendMs) < kMinSendIntervalMs) {
    return;
  }
  sendPatternVisualizerSnapshot();
}
