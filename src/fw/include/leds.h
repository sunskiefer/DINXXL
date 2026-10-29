
#include "scenes.h"

static inline void setStatusLeds(bool kitPatOn, bool stepOn, bool functionOn) {
  setLed(LED_kitPat, kitPatOn);
  setLed(LED_step, stepOn);
  setLed(LED_function, functionOn);
}

static inline void setTriIndicator(int8_t index) {
  setStatusLeds(index == 0, index == 1, index == 2);
}

static inline void showPunchOffsetIndicator(uint8_t punchOffset) {
  switch (punchOffset) {
    case 0:
    case 12:
    case 24:
      setTriIndicator(punchOffset / 12);
      break;
  }
}

static inline void showOctaveOffsetIndicator(int8_t offset) {
  if (offset >= 2) {
    setTriIndicator(2);
  } else if (offset == 1) {
    setTriIndicator(1);
  } else {
    setTriIndicator(0);
  }
}

static inline void showRateMultiplierIndicator(uint8_t value) {
  if (value <= 2) {
    setTriIndicator(value);
  }
}

void showNumberSynth(uint8_t data) {

  visualiserTimeout = millis() + visualiserTimeoutMS;

  showing = data;
  showTimer = 300;
  topRow5(data);
}

void showMode() {
  setLed(LED_rec, 0);
  switch (mode) {
    case MODE_SONG:  //Song
      setStatusLeds(true, false, true);
      if (!folderChanged) {
        bottomRow(currentSong);
        topRow(currentFolder);
      }
      break;

    case MODE_PATT_KIT:  //Pattern
      setStatusLeds(true, false, false);

      if (recHeld) {
        if (chainFreeze) {
          bottomRow(10);
        } else {
          clearBottom();
        }
      } else {
        bottomRow(currentPattern);
      }
      topRow(currentKit % 10);

      break;

    case MODE_STEP:  //Step
      setStatusLeds(false, true, false);

      if (trickStepEdit) {

        if (stepHeld) {
          bottomRow(7 + trickStepEdit);
        } else {
          showTricks();
        }
      } else {

        if (keyChangeEdit) {
          showPiano();
          topRow(musicModes[currentPattern]);
        } else {
          if (recHeld) {
            showPunchOffsetIndicator(punchOffset);
          }

          if (!showTimer && !trickStepEdit) topRow(selectedVoice);


          //show octaveOffset
          if ((heldTop) && (!recHeld)) {
            showOctaveOffsetIndicator(offsetOct[currentPattern][selectedVoice]);
          }
        }
        showPattern();
      }
      break;

    case 2:  //Function
      if (recHeld) {
        showPunchOffsetIndicator(punchOffset);
      } else {
        setStatusLeds(false, false, true);
      }

      clearBottom();
      clearTop();

      //show function values
      bottomRow(function);

      switch (function) {

        case 8:
        case 9: topRow(scan[scanCount]); break;  // scan for live effects

        case 1:
          if (stepHeld) {
            topRow(selectedVoice);
          } else {
            if (!sdLedsMuted) {
              mx.setPoint(4, 3, 1);
              if (pages[currentPattern][selectedVoice] > 0) {
                mx.setPoint(0, 4, 1);  //top2
              } else {
                mx.setPoint(0, 4, 0);
              }
              if (pages[currentPattern][selectedVoice] > 1) {
                mx.setPoint(1, 4, 1);  //top3
              } else {
                mx.setPoint(1, 4, 0);
              }
              if (pages[currentPattern][selectedVoice] > 2) {
                mx.setPoint(2, 4, 1);  //top4
              } else {
                mx.setPoint(2, 4, 0);
              }
            }
          }
          break;  //length

        case 2: topRow(selectedVoice); break;  //clear

        case 3: topRow(randomDepth[currentPattern]); break;  //wiggle

        case 5:
          if (muteHeld) {
            clearTop();
            for (int16_t i = 0; i < 4; i++) {
              setLed(i + 16, 1);
            }
          } else {
            for (int16_t i = 0; i < 10; i++) {
              setLed(i + 16, mute[i]);
            }
          }
          break;  //mute

        case 6:
          if (solo) {
            topRow(solo - 1);
          } else {
            clearTop();
          }
          break;  //solo

        case 7:
          if (probHeld) {
            topRow(probTarget);
          } else {
            topRow(9 - prob[kit[currentPattern]][probTarget]);
          }
          break;  //prob

        case 4: topRow(swingSetting[currentPattern]); break;  //swing

        case 11:
          if (funcHeld) {
            topRow(tremRateMultiplier[currentPattern]);
          } else if (tremHeld) {
            topRow(tremWav);
            showRateMultiplierIndicator(tremRateMultiplier[currentPattern]);
          } else {
            topRow(tremRate[currentPattern]);
          }
          break;  //trem
        case 12:
          if (vibHeld) {
            showRateMultiplierIndicator(vibratoRateMultiplier[currentPattern]);
            topRow(vibWav);
          } else {

            if (funcHeld) {
              topRow(vibratoPower[currentPattern]);
            } else {
              topRow(vibratoRate[currentPattern]);
            }
          }
          break;  //vibr

        case 13: topRow(phonesVol); break;  //phones
        case 14: topRow(mixVol); break;     //mix

        case 15:
          clearTop();

          if (!routeHeld) {

            //show target
            switch (routeTarget) {

              case 0: setLed(0 + 16, 1); break;  //bass drum
              case 1:
                setLed(1 + 16, 1);
                setLed(3 + 16, 1);
                setLed(4 + 16, 1);
                break;  //snare/hat
              case 2:
                setLed(2 + 16, 1);
                setLed(5 + 16, 1);
                break;                           //tom/cymb
              case 3: setLed(6 + 16, 1); break;  //synth 1
              case 4: setLed(7 + 16, 1); break;  //synth 2
              case 5: setLed(8 + 16, 1); break;  //synth 3
              case 6: setLed(9 + 16, 1); break;  //synth 4
            }
          } else {
            //show routing

            setLed(16, bitRead(routing[routeTarget], 5));
            setLed(17, bitRead(routing[routeTarget], 4));
            setLed(18, bitRead(routing[routeTarget], 7));
            setLed(19, bitRead(routing[routeTarget], 6));
          }
          break;  //routing
      }
      break;
  }

  if (sceneModeActive) {
    maintainSceneModeStatusLeds();
    renderSceneModeLeds();
  }
}

