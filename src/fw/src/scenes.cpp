#include <Arduino.h>
#include <string.h>

#include "scenes.h"
#include "packed_storage.h"

#ifndef LED_kitPat
#define LED_kitPat 40
#endif
#ifndef LED_step
#define LED_step 41
#endif
#ifndef LED_function
#define LED_function 42
#endif

extern uint8_t mixVol;
extern uint8_t phonesVol;

extern SceneBank scenes;
extern uint16_t sceneValidMask[16];
extern bool sceneModeActive;
extern bool sceneMorphActive;
extern uint8_t sceneMorphPattern;
extern uint8_t sceneMorphSlot;
extern SceneState sceneMorphStartState;
extern SceneState sceneMorphTargetState;
extern uint32_t sceneMorphTicksRemaining;
extern uint32_t sceneMorphTicksTotal;
extern uint8_t sceneMorphLastMix;
extern uint8_t sceneMorphLastPhones;
extern uint8_t sceneMorphActiveSlot;
extern uint8_t sceneMorphLengthSteps;
extern uint32_t sceneMorphElapsedTicks;
extern uint16_t sceneMorphLastTickCounter;

extern volatile int16_t stepTicks;
extern volatile int16_t stepTickCounter;
extern bool saveAuto;
extern uint8_t currentPattern;
extern volatile bool flasher;
extern bool funcHeld;
extern bool currentLed[53];
extern uint8_t currentKit;
extern uint8_t opl[40][152];
extern uint8_t multBase[4];

void setMixVol();
void setPhonesVol();
void showNumber(uint8_t data);
void showMode();
void setLed(uint8_t number, bool data);

