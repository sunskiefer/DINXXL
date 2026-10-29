void kit_bottom(uint8_t number, bool value) {

  if (value) {
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////                        PRESSED                                       /////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //clear kitAutomation
    if ((kitHeld) && (number == 2)) {
      for (int16_t i = 0; i < 32; i++) {
        kitAuto[currentPattern][i] = 0;
        saveAuto = true;
      }
    } else {
      if ((recHeld) && (number == 10)) {
        chainFreeze = !chainFreeze;
        flashLeds();
        noRecAction = false;
      } else {
        if ((rec2) && (!pattArmed)) {
          armPatt();
        }
        notPc = true;
        changePattern(number);
        //first press? select target to clone
        if (!pattHeldCounter) {
          pattCopy = number;
        } else {
          //clone pattern

          //clear automation at target pattern
          for (int16_t i = 0; i < autoThird; i++) {
            if (getPattern(i) == number) autoData[i] = 0;
          }
          vibratoPower[number] = vibratoPower[pattCopy];
          vibratoRateMultiplier[number] = vibratoRateMultiplier[pattCopy];
          tremRateMultiplier[number] = tremRateMultiplier[pattCopy];

          //copy over from source
          for (int16_t i = 0; i < 4; i++) {
            clonePlocPage(i, i, pattCopy, number);
          }

          kit[number] = kit[pattCopy];
          routeAuto.copyPattern(number, pattCopy);

          for (int16_t j = 0; j < 5; j++) {
            for (int16_t i = 0; i < 64; i++) {
              nudgeDelay[number][j][i] = nudgeDelay[pattCopy][j][i];
            }
          }
          for (int16_t i = 0; i < 64; i++) {
            pattern[number][0][i] = pattern[pattCopy][0][i];
            pattern[number][1][i] = pattern[pattCopy][1][i];
            synth[number][0][i] = synth[pattCopy][0][i];
            synth[number][1][i] = synth[pattCopy][1][i];
            synth[number][2][i] = synth[pattCopy][2][i];
            synth[number][3][i] = synth[pattCopy][3][i];
            stutterSteps[number][i] = stutterSteps[pattCopy][i];
          }
          bendSteps.copyRow(number, pattCopy);
          freezeSteps.copyRow(number, pattCopy);

          //here go the optimiused automation data (reduced to 32 step (split in 4bit nibbles))
          for (int16_t i = 0; i < 32; i++) {
            wiggleAuto[number][i] = wiggleAuto[pattCopy][i];
            kitAuto[number][i] = kitAuto[pattCopy][i];
            vibAuto[number][i] = vibAuto[pattCopy][i];
            tremAuto[number][i] = tremAuto[pattCopy][i];
            tremAutoWav[number][i] = tremAutoWav[pattCopy][i];
            vibAutoWav[number][i] = vibAutoWav[pattCopy][i];
            swingAuto[number][i] = swingAuto[pattCopy][i];
            phonesAuto[number][i] = phonesAuto[pattCopy][i];
            mixAuto[number][i] = mixAuto[pattCopy][i];
            keyChange[number][i] = keyChange[pattCopy][i];
          }
          saveAuto = true;
          tremRate[number] = tremRate[pattCopy];
          vibratoRate[number] = vibratoRate[pattCopy];
          tremDepth[number] = tremDepth[pattCopy];
          vibratoDepth[number] = vibratoDepth[pattCopy];
          for (int16_t i = 0; i < 10; i++) {
            offsetOct[number][i] = offsetOct[pattCopy][i];
            lastStepPoly[number][i] = lastStepPoly[pattCopy][i];
            pages[number][i] = pages[pattCopy][i];
          }
          keyRoot[number] = keyRoot[pattCopy];
          musicModes[number] = musicModes[pattCopy];
          randomDepth[number] = randomDepth[pattCopy];
          swingSetting[number] = swingSetting[pattCopy];
          currentPattern = number;
          flashLeds();
        }
        pattHeldCounter++;
      }
    }
  } else {

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////                        RELEASED                                       /////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (pattHeldCounter) pattHeldCounter--;
  }
}


void kit_top(uint8_t number, bool value) {

  if (value) {
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////                        PRESSED                                       /////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////
    //   KITT/PATT MODE     //
    /////////////////////

    if (kitHeld && number < 4) {
      if (!kitHeldCounter) {
        //we are changing first variation
        kitVariation[currentPattern][kit[currentPattern]] = number;
        updateCurrentKit();
        saveAuto = true;
        resetRegisters = true;
        copyVariation = number;
      } else {

        //we copied a variation - paste it!
        for (int i = 0; i < 152; i++) {
          opl[kit[currentPattern] + (number * 10)][i] = opl[kit[currentPattern] + (copyVariation * 10)][i];
        }
        kitVariation[currentPattern][kit[currentPattern]] = number;
        updateCurrentKit();
        saveAuto = true;
        resetRegisters = true;
        flashLeds();
      }
      kitHeldCounter++;
    } else {
      if (!kitHeldCounter) {
        kitCopy = number;
      } else {
        //clone kit
        for (int16_t i = 0; i < 152; i++) {
          opl[number + (kitVariation[currentPattern][number] * 10)][i] = opl[kitCopy + (kitVariation[currentPattern][kitCopy] * 10)][i];
        }
      }
      kitHeldCounter++;
      //change kit
      //
      if (rec2) {
        if (getKitAuto(polyStep[0]) != number + 1) {
          setKitAuto(polyStep[0], number + 1);
          saveAuto = true;
        }
      }
      changeKit(number);
      showMode();
      if (!run) {
        fire();
      }
    }
  } else {

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////                        RELEASED                                       /////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (number == copyVariation - 1) { copyVariation = 0; }
  }
}