void showNudge() {
  topRow(getNudge(x16[selectedPage] + plocStep - 1, selectedVoice));
}

void flashAll() {
  allOn();
  delay(100);
  clearTop();
  clearBottom();
  delay(100);
}
void blink() {
  bottomRow(0);
  blinkTimer = 500;
}
void showNumber(uint8_t data) {

  visualiserTimeout = millis() + visualiserTimeoutMS;

  showing = data;
  showTimer = 300;
  topRow(data);
}


void twoLeds(uint8_t led1, uint8_t led2) {

  for (int16_t i = 0; i < 10; i++) {
    if (i == led1) {
      setLed(i + 16, 1);
    } else {
      setLed(i + 16, 0);
    }
  }
  for (int16_t i = 0; i < 16; i++) {
    if (i == led2) {
      setLed(i, 1);
    } else {
      setLed(i, 0);
    }
  }
}

void clearTop() {
  if (sceneModeActive) {
    renderSceneModeLeds();
    return;
  }
  for (int16_t i = 0; i < 10; i++) {
    setLed(i + 16, 0);
  }
}

void flipLed(uint8_t number) {

  if (!flipped[number]) {
    flipped[number] = 1;
    setLed(number, !currentLed[number]);
    flipLedTimer[number] = visualiserDuration;
  }
}

void unFlipLed(uint8_t number) {
  flipped[number] = 0;
  setLed(number, !currentLed[number]);
}

void flipStepLed() {
  if (sdLedsMuted) {
    return;
  }
  if (mode == MODE_STEP && !stepHeld && !trickStepEdit && !stutter) {
    uint8_t stepToFlip = 16;

    if (chaseAllPages) {
      stepToFlip = polyStep[selectedVoice];
      while (stepToFlip > 15) stepToFlip -= 16;

    } else {
      switch (selectedPage) {
        case 0:
          if (polyStep[selectedVoice] < 16) stepToFlip = polyStep[selectedVoice];
          break;
        case 1:
          if (polyStep[selectedVoice] > 15 && polyStep[selectedVoice] < 32) stepToFlip = polyStep[selectedVoice] - 16;
          break;
        case 2:
          if (polyStep[selectedVoice] > 31 && polyStep[selectedVoice] < 48) stepToFlip = polyStep[selectedVoice] - 32;
          break;
        case 3:
          if (polyStep[selectedVoice] > 47) stepToFlip = polyStep[selectedVoice] - 48;
          break;
      }
    }
    if (stepToFlip < 16) {
      switch (stepToFlip) {
        case 0 ... 7: mx.setPoint(stepToFlip, 1, !mx.getPoint(stepToFlip, 1)); break;
        case 8 ... 15: mx.setPoint(stepToFlip - 8, 2, !mx.getPoint(stepToFlip - 8, 2)); break;
      }
    }
  }
}

