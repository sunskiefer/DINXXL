#include "pattern_visualizer.h"

void step_bottom(uint8_t number, bool value) {


  if (value) {


    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////                        PRESSED                                       /////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /////////////////////
    // STEPS    0-15  //
    /////////////////////

    if (keyChangeEdit) {
      if (number < 15) {
        //key change
        if (rec) {
          setKeyChange(targetStep, number + 1);
        } else {
          keyRoot[currentPattern] = number;
        }
        if (funcHeld) {
          musicRoot = number;
        } else {
          key = number;
        }
      }
    } else {

      //enable / disable stutter steps



      if (recHeld) {
        // picking/placing
        if (selectedVoice > 5) {
          //synths
          if (pickNote == -1) {
            noRecAction = false;
            pickNote = getNote(selectedVoice - 6, number + x16[selectedPage]);
            pickStep = getStep(selectedVoice - 6, number + x16[selectedPage]);
            pickStepNumber = number;

          } else {

            //placing
            setNote(selectedVoice - 6, number + x16[selectedPage], pickNote);
            setStep(selectedVoice - 6, number + x16[selectedPage], pickStep);

            clonePlocStep(number);
          }
        } else {
          //drums
          if (pickNote == -1) {
            noRecAction = false;
            pickNote = 1;
            pickStep = bitRead(pattern[currentPattern][0][number + x16[selectedPage]], selectedVoice);
            pickStepNumber = number;

          } else {
            //placing

            uint8_t previous = bitRead(pattern[currentPattern][0][number + x16[selectedPage]], selectedVoice);
            bitWrite(pattern[currentPattern][0][number + x16[selectedPage]], selectedVoice, pickStep);
            if (previous != pickStep) {
              markPatternVisualizerDirty();
            }
            clonePlocStep(number);
          }
        }

        showMode();
      }

      //not picking/placing
      else {



        //enter/exit trick step edit
        if (stepHeld) {
          switch (number) {
            case 8:
              dontChangePage = true;
              if (trickStepEdit == 1) {
                trickStepEdit = 0;
                justChangedToTrickStepEdit = false;
              } else {
                trickStepEdit = 1;
                justChangedToTrickStepEdit = true;
              }


              mode = 1;
              showMode();
              break;
            case 9:
              dontChangePage = true;
              if (trickStepEdit == 2) {
                trickStepEdit = 0;
                justChangedToTrickStepEdit = false;
              } else {
                trickStepEdit = 2;
                justChangedToTrickStepEdit = true;
              }
              mode = 1;
              showMode();
              break;
            case 10:
              dontChangePage = true;
              if (trickStepEdit == 3) {
                trickStepEdit = 0;
                justChangedToTrickStepEdit = false;
              } else {
                trickStepEdit = 3;
                justChangedToTrickStepEdit = true;
              }
              mode = 1;
              showMode();
              break;
          }
        }
        if (trickStepEdit && !funcHeld) {
          if (justChangedToTrickStepEdit) {
            justChangedToTrickStepEdit = false;
            showPattern();
            showMode();
          } else {
            switch (trickStepEdit) {

              case 1:
                //STUTTER
                //is it lit
                if (stutterSteps[currentPattern][number + x16[selectedPage]]) {
                  //are we pressing same button again = toggle
                  if (lastBottomButton == number + x16[selectedPage]) {
                    if (stutterSteps[currentPattern][number + x16[selectedPage]]) {
                      stutterDefault = stutterSteps[currentPattern][number + x16[selectedPage]];
                      stutterSteps[currentPattern][number + x16[selectedPage]] = 0;
                    } else {
                      stutterSteps[currentPattern][number + x16[selectedPage]] = stutterDefault;
                      saveAuto = true;
                    }
                  }
                } else {
                  //not lit, light up
                  stutterSteps[currentPattern][number + x16[selectedPage]] = stutterDefault;
                  saveAuto = true;
                }

                break;

              case 2:  //BEND
                bendSteps[currentPattern][number + x16[selectedPage]] = !bendSteps[currentPattern][number + x16[selectedPage]];
                saveAuto = true;
                break;

              case 3:  //FREEZE
                freezeSteps[currentPattern][number + x16[selectedPage]] = !freezeSteps[currentPattern][number + x16[selectedPage]];
                saveAuto = true;
                break;
            }
          }

          showPattern();

          //end trick step
        } else {

          if (stepHeld) {
          } else {
            //  SYNTH VOICES
            if (selectedVoice > 5) {

              //no ploc if monitorSynth is on
              if ((monitorSynth) || (rec)) {
                triggerSynthWithButton(number, 1);
              } else {
                plocStep = number + 1;
                plocked = false;
                justTransposed = false;
                nudgeDelayTimer = 120;
              }

            } else {

              // DRUM VOICES
              if (rec) {
                bool previous = bitRead(pattern[currentPattern][0][number + x16[selectedPage]], selectedVoice);
                bool next = !previous;
                bitWrite(pattern[currentPattern][0][number + x16[selectedPage]], selectedVoice, next);
                if (previous != next) {
                  markPatternVisualizerDirty();
                }
                showPattern();
              } else {
                nudgeDelayTimer = 120;
                justTransposed = false;
                plocStep = number + 1;
                plocked = false;
              }
            }
          }
        }
      }
      lastBottomButton = number + x16[selectedPage];
    }

  } else {

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////                        RELEASED                                       /////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////// STEP MODE ///////

    if (tempoHeld && number == FUNC_STUTTER) {
      drill = !drill;
      blink();
      tempoTaps = 0;
    }

    else if (funcHeld && number == FUNC_STUTTER) {
      stutterEnabled = !stutterEnabled;
      if (stutter == 2) stutter = 0;
      blink();
      showMode();
      noFuncAction = false;
    }

    else if (funcHeld && number == FUNC_BEND) {
      autoBendEnabled = !autoBendEnabled;

      if (!autoBendEnabled && bendStepLast) {
        stopBend();
        bendStepLast = 0;
      }
      blink();
      showMode();
      noFuncAction = false;
    }

    else if (funcHeld && number == FUNC_FREEZE) {
      autoFreezeEnabled = !autoFreezeEnabled;
      if (!autoFreezeEnabled && freeze) {
        opl2.freeze(0);
        freeze = false;
        resetRegisters = true;
      }
      blink();
      showMode();
      noFuncAction = false;
    }

    else {

      if (!keyChangeEdit && !stepHeld && !trickStepEdit) {

        //  SYNTH VOICES
        if (selectedVoice > 5) {

          //synth off

          //no ploc if monitorSynth is on
          if (plocked) {
            plocStep = 0;

          } else {

            if ((!rec) && (!monitorSynth) && (!justTransposed) && (!recHeld)) {

              triggerSynthWithButton(number, 1);
              triggerSynthWithButton(number, 0);
            }
          }

          if ((rec) || (monitorSynth)) {
            triggerSynthWithButton(number, 0);
          }


        } else {
          // DRUM VOICES
          if ((!rec) && (!recHeld)) {

            if (!plocked) {
              bool previous = bitRead(pattern[currentPattern][0][number + x16[selectedPage]], selectedVoice);
              bool next = !previous;
              bitWrite(pattern[currentPattern][0][number + x16[selectedPage]], selectedVoice, next);
              if (previous != next) {
                markPatternVisualizerDirty();
              }
              showPattern();
              plocStep = 0;
            } else {
              plocStep = 0;
            }
          }
        }
      }
    }
  }
}