namespace {

bool previousKitLed = false;
bool previousStepLed = false;
bool previousFunctionLed = false;
uint8_t sceneMorphLastVolumes[SCENE_VOLUME_COUNT];
uint8_t sceneMorphLastMultipliers[SCENE_MULT_COUNT];
uint8_t sceneMorphLastDecays[SCENE_DECAY_COUNT];

void setSceneMultiplierValue(uint8_t index, uint8_t value) {
  opl[currentKit][index] = value;
  switch (index) {
    case 59:
      multBase[0] = value;
      break;
    case 87:
      multBase[1] = value;
      break;
    case 115:
      multBase[2] = value;
      break;
    case 143:
      multBase[3] = value;
      break;
    default:
      break;
  }
}

void renderSceneMorphLengthLeds() {
  if (!sceneModeActive) {
    return;
  }

  uint8_t steps = sceneMorphLengthSteps ? sceneMorphLengthSteps : static_cast<uint8_t>(1);
  uint8_t completedSteps = 0;

  if (sceneMorphActive && sceneMorphTicksTotal) {
    uint32_t total = sceneMorphTicksTotal;
    uint32_t elapsed = sceneMorphElapsedTicks > total ? total : sceneMorphElapsedTicks;
    completedSteps = static_cast<uint8_t>(((elapsed * steps) + total / 2) / total);
    if (completedSteps > steps) {
      completedSteps = steps;
    }
  }

  for (uint8_t i = 0; i < 16; ++i) {
    bool withinLength = i < steps;
    bool completed = i < completedSteps;
    bool lit = false;
    if (withinLength) {
      lit = completed ? flasher : true;
    }
    setLed(i, lit);
  }
}

void setSceneMorphLength(uint8_t steps, bool notify) {
  if (!steps) {
    steps = 1;
  } else if (steps > 16) {
    steps = 16;
  }

  if (sceneMorphLengthSteps != steps) {
    uint32_t ticksPerStep = stepTicks > 0 ? static_cast<uint32_t>(stepTicks) : 1u;
    uint32_t newTotal = ticksPerStep * steps;
    if (!newTotal) {
      newTotal = steps;
    }

    if (sceneMorphActive) {
      uint32_t oldTotal = sceneMorphTicksTotal ? sceneMorphTicksTotal : 1u;
      uint32_t adjustedElapsed = 0;
      if (sceneMorphTicksTotal) {
        uint64_t scaled = static_cast<uint64_t>(sceneMorphElapsedTicks) * newTotal;
        adjustedElapsed = static_cast<uint32_t>(scaled / oldTotal);
        if (adjustedElapsed > newTotal) {
          adjustedElapsed = newTotal;
        }
      }
      sceneMorphTicksTotal = newTotal;
      sceneMorphElapsedTicks = adjustedElapsed;
      sceneMorphTicksRemaining = newTotal - adjustedElapsed;
      sceneMorphLastTickCounter = stepTickCounter;
    }

    sceneMorphLengthSteps = steps;
  }

  renderSceneMorphLengthLeds();
  if (notify) {
    showNumber(sceneMorphLengthSteps);
  }
}

SceneState captureCurrentScene() {
  SceneState state{};
  state.mixVol = constrain(mixVol, 0, static_cast<uint8_t>(9));
  state.phonesVol = constrain(phonesVol, 0, static_cast<uint8_t>(9));

  for (uint8_t i = 0; i < SCENE_VOLUME_COUNT; ++i) {
    uint8_t index = SCENE_VOLUME_INDICES[i];
    state.volumeFaders[i] = constrain(opl[currentKit][index], 0, static_cast<uint8_t>(15));
  }

  for (uint8_t i = 0; i < SCENE_MULT_COUNT; ++i) {
    uint8_t index = SCENE_MULT_INDICES[i];
    state.multiplierFaders[i] = constrain(opl[currentKit][index], 0, static_cast<uint8_t>(15));
  }

  for (uint8_t i = 0; i < SCENE_DECAY_COUNT; ++i) {
    uint8_t index = SCENE_DECAY_INDICES[i];
    state.decayFaders[i] = constrain(opl[currentKit][index], 0, static_cast<uint8_t>(15));
  }

  return state;
}

void applySceneState(const SceneState& state) {
  mixVol = constrain(state.mixVol, 0, static_cast<uint8_t>(9));
  phonesVol = constrain(state.phonesVol, 0, static_cast<uint8_t>(9));
  for (uint8_t i = 0; i < SCENE_VOLUME_COUNT; ++i) {
    uint8_t index = SCENE_VOLUME_INDICES[i];
    uint8_t value = constrain(state.volumeFaders[i], 0, static_cast<uint8_t>(15));
    opl[currentKit][index] = value;
    sceneMorphLastVolumes[i] = value;
  }

  for (uint8_t i = 0; i < SCENE_MULT_COUNT; ++i) {
    uint8_t index = SCENE_MULT_INDICES[i];
    uint8_t value = constrain(state.multiplierFaders[i], 0, static_cast<uint8_t>(15));
    setSceneMultiplierValue(index, value);
    sceneMorphLastMultipliers[i] = value;
  }

  for (uint8_t i = 0; i < SCENE_DECAY_COUNT; ++i) {
    uint8_t index = SCENE_DECAY_INDICES[i];
    uint8_t value = constrain(state.decayFaders[i], 0, static_cast<uint8_t>(15));
    opl[currentKit][index] = value;
    sceneMorphLastDecays[i] = value;
  }

  setMixVol();
  setPhonesVol();
  showMode();
}

void refreshStatusLeds() {
  setLed(LED_kitPat, 1);
  setLed(LED_step, 1);
  setLed(LED_function, previousFunctionLed);
}

}  // namespace

void enterSceneMode() {
  if (sceneModeActive) {
    return;
  }

  previousKitLed = currentLed[LED_kitPat];
  previousStepLed = currentLed[LED_step];
  previousFunctionLed = currentLed[LED_function];

  sceneModeActive = true;
  refreshStatusLeds();
  renderSceneModeLeds();
}

void exitSceneMode() {
  if (!sceneModeActive) {
    return;
  }

  sceneModeActive = false;
  setLed(LED_kitPat, previousKitLed);
  setLed(LED_step, previousStepLed);
  setLed(LED_function, previousFunctionLed);
  sceneMorphActiveSlot = 0xFF;
  renderSceneModeLeds();
  showMode();
}

bool captureScene(uint8_t pattern, uint8_t slot) {
  if (pattern > 15 || slot > 9) {
    return false;
  }

  SceneState snapshot = captureCurrentScene();
  scenes[pattern][slot] = snapshot;
  sceneValidMask[pattern] |= static_cast<uint16_t>(1 << slot);
  saveAuto = true;
  if (sceneModeActive) {
    renderSceneModeLeds();
  }
  return true;
}