void setLed(uint8_t number, bool data) {
  //bottom row 0-15

  currentLed[number] = data;
  if (sdLedsMuted) {
    return;
  }

  switch (number) {

    case 0 ... 7: mx.setPoint(number, 1, data); break;
    case 8 ... 15: mx.setPoint(number - 8, 2, data); break;


    case 16: mx.setPoint(4, 3, data); break;  //top1
    case 17: mx.setPoint(0, 4, data); break;  //top2
    case 18: mx.setPoint(1, 4, data); break;  //top3
    case 19: mx.setPoint(2, 4, data); break;  //top4
    case 20: mx.setPoint(3, 4, data); break;  //top5
    case 21: mx.setPoint(4, 4, data); break;  //top6
    case 22: mx.setPoint(5, 4, data); break;  //top7
    case 23: mx.setPoint(7, 3, data); break;  //top8
    case 24: mx.setPoint(5, 3, data); break;  //top8
    case 25: mx.setPoint(6, 4, data); break;  //top10

    case 40: mx.setPoint(6, 3, data); break;  //patt
    case 41: mx.setPoint(2, 3, data); break;  //step
    case 42: mx.setPoint(3, 3, data); break;  //function

    case 50: mx.setPoint(7, 4, data); break;  //run
    case 51: mx.setPoint(0, 3, data); break;  //tempo
    case 52: mx.setPoint(1, 3, data); break;  //rec
  }
}



void wipe() {
  wipeStep = 0;
  wiping = true;
}
void topRow(uint8_t number) {
  if (sceneModeActive) {
    renderSceneModeLeds();
    return;
  }

  for (int16_t i = 0; i < 10; i++) {
    if (number == i) {
      setLed(i + 16, 1);
    } else {
      setLed(i + 16, 0);
    }
  }
}

void showTricks() {
  switch (trickStepEdit) {

    case 1:

      topRow(stutterSteps[currentPattern][lastBottomButton] - 1);

      //STUTTER
      switch (selectedPage) {
        case 0:
          for (int16_t i = 0; i < 16; i++) {
            setLed(i, stutterSteps[currentPattern][i]);
          }
          break;
        case 1:
          for (int16_t i = 0; i < 16; i++) {
            setLed(i, stutterSteps[currentPattern][i + 16]);
          }
          break;
        case 2:
          for (int16_t i = 0; i < 16; i++) {
            setLed(i, stutterSteps[currentPattern][i + 32]);
          }
          break;
        case 3:
          for (int16_t i = 0; i < 16; i++) {
            setLed(i, stutterSteps[currentPattern][i + 48]);
          }
          break;
      }
      break;

    case 2:

      clearTop();

      //BEND
      switch (selectedPage) {
        case 0:
          for (int16_t i = 0; i < 16; i++) {
            setLed(i, bendSteps[currentPattern][i]);
          }
          break;
        case 1:
          for (int16_t i = 0; i < 16; i++) {
            setLed(i, bendSteps[currentPattern][i + 16]);
          }
          break;
        case 2:
          for (int16_t i = 0; i < 16; i++) {
            setLed(i, bendSteps[currentPattern][i + 32]);
          }
          break;
        case 3:
          for (int16_t i = 0; i < 16; i++) {
            setLed(i, bendSteps[currentPattern][i + 48]);
          }
          break;
      }
      break;

    case 3:

      clearTop();

      //FREEZE
      switch (selectedPage) {
        case 0:
          for (int16_t i = 0; i < 16; i++) {
            setLed(i, freezeSteps[currentPattern][i]);
          }
          break;
        case 1:
          for (int16_t i = 0; i < 16; i++) {
            setLed(i, freezeSteps[currentPattern][i + 16]);
          }
          break;
        case 2:
          for (int16_t i = 0; i < 16; i++) {
            setLed(i, freezeSteps[currentPattern][i + 32]);
          }
          break;
        case 3:
          for (int16_t i = 0; i < 16; i++) {
            setLed(i, freezeSteps[currentPattern][i + 48]);
          }
          break;
      }
      break;
  }
}
void topRow5(uint8_t number) {

  for (int16_t i = 0; i < 5; i++) {
    if (number == i) {
      setLed(i + 16, 1);
    } else {
      setLed(i + 16, 0);
    }
  }
}