void step_top(uint8_t number, bool value) {

  if (value) {
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////                        PRESSED                                       /////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /////////////////////
    //   STEP MODE     //
    /////////////////////

    if (nudgeMode == 2) {
      setNudge(number);
      showNudge();
    } else {
      if (doubleClickTimer) {

        if (lastDouble == number) {
          //double click, toggle monitorSynth
          monitorSynth = !monitorSynth;
        }
      } else {
        doubleClickTimer = doubleClick_timeout;
        lastDouble = number;
      }



      if (stepHeld) {
        trickStepEdit = 0;
        showMode();
      }

      if (trickStepEdit) {

        //change stutter speed
        if (trickStepEdit == 1) {
          stutterSteps[currentPattern][lastBottomButton] = stutterDefault = number + 1;
          showMode();
        }


      } else {
        //clone synth when changing to new synth voice whilst holding another
        if (number > 5) {
          if (!topHeldCounter) {
            synthCopy = number - 6;
          } else {
            //clone kit

            for (int16_t i = 0; i < 28; i++) {
              opl[currentKit][i + 36 + ((number - 6) * 28)] = opl[currentKit][i + 36 + (synthCopy * 28)];
            }
          }
          topHeldCounter++;
        }


        showMode();



        if ((!run) || ((run) && (rec)) || ((midiSlave) && (rec)) || ((analogSlave) && (rec))) {

          switch (number) {
            //trigger manually
            case 0:
              midiHot[number] = true;
              opl[currentKit][0] = 0;
              bitWrite(opl[currentKit][0], 7, 1);
              bitWrite(opl[currentKit][0], 4, 1);
              pitchNow[0] = pitchRoot[0];
              pitchNow[1] = pitchRoot[1];
              break;  //kick
            case 1:
              midiHot[number] = true;
              opl[currentKit][0] = 0;
              bitWrite(opl[currentKit][0], 7, 1);
              bitWrite(opl[currentKit][0], 3, 1);
              pitchNow[2] = pitchRoot[2];
              break;  //snare
            case 2:
              midiHot[number] = true;
              opl[currentKit][0] = 0;
              bitWrite(opl[currentKit][0], 7, 1);
              bitWrite(opl[currentKit][0], 2, 1);
              pitchNow[3] = pitchRoot[3];
              break;  //tom
            case 3:
              midiHot[number] = true;
              opl[currentKit][0] = 0;
              bitWrite(opl[currentKit][0], 7, 1);
              bitWrite(opl[currentKit][0], 0, 1);
              pitchNow[4] = pitchRoot[4];
              opl2.setRelease(7, OPERATOR1, hatDecay);
              break;  //ch
            case 4:
              midiHot[number] = true;
              opl[currentKit][0] = 0;
              bitWrite(opl[currentKit][0], 7, 1);
              bitWrite(opl[currentKit][0], 0, 1);
              pitchNow[4] = pitchRoot[4];
              opl2.setRelease(7, OPERATOR1, hatDecayOpen);
              break;  //oh
            case 5:
              midiHot[number] = true;
              opl[currentKit][0] = 0;
              bitWrite(opl[currentKit][0], 7, 1);
              bitWrite(opl[currentKit][0], 1, 1);
              pitchNow[4] = pitchRoot[4];
              break;  //cym
            case 6 ... 9:

              if (!rec) {
                triggerSynth(number - 6, lastOctPlayed[number - 6], lastNotePlayed[number - 6], 1);
              }


              break;  //synths
          }
          if (rec) {
            if (number < 6) {
              // drum voices
              bitWrite(pattern[currentPattern][0][targetStep], selectedVoice, 1);
              setNudgeAtStep(selectedVoice);
            }
          }
        }

        if (funcHeld) {
          //Quantize
          for (int s = 0; s < 64; s++) {
            setNudgeAnyValueAtAnyStep(s, number, selectedVoice);
          }
          noFuncAction = false;
        } else {
          selectedVoice = probTarget = number;
        }



        //interrtup blinking page...
        if (showTimer > 1) showTimer = 1;

        if (number > 5) {
          selectedSynth = number - 6;
        }


        showMode();

        //show the synth algo
        switch (selectedVoice) {
          case 6:
            showNumberSynth(map(opl[currentKit][39], 0, 15, 0, 4));  //show algo
            break;

          case 7:
            showNumberSynth(map(opl[currentKit][28 + 39], 0, 15, 0, 4));  //show algo
            break;

          case 8:
            showNumberSynth(map(opl[currentKit][56 + 39], 0, 15, 0, 4));  //show algo
            break;

          case 9:
            showNumberSynth(map(opl[currentKit][84 + 39], 0, 15, 0, 4));  //show algo
            break;
        }
      }
    }
  } else {
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////                        REALEASED                                       /////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  }
}