bool scheduleSceneMorph(uint8_t pattern, uint8_t slot) {
  if (pattern > 15 || slot > 9) {
    return false;
  }

  if (!(sceneValidMask[pattern] & (1u << slot))) {
    return false;
  }

  SceneState target = scenes[pattern][slot];
  SceneState start = captureCurrentScene();

  sceneMorphActive = true;
  sceneMorphPattern = pattern;
  sceneMorphSlot = slot;
  sceneMorphActiveSlot = slot;
  sceneMorphStartState = start;
  sceneMorphTargetState = target;
  sceneMorphLastMix = start.mixVol;
  sceneMorphLastPhones = start.phonesVol;
  memcpy(sceneMorphLastVolumes, sceneMorphStartState.volumeFaders, sizeof(sceneMorphLastVolumes));
  memcpy(sceneMorphLastMultipliers,
         sceneMorphStartState.multiplierFaders,
         sizeof(sceneMorphLastMultipliers));
  memcpy(sceneMorphLastDecays,
         sceneMorphStartState.decayFaders,
         sizeof(sceneMorphLastDecays));

  uint32_t ticksPerStep = stepTicks > 0 ? static_cast<uint32_t>(stepTicks) : 1u;
  uint8_t steps = sceneMorphLengthSteps ? sceneMorphLengthSteps : static_cast<uint8_t>(1);
  uint32_t totalTicks = ticksPerStep * steps;
  if (!totalTicks) {
    totalTicks = steps;
  }

  sceneMorphTicksTotal = totalTicks;
  sceneMorphElapsedTicks = 0;
  sceneMorphTicksRemaining = totalTicks;
  sceneMorphLastTickCounter = stepTickCounter;

  if (sceneModeActive) {
    renderSceneModeLeds();
  }
  return true;
}