void bottomRow(uint8_t number) {

  for (int16_t i = 0; i < 16; i++) {
    if (number == i) {
      setLed(i, 1);
    } else {
      setLed(i, 0);
    }
  }
}

void clearBottom() {

  for (int16_t i = 0; i < 16; i++) {
    setLed(i, 0);
  }
}

void allOn() {
  for (int16_t i = 0; i < 10; i++) {
    setLed(i + 16, 1);
  }

  for (int16_t i = 0; i < 16; i++) {
    setLed(i, 1);
  }
}

void flashLeds() {
  allOn();
  swiping = 0;
}
void showVersion() {

  for (int16_t i = 0; i < 3; i++) {
    topRow(versionTop);
    delay(100);
    clearTop();
    delay(50);
  }
  delay(150);
  for (int16_t i = 0; i < 3; i++) {
    topRow(versionBottom);
    delay(100);
    clearTop();
    delay(50);
  }
}

void chase() {

  clearTop();
  clearBottom();

  for (int16_t i = 0; i < 16; i++) {
    setLed(i, 1);
    delay(33);
    setLed(i, 0);
  }
  if (mode == 1) {
    showPattern();
  } else {
    showMode();
  }
}


void showPattern() {

  if (!keyChangeEdit) {

    clearBottom();
    switch (mode) {

      case 0:
        if (pendingPattern > -1 && notPc) {
          clearBottom();
          bottomRow(pendingPattern);
        } else {
          bottomRow(currentPattern);
        }
        topRow(currentKit % 10);

        break;

      case 1:

        if (trickStepEdit) {
          showTricks();

        } else {

          if (((monitorSynth) && (selectedVoice > 5)) || ((rec) && (selectedVoice > 5))) {

            showPiano();
          } else {
            switch (selectedPage) {
              case 0:
                if (selectedVoice < 6) {
                  for (int16_t i = 0; i < 16; i++) {
                    setLed(i, bitRead(pattern[currentPattern][0][i], selectedVoice));
                  }
                } else {
                  for (int16_t i = 0; i < 16; i++) {
                    if (selectedVoice == 8) {
                      if ((getStep(selectedVoice - 6, i)) || (getStep2(i))) {
                        setLed(i, 1);
                      } else {
                        setLed(i, 0);
                      }
                    } else {
                      setLed(i, getStep(selectedVoice - 6, i));
                    }
                  }
                }
                break;
              case 1:
                if (selectedVoice < 6) {
                  for (int16_t i = 0; i < 16; i++) {
                    setLed(i, bitRead(pattern[currentPattern][0][i + 16], selectedVoice));
                  }
                } else {
                  for (int16_t i = 0; i < 16; i++) {
                    if (selectedVoice == 8) {
                      if ((getStep(selectedVoice - 6, i + 16)) || (getStep2(i + 16))) {
                        setLed(i, 1);
                      } else {
                        setLed(i, 0);
                      }
                    } else {
                      setLed(i, getStep(selectedVoice - 6, i + 16));
                    }
                  }
                }
                break;
              case 2:
                if (selectedVoice < 6) {
                  for (int16_t i = 0; i < 16; i++) {
                    setLed(i, bitRead(pattern[currentPattern][0][i + 32], selectedVoice));
                  }
                } else {
                  for (int16_t i = 0; i < 16; i++) {
                    if (selectedVoice == 8) {
                      if ((getStep(selectedVoice - 6, i + 32)) || (getStep2(i + 32))) {
                        setLed(i, 1);
                      } else {
                        setLed(i, 0);
                      }
                    } else {
                      setLed(i, getStep(selectedVoice - 6, i + 32));
                    }
                  }
                }
                break;
              case 3:
                if (selectedVoice < 6) {
                  for (int16_t i = 0; i < 16; i++) {
                    setLed(i, bitRead(pattern[currentPattern][0][i + 48], selectedVoice));
                  }
                } else {
                  for (int16_t i = 0; i < 16; i++) {
                    if (selectedVoice == 8) {
                      if ((getStep(selectedVoice - 6, i + 48)) || (getStep2(i + 48))) {
                        setLed(i, 1);
                      } else {
                        setLed(i, 0);
                      }
                    } else {
                      setLed(i, getStep(selectedVoice - 6, i + 48));
                    }
                  }
                }
                break;
            }
          }

          break;
        }
    }
  }
}

