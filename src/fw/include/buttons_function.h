#include <stddef.h>
#include <string.h>

#include "pattern_visualizer.h"
#include "seq.h"

// Small helpers shared across button handlers
static inline bool finalizeFunctionChange(void) {
  blink();
  showMode();
  noFuncAction = false;
  return true;
}

static inline void setFunctionAndShow(uint8_t newFunction) {
  function = newFunction;
  showMode();
}

template <typename ArrayType, size_t Length>
static inline void clearPatternRow(ArrayType (&row)[Length]) {
  memset(row, 0, sizeof(row));
}

// Bottom row helpers

static inline bool bottomHandleTempoHeldToggle(uint8_t number) {
  if (lastStepMode || !tempoHeld || number != FUNC_STUTTER) {
    return false;
  }

  drill = !drill;
  blink();
  tempoTaps = 0;
  return true;
}

static inline bool bottomHandleFuncHeldToggle(uint8_t number) {
  if (lastStepMode || !funcHeld) {
    return false;
  }

  switch (number) {
    case FUNC_STUTTER:
      stutterEnabled = !stutterEnabled;
      if (!stutterEnabled || stutter == 2) {
        stutter = 0;
      }
      return finalizeFunctionChange();

    case FUNC_BEND:
      autoBendEnabled = !autoBendEnabled;

      if (!autoBendEnabled && bendStepLast) {
        stopBend();
        bendStepLast = 0;
      }

      return finalizeFunctionChange();

    case FUNC_FREEZE:
      autoFreezeEnabled = !autoFreezeEnabled;

      if (!autoFreezeEnabled && freeze) {
        opl2.freeze(0);
        freeze = false;
        resetRegisters = true;
      }

      return finalizeFunctionChange();

    default:
      return false;
  }
}

static inline bool bottomHandleStutterMode(uint8_t number) {
  if (function != FUNC_STUTTER || !stutter) {
    return false;
  }

  wiggleHeld = number == FUNC_WIGGLE;

  if (number == FUNC_BEND) {
    bendy = 1 + number;
    startBend();
  }

  return true;
}

static inline bool bottomHandleLastStepMode(uint8_t number) {
  if (!lastStepMode) {
    return false;
  }

  auto *lastStepRow = lastStepPoly[currentPattern];
  auto *pageRow = pages[currentPattern];

  if (!funcHeld) {
    for (uint8_t voice = 0; voice < 10; ++voice) {
      lastStepRow[voice] = number;
      const uint8_t maxIndex = static_cast<uint8_t>((pageRow[voice] << 4) + number);
      if (polyStep[voice] > maxIndex) {
        polyStep[voice] = 0;
      }
    }
  } else {
    noFuncAction = false;
    lastStepRow[selectedVoice] = number;
    const uint8_t maxIndex = static_cast<uint8_t>((pageRow[selectedVoice] << 4) + number);
    if (polyStep[selectedVoice] > maxIndex) {
      polyStep[selectedVoice] = 0;
    }
  }

  markMaxStepCacheDirty();
  showMode();
  return true;
}

static inline bool bottomHandleSaveMode(uint8_t number) {
  if (function != FUNC_SAVE) {
    return false;
  }

  saveToNewSlot = currentSong != number;
  if (saveToNewSlot) {
    currentSong = number;
    EEPROMupdate(16, currentSong);
  }

  if (run) {
    saveFlag = true;
  } else {
#if defined(STM32F1) || defined(STM32F4)
    if (EEPROMChanged) {
      EEPROMSave();
    }
#endif
    save();
  }

  return true;
}

static inline void bottomSelectSaveFunction(void) {
  function = FUNC_SAVE;
}

static inline void bottomHandleLengthButton(void) {
  if (wiggleHeld) {
    wiggleNudge = !wiggleNudge;
    flashLeds();
    return;
  }

  lastStepMode = true;
  function = FUNC_LENGTH;
  showMode();
}

static inline void bottomHandleClearButton(void) {
  const bool awaitingSecondTap = doubleClickTimer && lastDouble == 6;

  if (!awaitingSecondTap) {
    lastDouble = 6;
    doubleClickTimer = doubleClick_timeout;
    detachedAFader = false;

    clearHeld = true;
    alreadyClearedAuto = false;
    return;
  }

  if (doubleClickTimer && !wiggleHeld) {
    resetFaders();
  }
}

