


#include "scenes.h"

#include "pattern_visualizer.h"

void loop() {


  if (mode != MODE_SONG && folderChanged) {
    folderChanged = false;
  }

  handleSceneMorph();
  if (sceneModeActive) {
    maintainSceneModeStatusLeds();
    renderSceneModeLeds();
  }

  for (int i = 0; i < 10; i++) {

    maxStep = pages[currentPattern][i];
    maxStep <<= 4;
    maxStep += lastStepPoly[currentPattern][i];

    if (wiggleHeld && stutter) {
      stutterStepPoly[i] = random(maxStep);
    }

    while (polyStep[i] < 64) {
      uint8_t nextPage = (uint8_t)((polyStep[i] + 1) >> 4);
      if (nextPage >= 4 || !pageSkip[nextPage]) {
        break;
      }
      polyStep[i] += 16;
    }
    polyStep[i] = constrain(polyStep[i], 0, maxStep);
  }



  //flashLeds
  if (swiping < 255) {
    swiping++;
    if (swiping == 255) {
      //flashLeds ENDED, SHOW MODES OR PATTERN

      if (mode == 0 || mode == 2) {
        showMode();
      } else if (mode == 1) {
        showPattern();
      }
    }
  }
  if (stepTicks != stepTickLast) {
    stepTickLast = stepTicks;
  }


  if (flipStepFlag && !keyChangeEdit) {
    flipStepLed();
    flipStepFlag = false;
  }
  if (seedNewRandomTable) {
    for (int i = 0; i < 8; i++) {
      randomTable[i] = random(16);
    }
    seedNewRandomTable = false;
  }


  if (newFlash) {
    blinkStepLed();
    newFlash = false;

    if (mode == MODE_SONG && folderChanged) {
      clearTop();
      setLed(16 + pendingFolder, flasher);
      for (int i = 0; i < 16; i++) {
        setLed(i, flasher);
      }
    }
  }
  if (killTempoLed) {
    setLed(LED_tempo, 0);
    killTempoLed = false;
  }

  if (scanCount2 > 12) {
    scanCount++;
    if (scanCount > 17) scanCount = 0;
    topRow(scan[scanCount]);
    scanCount2 = 0;
  }

  if (showModeFlag) {
    showModeFlag = false;
    showMode();
  }

  if (wipeCounter > 5) {
    wipeCounter = 0;
    if (wipeStep < 16) {
      clearTop();
      clearBottom();
      if (wipeStep < 11) {
        topRow(wipeStep);
      }
      bottomRow(wipeStep);
    }

    wipeStep++;
    if (wipeStep == 17) {
      wiping = false;
      showMode();
    }
  }



  for (int16_t i = 0; i < 16; i++) {
    if (swingSetting[i] > 9 || swingSetting[i] < 0) {
      swingSetting[i] = 0;
    }
  }
  if (!plocStep) {
    nudgeMode = 0;
  }

  //convert timer counter to nudgeStep
  nudgeStep = map(stepTickCounter, 0, stepTicks, 0, 9);
  if (nudgeStep > 9) nudgeStep = 9;


  //fire synths
  if (bitRead(synthTrig, 0)) {
    if (!mute[6]) {
      if (stutter && !drill) {
        triggerSynth(0, 0, getNote(0, stutterStepPoly[6]), 1);
      } else {
        triggerSynth(0, 0, getNote(0, polyStep[6]), 1);
      }
      bitWrite(synthTrig, 0, 0);
    }
  }


  if (bitRead(synthTrig, 1)) {
    if (!mute[7]) {

      if (stutter && !drill) {
        triggerSynth(1, 0, getNote(1, stutterStepPoly[7]), 1);
      } else {
        triggerSynth(1, 0, getNote(1, polyStep[7]), 1);
      }


      bitWrite(synthTrig, 1, 0);
    }
  }
  if (bitRead(synthTrig, 2)) {
    if (!mute[8]) {



      if (stutter && !drill) {
        triggerSynth(2, 0, getNote(2, stutterStepPoly[8]), 1);

        //second track
        if (getStep2(stutterStepPoly[8])) {
          triggerSynth(2, 0, getNote2(stutterStepPoly[8]), 1);
        }

      } else {
        triggerSynth(2, 0, getNote(2, polyStep[8]), 1);

        //second track
        if (getStep2(polyStep[8])) {
          triggerSynth(2, 0, getNote2(polyStep[8]), 1);
        }
      }
      bitWrite(synthTrig, 2, 0);
    }
  }
  if (bitRead(synthTrig, 3)) {
    if (!mute[9]) {
      if (stutter && !drill) {
        triggerSynth(3, 0, getNote(3, stutterStepPoly[9]), 1);
      } else {
        triggerSynth(3, 0, getNote(3, polyStep[9]), 1);
      }

      bitWrite(synthTrig, 3, 0);
    }
  }


  //open synth gates

  for (int16_t i = 0; i < 15; i++) {
    if (synthHotOffOn[i]) {
      opl2.setKeyOn(i, 0);
      opl2.setKeyOn(i, 1);
      synthHotOffOn[i] = 0;
    }
  }

  for (int16_t i = 0; i < 15; i++) {
    if (synthHotOff[i]) {
      opl2.setKeyOn(i, 0);
      synthHotOff[i] = 0;
    }
  }


  //nudge countdown expired?
  for (int16_t i = 0; i < 10; i++) {
    if (nudged[i]) {
      fireVoiceNow(i);
      nudged[i] = false;
    }
  }



  //show nudgeDelay
  if (nudgeMode == 1) {
    nudgeMode = 2;
    showNudge();
  }
  //modulate mults
  if (multLast[0] != multBase[0] + multLfo[0]) {
    multLast[0] = multBase[0] + multLfo[0];
    opl2.setMultiplier(3, OPERATOR2, multLast[0]);
  }
  if (multLast[1] != multBase[1] + multLfo[1]) {
    multLast[1] = multBase[1] + multLfo[1];
    opl2.setMultiplier(4, OPERATOR2, multLast[1]);
  }
  if (multLast[2] != multBase[2] + multLfo[2]) {
    multLast[2] = multBase[2] + multLfo[2];
    opl2.setMultiplier(5, OPERATOR2, multLast[2]);
    opl2.setMultiplier(13, OPERATOR2, multLast[2]);
  }
  if (multLast[3] != multBase[3] + multLfo[3]) {
    multLast[3] = multBase[3] + multLfo[3];
    opl2.setMultiplier(12, OPERATOR2, multLast[3]);
    opl2.setMultiplier(14, OPERATOR2, multLast[3]);
  }


  if ((millis() > visualiserTimeout && mode != MODE_FUNCTION) || (mode == MODE_FUNCTION && function == FUNC_MUTE) || (mode == MODE_FUNCTION && function == FUNC_SOLO)) {
    if (!funcHeld && !plocStep) visu = true;
  } else {
    visu = false;
  }

  if (freezeTimer) {
    freezeTimer--;
    if (!freezeTimer) {
      freeze = true;
      opl2.freeze(1);
    }
  }

  volTick();


  //unflip the blinked step.
  if ((flip) && (mode == 1) && (flipStep < 16)) {

    setLed(flipStep, flipBit);
    flip = false;
  }

  if (probCounter) {
    probStack[probCounter] = random(1, 9);
    probCounter--;
  }

  updateFreq();


  //a jack has been connected, save master (internal) BPM for when it is removed again
  if ((!analogSlave) && (!digitalRead(SYNC_DETECT_PIN))) {
    analogSlave = true;
  }



  //a jack has been removed, recall master (internal) BPM
  if ((analogSlave) && (digitalRead(SYNC_DETECT_PIN))) {
    analogSlave = false;
  }

  //analog sync in
  if (analogSlave) {
    analogPulseRead();
  }



  if (newStepHot) {

    seqStep();
    newStepHot = false;



    if (pendingKit) {
      kit[currentPattern] = pendingKit - 1;
      updateCurrentKit();
      resetRegisters = true;
      pendingKit = 0;
      showMode();
    }
  }

  if (engineTickHot) {
    engineTick();
    engineTickHot = false;
  }



  midiRead();


  randyTick();

  if (!autoDone) {
    readAuto();
    autoDone = true;
  }


  for (int16_t i = 0; i < 53; i++) {

    if (flipLedTimer[i]) {
      flipLedTimer[i]--;
      if (!flipLedTimer[i]) {
        unFlipLed(i);
      }
    }
  }



  if ((newStep) && (!lastStepMode) && (!stepHeld)) {

    if ((trickStepEdit) && (!keyChangeEdit)) {

      //blink the tricks
      switch (selectedPage) {
        case 0:
          if (polyStep[0] < 16) {
            switch (trickStepEdit) {
              case 1:
                if (!stutterSteps[currentPattern][polyStep[0]]) {
                  setLed(polyStep[selectedVoice], 1);
                  flipStep = polyStep[selectedVoice];
                  flipBit = 0;
                  flicker = flickerDuration;
                  newStep = false;
                }
                break;  //stutter
              case 2:
                if (!bendSteps[currentPattern][polyStep[0]]) {
                  setLed(polyStep[selectedVoice], 1);
                  flipStep = polyStep[selectedVoice];
                  flipBit = 0;
                  flicker = flickerDuration;
                  newStep = false;
                }
                break;  //bend
              case 3:
                if (!freezeSteps[currentPattern][polyStep[0]]) {
                  setLed(polyStep[selectedVoice], 1);
                  flipStep = polyStep[selectedVoice];
                  flipBit = 0;
                  flicker = flickerDuration;
                  newStep = false;
                }
                break;  //freeze
            }
          }
          break;
        case 1:
          if ((polyStep[0] > 15) && (polyStep[0] < 32)) {
            switch (trickStepEdit) {
              case 1:
                if (!stutterSteps[currentPattern][polyStep[0]]) {
                  setLed(polyStep[selectedVoice] - 16, 1);
                  flipStep = polyStep[selectedVoice] - 16;
                  flipBit = 0;
                  flicker = flickerDuration;
                  newStep = false;
                }
                break;  //stutter
              case 2:
                if (!bendSteps[currentPattern][polyStep[0]]) {
                  setLed(polyStep[0] - 16, 1);
                  flipStep = polyStep[0] - 16;
                  flipBit = 0;
                  flicker = flickerDuration;
                  newStep = false;
                }
                break;  //bend
              case 3:
                if (!freezeSteps[currentPattern][polyStep[0]]) {
                  setLed(polyStep[0] - 16, 1);
                  flipStep = polyStep[0] - 16;
                  flipBit = 0;
                  flicker = flickerDuration;
                  newStep = false;
                }
                break;  //freeze
            }
          }

          break;

        case 2:
          if ((polyStep[0] > 31) && (polyStep[0] < 48)) {
            switch (trickStepEdit) {
              case 1:
                if (!stutterSteps[currentPattern][polyStep[0]]) {
                  setLed(polyStep[0] - 32, 1);
                  flipStep = polyStep[0] - 32;
                  flipBit = 0;
                  flicker = flickerDuration;
                  newStep = false;
                }
                break;  //stutter
              case 2:
                if (!bendSteps[currentPattern][polyStep[0]]) {
                  setLed(polyStep[0] - 32, 1);
                  flipStep = polyStep[0] - 32;
                  flipBit = 0;
                  flicker = flickerDuration;
                  newStep = false;
                }
                break;  //bend
              case 3:
                if (!freezeSteps[currentPattern][polyStep[0]]) {
                  setLed(polyStep[0] - 32, 1);
                  flipStep = polyStep[0] - 32;
                  flipBit = 0;
                  flicker = flickerDuration;
                  newStep = false;
                }
                break;  //freeze
            }
          }
          break;

        case 3:
          if (polyStep[0] > 47) {
            switch (trickStepEdit) {
              case 1:
                if (!stutterSteps[currentPattern][polyStep[0]]) {
                  setLed(polyStep[0] - 48, 1);
                  flipStep = polyStep[0] - 48;
                  flipBit = 0;
                  flicker = flickerDuration;
                  newStep = false;
                }
                break;  //stutter
              case 2:
                if (!bendSteps[currentPattern][polyStep[0]]) {
                  setLed(polyStep[0] - 48, 1);
                  flipStep = polyStep[0] - 48;
                  flipBit = 0;
                  flicker = flickerDuration;
                  newStep = false;
                }
                break;  //bend
              case 3:
                if (!freezeSteps[currentPattern][polyStep[0]]) {
                  setLed(polyStep[0] - 48, 1);
                  flipStep = polyStep[0] - 48;
                  flipBit = 0;
                  flicker = flickerDuration;
                  newStep = false;
                }
                break;  //freeze
            }
          }
          break;
      }


    } else {



      if ((!stutter) && (!keyChangeEdit)) {

        if (mode == MODE_STEP) {
          if (newStep && stepTickCounter == stepTicks >> 1) {
            showPattern();
            newStep = false;
          }
        }
      }
    }
  }

  readMux();
  oplUpdate();
  runButton();
  sendMidiDrums();
  patternVisualizerTick();
  serviceEEPROMAutosave();
}