void handleSceneMorph() {
  if (!sceneMorphActive) {
    return;
  }

  uint32_t total = sceneMorphTicksTotal ? sceneMorphTicksTotal : 1u;

  uint16_t ticksPerStep = stepTicks > 0 ? static_cast<uint16_t>(stepTicks) : static_cast<uint16_t>(1);
  uint16_t currentTicks = static_cast<uint16_t>(stepTickCounter);
  uint16_t lastTicks = sceneMorphLastTickCounter;
  sceneMorphLastTickCounter = currentTicks;

  if (sceneMorphTicksRemaining) {
    uint32_t deltaTicks = 0;
    if (currentTicks >= lastTicks) {
      deltaTicks = currentTicks - lastTicks;
    } else {
      uint16_t cappedLast = lastTicks;
      if (cappedLast > ticksPerStep) {
        cappedLast = ticksPerStep;
      }
      deltaTicks = static_cast<uint32_t>(ticksPerStep - cappedLast) + currentTicks;
    }

    if (deltaTicks) {
      uint32_t elapsedAccum = sceneMorphElapsedTicks + deltaTicks;
      if (elapsedAccum >= total) {
        sceneMorphElapsedTicks = total;
        sceneMorphTicksRemaining = 0;
      } else {
        sceneMorphElapsedTicks = elapsedAccum;
        sceneMorphTicksRemaining = total - elapsedAccum;
      }
    }
  }

  if (!sceneMorphTicksRemaining) {
    applySceneState(sceneMorphTargetState);
    sceneMorphActive = false;
    sceneMorphActiveSlot = 0xFF;
    if (sceneModeActive) {
      renderSceneModeLeds();
    }
    return;
  }

  uint32_t elapsed = sceneMorphElapsedTicks;

  int16_t mixDelta = static_cast<int16_t>(sceneMorphTargetState.mixVol) -
                     static_cast<int16_t>(sceneMorphStartState.mixVol);
  int16_t phoneDelta = static_cast<int16_t>(sceneMorphTargetState.phonesVol) -
                       static_cast<int16_t>(sceneMorphStartState.phonesVol);

  uint8_t nextMix = sceneMorphStartState.mixVol +
                    static_cast<int32_t>(mixDelta * static_cast<int32_t>(elapsed) + (total / 2)) / total;
  uint8_t nextPhones = sceneMorphStartState.phonesVol +
                       static_cast<int32_t>(phoneDelta * static_cast<int32_t>(elapsed) + (total / 2)) / total;

  nextMix = constrain(nextMix, 0, static_cast<uint8_t>(9));
  nextPhones = constrain(nextPhones, 0, static_cast<uint8_t>(9));

  if (nextMix != sceneMorphLastMix) {
    mixVol = nextMix;
    sceneMorphLastMix = nextMix;
    setMixVol();
  }

  if (nextPhones != sceneMorphLastPhones) {
    phonesVol = nextPhones;
    sceneMorphLastPhones = nextPhones;
    setPhonesVol();
  }

  for (uint8_t i = 0; i < SCENE_VOLUME_COUNT; ++i) {
    int16_t delta = static_cast<int16_t>(sceneMorphTargetState.volumeFaders[i]) -
                    static_cast<int16_t>(sceneMorphStartState.volumeFaders[i]);
    int32_t scaled = static_cast<int32_t>(delta) * static_cast<int32_t>(elapsed) + (total / 2);
    int16_t interpolated = static_cast<int16_t>(sceneMorphStartState.volumeFaders[i]) +
                           static_cast<int16_t>(scaled / static_cast<int32_t>(total));
    uint8_t nextVolume = constrain(interpolated, 0, static_cast<int16_t>(15));
    if (nextVolume != sceneMorphLastVolumes[i]) {
      sceneMorphLastVolumes[i] = nextVolume;
      opl[currentKit][SCENE_VOLUME_INDICES[i]] = nextVolume;
    }
  }

  for (uint8_t i = 0; i < SCENE_MULT_COUNT; ++i) {
    int16_t delta = static_cast<int16_t>(sceneMorphTargetState.multiplierFaders[i]) -
                    static_cast<int16_t>(sceneMorphStartState.multiplierFaders[i]);
    int32_t scaled = static_cast<int32_t>(delta) * static_cast<int32_t>(elapsed) + (total / 2);
    int16_t interpolated = static_cast<int16_t>(sceneMorphStartState.multiplierFaders[i]) +
                           static_cast<int16_t>(scaled / static_cast<int32_t>(total));
    uint8_t nextMultiplier = constrain(interpolated, 0, static_cast<int16_t>(15));
    if (nextMultiplier != sceneMorphLastMultipliers[i]) {
      sceneMorphLastMultipliers[i] = nextMultiplier;
      setSceneMultiplierValue(SCENE_MULT_INDICES[i], nextMultiplier);
    }
  }

  for (uint8_t i = 0; i < SCENE_DECAY_COUNT; ++i) {
    int16_t delta = static_cast<int16_t>(sceneMorphTargetState.decayFaders[i]) -
                    static_cast<int16_t>(sceneMorphStartState.decayFaders[i]);
    int32_t scaled = static_cast<int32_t>(delta) * static_cast<int32_t>(elapsed) + (total / 2);
    int16_t interpolated = static_cast<int16_t>(sceneMorphStartState.decayFaders[i]) +
                           static_cast<int16_t>(scaled / static_cast<int32_t>(total));
    uint8_t nextDecay = constrain(interpolated, 0, static_cast<int16_t>(15));
    if (nextDecay != sceneMorphLastDecays[i]) {
      sceneMorphLastDecays[i] = nextDecay;
      opl[currentKit][SCENE_DECAY_INDICES[i]] = nextDecay;
    }
  }

  if (!sceneMorphTicksRemaining) {
    applySceneState(sceneMorphTargetState);
    sceneMorphActive = false;
    sceneMorphActiveSlot = 0xFF;
    if (sceneModeActive) {
      renderSceneModeLeds();
    }
  }
}

void renderSceneModeLeds() {
  if (!sceneModeActive) {
    return;
  }

  for (uint8_t i = 0; i < 10; ++i) {
    bool slotValid = (sceneValidMask[currentPattern] & (1u << i)) != 0;
    bool slotActive = sceneMorphActive && sceneMorphPattern == currentPattern && sceneMorphActiveSlot == i;

    if (slotActive) {
      setLed(16 + i, flasher);
    } else if (slotValid) {
      setLed(16 + i, 1);
    } else {
      setLed(16 + i, 0);
    }
  }

  renderSceneMorphLengthLeds();
}

void sceneHandleTopButton(uint8_t slot, bool value) {
  if (slot > 9) {
    return;
  }

  if (value) {
    return;
  }

  if (funcHeld) {
    captureScene(currentPattern, slot);
  } else {
    scheduleSceneMorph(currentPattern, slot);
  }
}

void maintainSceneModeStatusLeds() {
  if (!sceneModeActive) {
    return;
  }
  refreshStatusLeds();
  renderSceneMorphLengthLeds();
}

void sceneHandleBottomButton(uint8_t slot, bool value) {
  if (!sceneModeActive || slot > 15) {
    return;
  }

  if (value) {
    renderSceneMorphLengthLeds();
    return;
  }

  setSceneMorphLength(slot + 1, true);
}