static inline void bottomHandleWiggleButton(uint8_t number) {
  wiggleHeld = true;

  if (clearHeld) {
    clearPatternRow(wiggleAuto[currentPattern]);
    showMode();
    return;
  }

  if (!stutterPressed && !freezeHeld && !stutter) {
    setFunctionAndShow(number);
  } else if (freezeHeld) {
    opl2.setCrazy(1);
  }
}

static inline void bottomHandleSwingButton(uint8_t number) {
  if (clearHeld) {
    clearPatternRow(swingAuto[currentPattern]);
    saveAuto = true;
    showMode();
    return;
  }

  if (funcHeld) {
    wonkySwing = !wonkySwing;
    flashLeds();
  }
  setFunctionAndShow(number);
}

static inline void bottomHandleMuteButton(uint8_t number) {
  visu = true;
  muteHeld = true;
  setFunctionAndShow(number);
}

static inline void bottomHandleSoloButton(uint8_t number) {
  visu = true;
  setFunctionAndShow(number);
}

static inline void bottomHandleProbabilityButton(uint8_t number) {
  probHeld = true;
  setFunctionAndShow(number);
}

static inline void bottomHandleStutterButton(uint8_t number) {
  if (!clearHeld) {
    setFunctionAndShow(number);
  } else {
    clearPatternRow(stutterSteps[currentPattern]);
  }

  saveAuto = true;
}

static inline void bottomHandleBendButton(uint8_t number) {
  if (clearHeld) {
    bendSteps.clearRow(currentPattern);
    saveAuto = true;
    return;
  }

  if (!stutterPressed) {
    setFunctionAndShow(number);
  }

  punchBend();
  startBend();
  saveAuto = true;
  bendHeld = true;
}

static inline void bottomHandleFreezeButton(void) {
  if (clearHeld) {
    freezeSteps.clearRow(currentPattern);
    saveAuto = true;
    return;
  }

  freezeTimer = 80;
  freezeHeld = true;

  if (rec && (stepTickCounter < (stepTicks >> 1))) {
    freezeSteps[currentPattern][polyStep[0]] = 1;
    saveAuto = true;
  }
}

static inline void bottomHandleTremButton(uint8_t number) {
  tremHeld = true;

  if (clearHeld) {
    clearPatternRow(tremAuto[currentPattern]);
    saveAuto = true;
    return;
  }

  setFunctionAndShow(number);
}

static inline void bottomHandleVibratoButton(uint8_t number) {
  vibHeld = true;

  if (clearHeld) {
    clearPatternRow(vibAuto[currentPattern]);
    saveAuto = true;
    return;
  }

  setFunctionAndShow(number);
}

static inline void bottomHandlePhonesButton(uint8_t number) {
  if (clearHeld) {
    clearPatternRow(phonesAuto[currentPattern]);
    saveAuto = true;
    return;
  }

  setFunctionAndShow(number);
}

static inline void bottomHandleMixButton(uint8_t number) {
  if (clearHeld) {
    clearPatternRow(mixAuto[currentPattern]);
    saveAuto = true;
    showMode();
    return;
  }

  setFunctionAndShow(number);
}

static inline void bottomHandleRouteButton(uint8_t number) {
  if (clearHeld) {
    routeAuto.clearPattern(currentPattern);
    saveAuto = true;
    return;
  }

  routeHeld = 0;
  setFunctionAndShow(number);
}

static inline void bottomHandleFunctionButton(uint8_t number) {
  switch (number) {
    case FUNC_SAVE:
      bottomSelectSaveFunction();
      return;

    case FUNC_LENGTH:
      bottomHandleLengthButton();
      return;

    case FUNC_CLEAR:
      bottomHandleClearButton();
      return;

    case FUNC_WIGGLE:
      bottomHandleWiggleButton(number);
      return;

    case FUNC_SWING:
      bottomHandleSwingButton(number);
      return;

    case FUNC_MUTE:
      bottomHandleMuteButton(number);
      return;

    case FUNC_SOLO:
      bottomHandleSoloButton(number);
      return;

    case FUNC_PROB:
      bottomHandleProbabilityButton(number);
      return;

    case FUNC_STUTTER:
      bottomHandleStutterButton(number);
      return;

    case FUNC_BEND:
      bottomHandleBendButton(number);
      return;

    case FUNC_FREEZE:
      bottomHandleFreezeButton();
      return;

    case FUNC_TREM:
      bottomHandleTremButton(number);
      return;

    case FUNC_VIBR:
      bottomHandleVibratoButton(number);
      return;

    case FUNC_PHONES:
      bottomHandlePhonesButton(number);
      return;

    case FUNC_MIX:
      bottomHandleMixButton(number);
      return;

    case FUNC_ROUTE:
      bottomHandleRouteButton(number);
      return;

    default:
      return;
  }
}