void showPiano() {

  for (int16_t i = 0; i < 16; i++) {
    if ((i == 0) || (i == 2) || (i == 4) || (i == 5) || (i == 7) || (i == 9) || (i == 11) || (i == 12) || (i == 14)) {
      setLed(i, 1);
    } else {
      setLed(i, 0);
    }
  }
}

void blinkStepLed() {
  if (keyChangeEdit) {
    if (funcHeld) {
      setLed(musicRoot, flasher);
    } else {
      setLed(key, flasher);
    }
    setLed(LED_rec, flasher && rec);
  } else {

    /// FLASHER STUFF ///
    //Flash pages to skip (muted)
    if ((mode == 2) && (function == 5) && (muteHeld)) {
      if (pageSkip[0]) {
        setLed(16, flasher);
      }
      if (pageSkip[1]) {
        setLed(17, flasher);
      }
      if (pageSkip[2]) {
        setLed(18, flasher);
      }
      if (pageSkip[3]) {
        setLed(19, flasher);
      }
    }

    //Flash pages to skip (muted)
    if (mode == 2 && function == FUNC_ROUTE) {

      for (int i = 16; i < 26; i++) {
        if (recHeld) {
          setLed(i, flasher);
        } else if (funcHeld) {
          setLed(i, 1);
        }
      }
    }

    //Flash target folder or pattern to save
    if ((mode == 2) && (function == 0)) {
      setLed(LED_kitPat, flasher);

      clearBottom();
      clearTop();
      setLed(16 + currentFolder, flasher);
      setLed(currentSong, flasher);

    } else if ((lastStepMode) && (mode == 2)) {
      if (recHeld) {
        //flash the pages in page clone mode
        clearTop();
        setLed(16, flasher);
        setLed(17, flasher);
        setLed(18, flasher);
        setLed(19, flasher);
      }
      for (int16_t i = 0; i < 16; i++) {
        if (recHeld) {
          //hide the steps in page clone mode
          setLed(i, 0);
        } else {
          if (i < lastStepPoly[currentPattern][selectedVoice] + 1) {
            setLed(i, flasher);
          } else {
            setLed(i, 0);
          }
        }
      }

    } else if (mode == 0) {
      //KIT/PATT MODE
      if (kitHeld) {
        setLed(16, 1);
        setLed(16 + 1, 1);
        setLed(16 + 2, 1);
        setLed(16 + 3, 1);
        setLed(16 + kitVariation[currentPattern][kit[currentPattern]], flasher);
        //variation mode
      } else {

        if (pattArmed) {
          setLed(LED_rec, flasher);  //flash rec
        } else {
          setLed(LED_rec, rec2);
        }

        //show pending pattern
        if (pendingPattern > -1) {
          setLed(pendingPattern, flasher);
        }
        if (pattArmed) {
          setLed(currentPattern, flasher);
        }
      }
    } else if (mode == 1) {
      //STEP MODE
      if ((monitorSynth) && (selectedVoice > 5) && (!trickStepEdit)) {
        setLed(16 + selectedVoice, flasher);
      }

      if (showTimer) {
        setLed(22, 0);
        setLed(23, 0);
        setLed(24, 0);
        setLed(25, 0);

        if (!trickStepEdit) setLed(16 + selectedVoice, 1);
        setLed(16 + showing, flasher);
      }

      if (rec) {
        setLed(LED_rec, flasher);  //flash rec

        if ((selectedVoice > 5)) {
          //flash synth keys
          if (rec) {
            //stepRec mode (chase)
            //show Piano
            showPiano();
          }
        }

        //show target step when punching synths in stepRec
        if (((monitorSynth) || (rec)) && (stepRec) && (selectedVoice > 5) && (!run)) {
          uint8_t targetStepToFlash = targetStep;
          while (targetStepToFlash > 15) targetStepToFlash -= 16;
          setLed(targetStepToFlash, flasher);
        }
      }
    } else if (mode == 2) {
      if (rec) {
        setLed(LED_rec, flasher);  //flash rec
      }
    }
  }
}
