


#include "scenes.h"

void buttPressed(uint8_t number, bool value) {

  visualiserTimeout = millis() + visualiserTimeoutMS;

  if (channelSetup) {
    if (!value) {
      switch (number) {
        case 28:
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
          midiSetupTarget = 0;
          clearTop();
          setLed(16, 1);
          setLed(17, 1);
          setLed(18, 1);
          setLed(19, 1);
          setLed(20, 1);
          setLed(21, 1);
          bottomRow(ch10 - 1);
          break;

        case 5:
          clearTop();
          midiSetupTarget = 1;
          setLed(22, 1);
          bottomRow(ch1 - 1);
          break;
        case 31:
          clearTop();
          midiSetupTarget = 2;
          setLed(23, 1);
          bottomRow(ch2 - 1);
          break;
        case 29:
          clearTop();
          midiSetupTarget = 3;
          setLed(24, 1);
          bottomRow(ch3 - 1);
          break;
        case 6:
          clearTop();
          midiSetupTarget = 4;
          setLed(25, 1);
          bottomRow(ch4 - 1);
          break;

        case 8 ... 23:
          if (number > 15) number -= 16;

          switch (midiSetupTarget) {

            case 0:
              if (ch10 == number + 1) {
                ch10 = 0;
                EEPROMupdate(9, 0);
                clearBottom();
              } else {
                ch10 = number + 1;
                EEPROMupdate(9, number + 1);
                bottomRow(number);
              }
              break;
            case 1:
              if (ch1 == number + 1) {
                ch1 = 0;
                EEPROMupdate(10, 0);
                clearBottom();
              } else {
                ch1 = number + 1;
                EEPROMupdate(10, number + 1);
                bottomRow(number);
              }
              break;
            case 2:
              if (ch2 == number + 1) {
                ch2 = 0;
                EEPROMupdate(11, 0);
                clearBottom();
              } else {
                ch2 = number + 1;
                EEPROMupdate(11, number + 1);
                bottomRow(number);
              }
              break;
            case 3:
              if (ch3 == number + 1) {
                ch3 = 0;
                EEPROMupdate(12, 0);
                clearBottom();
              } else {
                ch3 = number + 1;
                EEPROMupdate(12, number + 1);
                bottomRow(number);
              }
              break;
            case 4:
              if (ch4 == number + 1) {
                ch4 = 0;
                EEPROMupdate(13, 0);
                clearBottom();
              } else {
                ch4 = number + 1;
                EEPROMupdate(13, number + 1);
                bottomRow(number);
              }
              break;
          }

          break;
      }
    }
  } else {


    if (!value) {
      //pressed

      switch (number) {

        case 28: topButt(0, value); break;
        case 0: topButt(1, value); break;
        case 1: topButt(2, value); break;
        case 2: topButt(3, value); break;
        case 3: topButt(4, value); break;
        case 4: topButt(5, value); break;
        case 5: topButt(6, value); break;
        case 31: topButt(7, value); break;
        case 29: topButt(8, value); break;
        case 6: topButt(9, value); break;

        /////////////////////
        //   MODE BUTTONS  //
        /////////////////////
        case BUTTON_KIT:

          if (!value) {
            kitHeld = true;
            if (sceneModeActive && !stepHeld) {
              exitSceneMode();
              break;
            }
            if (!sceneModeActive && stepHeld) {
              enterSceneMode();
              break;
            }
          }

          if (mode == 2 && function == FUNC_VIBR && vibHeld) {
            vibratoRateMultiplier[currentPattern] = 0;
            showMode();
          } else if (mode == 2 && function == FUNC_TREM && tremHeld) {
            tremRateMultiplier[currentPattern] = 0;
            showMode();
          }


          else {


            keyChangeEdit = muteHeld = false;
            if (mode == 4) {
            } else {
              if (mode == 3) {
                mode = 0;
                showMode();
              } else {
                if (funcHeld) {


                  if ((millis() - funcHeldTime) < 100u) {
                    mode = 3;
                    showMode();
                  } else {
                    startSD();
                    loadKit(kitVariation[currentPattern][kit[currentPattern]], kit[currentPattern]);
                    endSD();
                    flashLeds();
                    noFuncAction = false;
                    //reload kit from sd
                  }


                }

                else if ((mode == 1) && (plocStep) && (selectedVoice > 5)) {

                  uint8_t tempNote = getNote(selectedVoice - 6, x16[selectedPage] + plocStep - 1);
                  if (tempNote) {
                    tempNote--;
                    setNote(selectedVoice - 6, x16[selectedPage] + plocStep - 1, tempNote);
                    triggerSynth(selectedVoice - 6, 0, tempNote, 1);
                  }
                  justTransposed = true;
                }

                else if ((mode == 1) && (stepHeld)) {
                  rotate(0);
                } else {

                  if ((mode == 1) && (heldTop)) {

                    //single click?!
                    if ((!doubleClickTimer) || ((doubleClickTimer) && (lastDouble != 0))) {
                      lastDouble = 0;
                      doubleClickTimer = doubleClick_timeout;

                      if (offsetOct[currentPattern][selectedVoice] == 0) {
                        offsetOct[currentPattern][selectedVoice] = -1;
                      } else {
                        offsetOct[currentPattern][selectedVoice] = 0;
                      }
                      showMode();
                      updateOctaves();
                    } else if ((doubleClickTimer) && (lastDouble == 0)) {
                      //DOUBLE CLICK
                      for (int16_t i = 0; i < 10; i++) {
                        offsetOct[currentPattern][i] = 0;
                      }
                      showMode();
                      updateOctaves();
                    }

                  } else if ((mode == 2) && (function == 0)) {

                    if (run) {
                      saveKitFlag = true;
                    } else {
                      saveKit();
                    }
                    //save kit

                    function = 16;
                    showMode();




                  }



                  else {


                    if (wiggleHeld) {

                      shuffleKit();


                    } else {

                      if (clearHeld) {
                        clearPattern();

                      }


                      else {
                        trickStepEdit = 0;


                        if (recHeld) {
                          punchOffset = 0;
                          noRecAction = false;
                          showMode();
                        } else {

                          if (mode) {


                            mode = 0;
                            lastStepMode = false;
                            showMode();
                          } else {
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            kitHeld = true;
            keyChangeEdit = false;
            copyVariation = 0;
          }
          break;  //kit/pattern







        case BUTTON_STEP:

          if (!value) {
            stepHeld = true;
            if (sceneModeActive && !kitHeld) {
              exitSceneMode();
              break;
            }
            if (!sceneModeActive && kitHeld) {
              enterSceneMode();
              break;
            }
          }

          if (mode == 2 && function == FUNC_VIBR && vibHeld) {
            vibratoRateMultiplier[currentPattern] = 1;
            showMode();
          } else if (mode == 2 && function == FUNC_TREM && tremHeld) {
            tremRateMultiplier[currentPattern] = 1;
            showMode();
          }


          else {

            if (funcHeld) {
              chaseAllPages = !chaseAllPages;
              EEPROMupdate(112, chaseAllPages);
              flashLeds();
              dontChangePage = true;
              noFuncAction = false;
            }
            if (lastStepMode) {
              stepHeld = true;
              changedSelectedVoice = false;
              showMode();
              //offer to select another track without exiting lastStepMode
            } else {
              if (!clearHeld) {
                keyChangeEdit = muteHeld = false;
                if (mode == 4) {
                } else {
                  if ((mode == 1) && (heldTop)) {

                    //single click?!
                    if ((!doubleClickTimer) || ((doubleClickTimer) && (lastDouble != 1))) {
                      lastDouble = 1;
                      doubleClickTimer = doubleClick_timeout;

                      offsetOct[currentPattern][selectedVoice] = 1;
                      showMode();
                      updateOctaves();
                    } else if ((doubleClickTimer) && (lastDouble == 1)) {
                      //DOUBLE CLICK
                      for (int16_t i = 0; i < 10; i++) {
                        offsetOct[currentPattern][i] = 1;
                      }
                      showMode();
                      updateOctaves();
                    }
                  } else {
                    if (wiggleHeld) {
                      shufflePattern();
                    } else {
                      if (mode == 1) {
                        dontChangePage = false;
                      } else {
                        dontChangePage = true;
                      }
                      stepHeld = true;
                      if (keyChangeEdit) {
                        keyChangeEdit = false;
                      }
                      if (trickStepEdit) {
                        bottomRow(7 + trickStepEdit);
                      } else {
                        if (recHeld) {
                          punchOffset = 12;
                          noRecAction = false;
                          showMode();
                        } else {
                          if (mode != 1) {
                            rec2 = false;
                            mode = 1;
                            lastStepMode = false;
                            showMode();
                            showPattern();
                          } else {
                            showMode();
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
          break;  //step

        case BUTTON_FUNCTION:

          if (sceneModeActive) {
            funcHeld = true;
            funcHeldTime = millis();
            break;
          }

          if (mode == 2 && function == FUNC_VIBR && vibHeld) {
            vibratoRateMultiplier[currentPattern] = 2;
            showMode();
          } else if (mode == 2 && function == FUNC_TREM && tremHeld) {
            tremRateMultiplier[currentPattern] = 2;
            showMode();
          }


          else {

            if (mode == 4) {
            } else {
              if (!keyChangeEdit) {
                muteHeld = false;
                if (mode == 3) {
                  mode = 2;
                  if (function == 1) lastStepMode = true;
                  showMode();
                } else if ((mode == 1) && (plocStep) && (selectedVoice > 5)) {
                  uint8_t tempNote = getNote(selectedVoice - 6, x16[selectedPage] + plocStep - 1);
                  tempNote++;
                  setNote(selectedVoice - 6, x16[selectedPage] + plocStep - 1, tempNote);
                  triggerSynth(selectedVoice - 6, 0, tempNote, 1);
                  justTransposed = true;
                  noFuncAction = false;
                } else if (stepHeld) {
                  rotate(1);
                } else {
                  if (kitHeld) {
                    mode = 3;
                    showMode();
                  } else {
                    if ((mode == 1) && (heldTop)) {
                      noFuncAction = false;
                      //single click?!
                      if ((!doubleClickTimer) || ((doubleClickTimer) && (lastDouble != 2))) {
                        lastDouble = 2;
                        doubleClickTimer = doubleClick_timeout;
                        offsetOct[currentPattern][selectedVoice] = 2;
                        showMode();
                        updateOctaves();
                      } else if ((doubleClickTimer) && (lastDouble == 2)) {
                        //DOUBLE CLICK
                        for (int16_t i = 0; i < 10; i++) {
                          offsetOct[currentPattern][i] = 2;
                        }
                        showMode();
                        updateOctaves();
                      }
                    } else {
                      //when function is released if nothing (like rec button for undo) set this to false then we change to function mode
                      noFuncAction = true;
                    }
                  }
                }
              } else {
                showMode();
              }
            }
            funcHeld = true;
            funcHeldTime = millis();
            if (function == 1 || function == FUNC_VIBR) showMode();
          }
          break;  //function

        case BUTTON_TEMPO:
          if (recHeld && !funcHeld) {
            noRecAction = false;
            quantise = !quantise;
            flashLeds();
            if (quantise) {
              EEPROMupdate(29, 0);
            } else {
              EEPROMupdate(29, 1);
            }
          } else {
            if (funcHeld && !recHeld) {
              noFuncAction = false;
              metro = !metro;
              flashLeds();
              EEPROMupdate(23, metro);
              setMetronome(metro);
            } else if (funcHeld && recHeld) {
              noFuncAction = false;
              noRecAction = false;
              OnePpq = !OnePpq;
              flashLeds();
              EEPROMupdate(114, OnePpq);
            } else {
              tempoHeld = true;
              tempoPhase = 0;
              if (tempoTaps == 0) {
                tapCounter = 0;
              }
              tempoTaps++;

              if (tempoTaps == 5) {
                tempoTaps = 0;
                tempo = counterToBPM(tapCounter);
                tapCounter = 0;
                updateSwing(swingSetting[currentPattern]);
                uClock.setTempo(tempo);
                if (millis() > 2000) {
                  EEPROMupdate(26, lowByte(tempo));
                  EEPROMupdate(27, highByte(tempo));
                }
                if ((!run) && (!analogSlave) && (!midiSlave)) {
                  startSeq();
                  fire();
                  sendSync();
                }
              }
            }
          }
          break;  //tempo

        case BUTTON_REC:

          recHeld = true;
          if (mode == 2 && function == FUNC_LENGTH) { pageCopyTarget = 4; }
          noRecAction = true;
          pickNote = -1;
          if (funcHeld) {
            noRecAction = false;
            noFuncAction = false;
            undoRedo();
          }
          if ((clearHeld) && (mode == 2)) {
            if (alreadyClearedAuto) {
              clearAutoPAttern();
            } else {
              clearTrack(selectedVoice);
              alreadyClearedAuto = true;
            }
            noRecAction = false;
          } else {
            showMode();
          }
          break;  //rec

          break;  //change voice
        case 8 ... 23:
          if (number > 15) number -= 16;

          //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

          ////////////////////                         BOTTOM BUTTONS PRESSED                         /////////////////////////////////

          ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          if (sceneModeActive) {
            sceneHandleBottomButton(number, value);
            bottomButtonLimitCounter = 0;
            break;
          }

          bottomButtonLimitCounter++;
          if (bottomButtonLimitCounter < bottomButtonLimit) {

            switch (mode) {
              case MODE_VGM:
                ///////////////////////////////////////////////////////////////////////////
                //                          vgm mode                                     //
                ///////////////////////////////////////////////////////////////////////////
                if (!vgmState) {
                  if (number + 1 <= fileCount) {
                    playFile(number);
                  }
                }
                break;
              ///////////////////////////////////////////////////////////////////////////
              //                          song mode                                    //
              ///////////////////////////////////////////////////////////////////////////
              case MODE_SONG:
                if (folderChanged) {
                  folderChanged = false;
                  currentSong = number;
                  currentFolder = pendingFolder;
                  EEPROMupdate(17, currentFolder);
                  EEPROMupdate(16, currentSong);
                  stopSeq();
                  seqReset();
                } else {
                  currentSong = number;
                  EEPROMupdate(16, currentSong);
                }
#if defined(__MK20DX256__)  // Macro for Teensy 3.1/3.2

                startSkip = 1;
                load();
                showMode();
#elif defined(STM32F1) || defined(STM32F4)  // Code specific to STM32
                load();
                showMode();

#endif

                break;
              case MODE_PATT_KIT:
                //pattern/kit  mode
                kit_bottom(number, 1);
                break;
              case MODE_FUNCTION:
                if (number == FUNC_TREM && funcHeld && (mode != 2 || function != FUNC_LENGTH)) {
                  tremRateMultiplier[currentPattern] = !tremRateMultiplier[currentPattern];
                  flashLeds();
                  noFuncAction = false;
                } else if (number == FUNC_VIBR && funcHeld && (mode != 2 || function != FUNC_LENGTH)) {
                  vibratoRateMultiplier[currentPattern] = !vibratoRateMultiplier[currentPattern];
                  flashLeds();
                  noFuncAction = false;
                } else {
                  //function mode pressed
                  function_bottom(number, 1);
                }
                break;
              case MODE_STEP:
                //step mode pressed
                step_bottom(number, 1);

                break;
            }
          }
          break;
      }
    } else {

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

      ////////////////////                        RELEASED                                       /////////////////////////////////

      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      switch (number) {
        case BUTTON_STEP:
          if (mode == 4) {
          } else {
            stepHeld = false;

            if (lastStepMode) {

              if (!changedSelectedVoice) {
                mode = 1;
                lastStepMode = 0;
                function = -1;
              }
              showMode();
            } else {
              if ((!dontChangePage) && (!recHeld) && (!heldTop)) {

                //insert a rest if we press edit
                if ((!run) && (stepRec) && (rec) && (selectedVoice > 5)) {



                  if (selectedVoice - 6 != 2) {
                    //mono voices
                    setStep(selectedVoice - 6, targetStep, 0);

                  } else {

                    setStep(selectedVoice - 6, targetStep, 0);
                    setStep2(targetStep, 0);
                  }
                  nextTargetStep();

                }


                else {

                  selectedPage++;
                  if (selectedPage > pages[currentPattern][selectedVoice]) {
                    selectedPage = 0;
                  }
                  showNumber(selectedPage);
                }
              }
              showMode();

              if (!keyChangeEdit) showPattern();
            }
          }
          break;  //step button

        case BUTTON_TEMPO:
          tempoHeld = false;

          //APPLY BPM
          if (tempoPhase > 1) {
            tempoEntry(10);
          }

          break;

        case BUTTON_KIT:
          kitHeld = false;
          showMode();
          break;  //kit

        case BUTTON_FUNCTION:
          if (sceneModeActive) {
            funcHeld = false;
            break;
          }

          if (mode != 3 && !keyChangeEdit) {
            if (noFuncAction) {

              if (mode == 2) {
                if ((noFuncAction) && (function == FUNC_SAVE || function == FUNC_LENGTH || function == FUNC_ROUTE)) {
                  function = -1;  //reset to main function menu
                  lastStepMode = 0;
                }
              }

              rec2 = false;
              trickStepEdit = 0;
              if (recHeld) {
                punchOffset = 24;
                noRecAction = false;
                showMode();
              } else {
                if (function < 1) {
                  function = 16;
                }
                rec2 = false;
                if (!stepHeld) {
                  mode = 2;
                  if (function == 1) lastStepMode = true;
                }
              }
            }
          }
          funcHeld = false;

          showMode();

          break;  //function

        case BUTTON_REC:
          recHeld = false;


          if (noRecAction) {

            if (mode == 0) {

              rec2 = !rec2;
              if (!rec2) {
                setLed(LED_rec, 0);

                if (pattArmed) {
                  pattArmed = false;
                }
                //we want to reset to beginning of chain
                pattChainStep = pattChainLength;
                showPattern();
              } else {

                createSnapShot();
              }
            } else {

              rec = !rec;

              if (!rec) {

                setMetronome(0);


                setLed(LED_rec, 0);

              } else {

                createSnapShot();

                if (!run) {
                  polyStep[0] = targetStep = 0;  //reset step if you want to restart punch
                }
              }
            }
          }
          showMode();

          break;


        case 28: topButt(0, value); break;
        case 0: topButt(1, value); break;
        case 1: topButt(2, value); break;
        case 2: topButt(3, value); break;
        case 3: topButt(4, value); break;
        case 4: topButt(5, value); break;
        case 5: topButt(6, value); break;
        case 31: topButt(7, value); break;
        case 29: topButt(8, value); break;
        case 6: topButt(9, value); break;

        case 8 ... 23:
          if (number > 15) number -= 16;

          //bottom 16
          if (sceneModeActive) {
            sceneHandleBottomButton(number, value);
            bottomButtonLimitCounter = 0;
            break;
          }

          bottomButtonLimitCounter--;
          if (bottomButtonLimitCounter < 1) {
            bottomButtonLimitCounter = 0;

            plocStep = 0;  //force ploc off when all buttons up... just in case

            if (mode == 1) {

              showMode();
            }
          }

          if (bottomButtonLimitCounter < bottomButtonLimit) {


            switch (mode) {

              case 1:
                //step released
                step_bottom(number, 0);


                break;

              case 0:
                kit_bottom(number, 0);


                break;

              case 2:
                function_bottom(number, 0);
                break;
            }











            break;
          }
          break;
      }
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////

/////////////////////
//   TOP BUTTONS   //
/////////////////////

/////////////////////////////////////////////////////////////////////////////////
void topButt(uint8_t number, bool value) {
  if (sceneModeActive) {
    sceneHandleTopButton(number, value);
    return;
  }

  if (keyChangeEdit) {

    musicModes[currentPattern] = number;

    showMode();
  } else {
    if (mode == 4) {

      if (!vgmState) {
        vgmFolder = number;
        vgm();
      }

    } else {
      if (tempoHeld) {
        if (!value) {
          tempoEntry(number);
        }
      } else {
        if (!number) {
          firstHeld = !value;
        }

        if (!value) {
          //pressed

          heldTop++;



          switch (mode) {

            case 3:

              /////////////////////
              //   SONG MODE     //
              /////////////////////

              //Change currentFolder
              pendingFolder = number;
              folderChanged = 1;
              showMode();

              break;

            case 0:

              kit_top(number, 1);
              break;

            case 1:

              step_top(number, 1);
              break;  //step


            case 2:

              function_top(number, 1);

              break;
          }
          lastTop = number;
        } else {
          //RELEASED
          //cut synth if we're previewing it
          if ((number > 5) && (mode == 2) && (!rec) && (!run)) {
            triggerSynth(number - 6, lastOctPlayed[number - 6], lastNotePlayed[number - 6], 0);
          }
          if ((number > 5) && (topHeldCounter)) {
            topHeldCounter--;
          }
          if (kitHeldCounter) kitHeldCounter--;


          ///remove stutter key from stack
          if (stutterPressed) {
            stutterPressed--;
            if (stutterPressed < 1) stutterPressed = 0;
            stutterHeld[number] = 0;
            sortStutter();
          }
          if (heldTop) {
            heldTop--;
          }


          //released


          if (bend) {
            stopBend();
          }
          bendHeld = false;

          if (nudgeMode != 2) showMode();
        }
      }
    }
  }
}

bool lastRun;
void runButton() {



  if (!runBounce) {
    if (digitalRead(RUN_PIN) != lastRun) {



      runBounce = 20;
      lastRun = !lastRun;



      if (lastRun) {  //run released

        tempoAlt = true;

      } else {  //run pressed

        if (funcHeld && run) {
          if (tremRate[currentPattern]) tremTickCounter = 0;
          if (vibratoRate[currentPattern]) vibTickCounter = 0;

          for (int i = 0; i < 10; i++) {
            allStepsZeroFlag = true;
            noFuncAction = false;
          }
        } else {
          if (clearHeld) {


            for (int i = 0; i < 32; i++) {
              keyChange[currentPattern][i] = 0;
            }
            saveAuto = true;

          } else {
            if (stepHeld) {
              //enter keyChange mode
              dontChangePage = true;
              keyChangeEdit = true;

              showMode();

            } else {
              if (recHeld) {
                noRecAction = false;
                stepRec = !stepRec;
                if (stepRec) chase();
                EEPROMwrite(22, stepRec);

              } else {



                if ((analogSlave) || (midiSlave)) {
                  seqReset();
                  fire();

                } else {
                  run = !run;
                  setLed(LED_run, run);

                  if (run) {


                    uClock.start();

                    fire();
                    sendSync();
                  } else {

                    stopSeq();
                    seqReset();
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