static inline void bottomHandleRelease(uint8_t number) {
  if (lastStepMode) {
    return;
  }

  switch (number) {
    case FUNC_MUTE:
      muteHeld = false;
      showMode();
      return;

    case FUNC_CLEAR:
      if (wiggleHeld) {
        if (!detachedAFader) {
          randomer.clearRow(currentPattern);
          saveAuto = true;
        }
      } else {
        function = number;
      }
      clearHeld = false;
      showMode();
      return;

    case FUNC_FREEZE:
      freezeHeld = false;
      if (freeze) {
        opl2.freeze(0);
        resetRegisters = true;
        freeze = false;
      }
      return;

    case FUNC_BEND:
      bendHeld = false;
      if (bend) {
        stopBend();
      }
      return;

    case FUNC_WIGGLE:
      wiggleHeld = false;
      opl2.setCrazy(0);
      return;

    case FUNC_PROB:
      probHeld = false;
      showMode();
      return;

    case FUNC_ROUTE:
      routeHeld = 1;
      showMode();
      return;

    case FUNC_TREM:
      tremHeld = false;
      showMode();
      return;

    case FUNC_VIBR:
      vibHeld = false;
      showMode();
      return;

    default:
      return;
  }
}

void function_bottom(uint8_t number, bool value) {
  if (value) {
    if (bottomHandleTempoHeldToggle(number)) {
      return;
    }

    if (bottomHandleFuncHeldToggle(number)) {
      return;
    }

    if (bottomHandleStutterMode(number)) {
      return;
    }

    lastTop = 10;

    if (bottomHandleLastStepMode(number)) {
      return;
    }

    if (bottomHandleSaveMode(number)) {
      return;
    }

    bottomHandleFunctionButton(number);
    return;
  }

  bottomHandleRelease(number);
}

// Top row helpers

static inline void topHandleSavePress(uint8_t number) {
  if (currentFolder != number) {
    currentFolder = number;
    EEPROMupdate(17, currentFolder);
    showMode();
  }
}

static inline void topHandleLengthPress(uint8_t number) {
  if (recHeld) {
    if (pageCopyTarget == 4) {
      pageCopyTarget = number;
    } else if (number != pageCopyTarget) {
      clonePlocPage(pageCopyTarget, number, currentPattern, currentPattern);

      for (int16_t v = 0; v < 10; ++v) {
        if ((funcHeld && v == selectedVoice) || !funcHeld) {
          for (int16_t i = 0; i < 16; ++i) {
            uint8_t targetStep = i + (number << 4);
            uint8_t sourceStep = i + (pageCopyTarget << 4);
            setNudgeAnyValueAtAnyStep(targetStep, getNudge(sourceStep, v), v);
            if (v < 6) {
              bitWrite(pattern[currentPattern][0][targetStep], v, bitRead(pattern[currentPattern][0][sourceStep], v));
            } else {
              setStep(v - 6, targetStep, getStep(v - 6, sourceStep));
              setNote(v - 6, targetStep, getNote(v - 6, sourceStep));
              if (v == 8) {
                setNote2(targetStep, getNote2(sourceStep));
                setStep2(targetStep, getStep2(sourceStep));
              }
            }
          }
        }
      }
    }

    noRecAction = false;
    return;
  }

  if (stepHeld) {
    changedSelectedVoice = true;
    selectedVoice = number;
    showMode();
    return;
  }

  if (number >= 4) {
    return;
  }

  if (funcHeld) {
    noFuncAction = false;
    pages[currentPattern][selectedVoice] = number;
    while (polyStep[selectedVoice] >= (pages[currentPattern][selectedVoice] + 1) << 4) {
      polyStep[selectedVoice] -= 16;
    }
    if (selectedPage > pages[currentPattern][selectedVoice]) {
      selectedPage = pages[currentPattern][selectedVoice];
    }
    markMaxStepCacheDirty();
    markPatternVisualizerDirty();
    return;
  }

  for (int16_t i = 0; i < 10; ++i) {
    pages[currentPattern][i] = number;
    if (polyStep[i] >= (pages[currentPattern][i] + 1) << 4) {
      polyStep[i] = 0;
    }
    if (selectedPage > pages[currentPattern][i]) {
      selectedPage = pages[currentPattern][i];
    }
  }
  markMaxStepCacheDirty();
  markPatternVisualizerDirty();
}

static inline void topHandleClearPress(uint8_t number) {
  if (clearHeld) {
    clearTrack(number);
    for (int16_t i = 0; i < 64; ++i) {
      if (number < 6) {
        bitWrite(pattern[currentPattern][0][i], number, 0);
      } else {
        if (number == 8) {
          setStep2(i, 0);
        }
        setStep(number - 6, i, 0);
      }
      setNudgeAnyValueAtAnyStep(i, 0, number);
    }
    return;
  }

  selectedVoice = number;
  showMode();
}

static inline void topHandleWigglePress(uint8_t number) {
  if (lastDouble == 2 && doubleClickTimer) {
    for (int16_t i = 0; i < 16; ++i) {
      if (randomDepth[i] != number) {
        randomDepth[i] = number;
        saveAuto = true;
      }
    }
    return;
  }

  if (rec) {
    setWiggleAuto(polyStep[0], number + 1);
  }

  doubleClickTimer = doubleClick_timeout;
  lastDouble = 2;

  if (randomDepth[currentPattern] != number) {
    randomDepth[currentPattern] = number;
    saveAuto = true;
  }
}

static inline void topHandleSwingPress(uint8_t number) {
  if (rec) {
    setSwingAuto(polyStep[0], number + 1);
  } else {
    swingSetting[currentPattern] = number;
  }
  updateSwing(number);
}

static inline void topHandleMutePress(uint8_t number) {
  if (muteHeld && number < 4) {
    if (!pageSkip[number]) {
      uint8_t skipCount = 0;
      for (int16_t i = 0; i < 4; ++i) {
        if (pageSkip[i]) {
          ++skipCount;
        }
      }
      if (skipCount < 3) {
        pageSkip[number] = 1;
      }
    } else {
      pageSkip[number] = 0;
    }
  } else {
    mute[number] = !mute[number];

    if (number == 6 && mute[number]) {
      synthHotOff[0] = 1;
      synthHotOff[3] = 1;
    }
    if (number == 7 && mute[number]) {
      synthHotOff[1] = 1;
      synthHotOff[4] = 1;
    }
    if (number == 8 && mute[number]) {
      synthHotOff[2] = 1;
      synthHotOff[5] = 1;
    }
    if (number == 9 && mute[number]) {
      synthHotOff[9] = 1;
      synthHotOff[12] = 1;
    }
  }

  showMode();
}

static inline void topHandleSoloPress(uint8_t number) {
  if (number + 1 == solo) {
    solo = 0;
  } else {
    solo = number + 1;
  }
}

static inline void topHandleProbabilityPress(uint8_t number) {
  if (probHeld) {
    probTarget = number;
  } else {
    prob[kit[currentPattern]][probTarget] = 9 - number;
    saveAuto = true;
  }
}

static inline void topHandleStutterPress(uint8_t number) {
  ++stutterPressed;
  stutterHeld[number] = 1;
  sortStutter();
}

static inline void topHandleBendPress(uint8_t number) {
  bendy = 1 + number;
  punchBend();
  startBend();
  bendHeld = true;
}

static inline void topHandleTremPress(uint8_t number) {
  if (tremHeld) {
    if (rec) {
      setTremAutoWav(polyStep[0], number + 1);
    } else {
      tremWavPatt[currentPattern] = number;
      saveAuto = true;
    }
    tremWav = number;
  } else {
    if (rec) {
      setTremAuto(polyStep[0], number + 1);
      saveAuto = true;
      tremRate[currentPattern] = number;
    } else {
      tremRate[currentPattern] = number;
    }
    newTremRate = true;
  }

  showMode();
}

static inline void topHandleVibratoPress(uint8_t number) {
  if (funcHeld) {
    vibratoPower[currentPattern] = number;
    noFuncAction = false;
    showMode();
    return;
  }

  if (vibHeld) {
    if (rec) {
      setVibAutoWav(polyStep[0], number + 1);
    } else {
      vibWavPatt[currentPattern] = number;
      saveAuto = true;
    }
    vibWav = number;
  } else {
    if (rec) {
      setVibAuto(polyStep[0], number + 1);
      vibratoRate[currentPattern] = number;
    } else {
      vibratoRate[currentPattern] = number;
    }
    newVibRate = true;
  }

  showMode();
}

static inline void topHandlePhonesPress(uint8_t number) {
  if (rec) {
    setPhonesAuto(polyStep[0], number + 1);
    phonesVol = number;
    setPhonesVol();
  } else if (phonesVol != number) {
    phonesVol = number;
    EEPROMupdate(20, phonesVol);
    setPhonesVol();
  }

  showMode();
}

static inline void topHandleMixPress(uint8_t number) {
  if (rec) {
    setMixAuto(polyStep[0], number + 1);
    mixVol = number;
    setMixVol();
  } else if (mixVol != number) {
    mixVol = number;
    EEPROMupdate(1, mixVol);
    setMixVol();
  }

  showMode();
}

static inline void persistRoutingConfiguration(void) {
  EEPROMupdate(3, routing[0]);
  EEPROMupdate(4, routing[1]);
  EEPROMupdate(5, routing[2]);
  EEPROMupdate(6, routing[3]);
  EEPROMupdate(7, routing[4]);
  EEPROMupdate(18, routing[5]);
  EEPROMupdate(19, routing[6]);
}

static inline void commitRoutingChange(void) {
  updateRouting();
  if (rec) {
    routeAuto[currentPattern][polyStep[0]][routeTarget] = routing[routeTarget] + 1;
    saveAuto = true;
  } else {
    persistRoutingConfiguration();
  }
}

static inline void topHandleRoutePress(uint8_t number) {
  if (recHeld) {
    saveRoutingPreset(number);
    noRecAction = false;
    return;
  }

  if (funcHeld) {
    loadRoutingPreset(number);
    noFuncAction = false;
    return;
  }

  if (!routeHeld) {
    switch (number) {
      case 0:
        routeTarget = 0;
        break;
      case 1:
      case 3:
      case 4:
        routeTarget = 1;
        break;
      case 2:
      case 5:
        routeTarget = 2;
        break;
      case 6:
        routeTarget = 3;
        break;
      case 7:
        routeTarget = 4;
        break;
      case 8:
        routeTarget = 5;
        break;
      case 9:
        routeTarget = 6;
        break;
      default:
        break;
    }
    return;
  }

  switch (number) {
    case 0:
      bitWrite(routing[routeTarget], 5, !bitRead(routing[routeTarget], 5));
      commitRoutingChange();
      break;
    case 1:
      bitWrite(routing[routeTarget], 4, !bitRead(routing[routeTarget], 4));
      commitRoutingChange();
      break;
    case 3:
      bitWrite(routing[routeTarget], 6, !bitRead(routing[routeTarget], 6));
      commitRoutingChange();
      break;
    case 2:
      bitWrite(routing[routeTarget], 7, !bitRead(routing[routeTarget], 7));
      commitRoutingChange();
      break;
    default:
      break;
  }
}

static inline void handleTopFunctionPress(uint8_t number) {
  switch (function) {
    case FUNC_SAVE:
      topHandleSavePress(number);
      break;

    case FUNC_LENGTH:
      topHandleLengthPress(number);
      break;

    case FUNC_CLEAR:
      topHandleClearPress(number);
      break;

    case FUNC_WIGGLE:
      topHandleWigglePress(number);
      break;

    case FUNC_SWING:
      topHandleSwingPress(number);
      break;

    case FUNC_MUTE:
      topHandleMutePress(number);
      break;

    case FUNC_SOLO:
      topHandleSoloPress(number);
      break;

    case FUNC_PROB:
      topHandleProbabilityPress(number);
      break;

    case FUNC_STUTTER:
      topHandleStutterPress(number);
      break;

    case FUNC_BEND:
      topHandleBendPress(number);
      break;

    case FUNC_TREM:
      topHandleTremPress(number);
      break;

    case FUNC_VIBR:
      topHandleVibratoPress(number);
      break;

    case FUNC_PHONES:
      topHandlePhonesPress(number);
      break;

    case FUNC_MIX:
      topHandleMixPress(number);
      break;

    case FUNC_ROUTE:
      topHandleRoutePress(number);
      break;

    default:
      break;
  }

  showMode();
}

static inline void handleTopFunctionRelease(uint8_t number) {
  (void)number;
}

void function_top(uint8_t number, uint8_t value) {
  if (value) {
    handleTopFunctionPress(number);
    return;
  }

  handleTopFunctionRelease(number);
}
