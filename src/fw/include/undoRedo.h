



void createSnapShot() {

  changeUndo = true;
  undoPattern = currentPattern;

  for (int16_t i = 0; i < 500; i++) { autoDataUR[i] = autoData[i]; }



  for (int16_t i = 0; i < 10; i++) {
    lastStepUR[i] = lastStepPoly[currentPattern][i];
    pagesUR[i] = pages[currentPattern][i];
    for (int16_t j = 0; j < 152; j++) { oplUR[i][j] = opl[i][j]; }
  }
  for (int16_t j = 0; j < 152; j++) { randomerUR[j] = randomer[currentPattern][j]; }
  for (int16_t j = 0; j < 4; j++) {
    for (int16_t k = 0; k < 64; k++) { synthUR[j][k] = synth[currentPattern][j][k]; }
  }
  for (int16_t j = 0; j < 2; j++) {
    for (int16_t k = 0; k < 64; k++) { patternUR[j][k] = pattern[currentPattern][j][k]; }
  }

  swingSettingUR = swingSetting[currentPattern];


  vibratoRateMultiplierUR = vibratoRateMultiplier[currentPattern];


    tremRateMultiplierUR = tremRateMultiplier[currentPattern];



  swingSettingUR = swingSetting[currentPattern];



  vibratoPowerUR = vibratoPower[currentPattern];



  tremRateUR = tremRate[currentPattern];
  vibratoRateUR = vibratoRate[currentPattern];


  for (int16_t i = 0; i < 5; i++) {
    for (int16_t j = 0; j < 64; j++) {
      nudgeDelayUR[i][j] = nudgeDelay[currentPattern][i][j];
    }
  }

  for (int16_t i = 0; i < 32; i++) {

    keyChangeUR[i] = keyChange[currentPattern][i];
  }



  for (int16_t i = 0; i < 7; i++) {
    for (int16_t j = 0; j < 64; j++) {
      routeAutoUR[j][i] = routeAuto[currentPattern][j][i];
    }
  }

  for (int16_t j = 0; j < 64; j++) {
    stutterStepsUR[j] = stutterSteps[currentPattern][j];
    bendStepsUR[j] = bendSteps[currentPattern][j];
    freezeStepsUR[j] = freezeSteps[currentPattern][j];
  }

  //optimised to 4 bit nibbles
  for (int16_t j = 0; j < 32; j++) {

    wiggleAutoUR[j] = wiggleAuto[currentPattern][j];
    kitAutoUR[j] = kitAuto[currentPattern][j];
    vibAutoUR[j] = vibAuto[currentPattern][j];
    vibAutoWavUR[j] = vibAutoWav[currentPattern][j];
    tremAutoWavUR[j] = tremAutoWav[currentPattern][j];
    tremAutoUR[j] = tremAuto[currentPattern][j];
    swingAutoUR[j] = swingAuto[currentPattern][j];
    phonesAutoUR[j] = phonesAuto[currentPattern][j];
    mixAutoUR[j] = mixAuto[currentPattern][j];
  }
}




void undoRedo() {

  if (changeUndo) {
    if (undoPattern == currentPattern) {






      //when called we swap array[x] and arrayUR[x] (UndoRedo snapshot)

      uint8_t temp = 0;


      //phase 1 copy current to temp
      //phase 2 replace current with UR snapshot
      //phase 3 copy temp to UR snapShot


      for (int16_t i = 0; i < 500; i++) {
        temp = autoData[i];
        autoData[i] = autoDataUR[i];
        autoDataUR[i] = temp;
      }


      for (int16_t i = 0; i < 32; i++) {

        temp = keyChange[currentPattern][i];
        keyChange[currentPattern][i] = keyChangeUR[i];
        keyChangeUR[i] = temp;
      }


      for (int16_t i = 0; i < 5; i++) {
        for (int16_t j = 0; j < 64; j++) {
          temp = nudgeDelay[currentPattern][i][j];
          nudgeDelay[currentPattern][i][j] = nudgeDelayUR[i][j];
          nudgeDelayUR[i][j] = temp;
        }
      }


      for (int16_t i = 0; i < 10; i++) {

        temp = lastStepPoly[currentPattern][i];
        lastStepPoly[currentPattern][i] = lastStepUR[i];
        lastStepUR[i] = temp;

        temp = pages[currentPattern][i];
        pages[currentPattern][i] = pagesUR[i];
        pagesUR[i] = temp;

        for (int16_t j = 0; j < 152; j++) {
          temp = opl[i][j];
          opl[i][j] = oplUR[i][j];
          oplUR[i][j] = temp;
        }
      }

      for (int16_t j = 0; j < 152; j++) {
        temp = randomer[currentPattern][j];
        randomer[currentPattern][j] = randomerUR[j];
        randomerUR[j] = temp;
      }


      for (int16_t j = 0; j < 4; j++) {
        for (int16_t k = 0; k < 64; k++) {

          temp = synth[currentPattern][j][k];
          synth[currentPattern][j][k] = synthUR[j][k];
          synthUR[j][k] = temp;
        }
      }


      for (int16_t j = 0; j < 2; j++) {
        for (int16_t k = 0; k < 64; k++) {
          temp = pattern[currentPattern][j][k];
          pattern[currentPattern][j][k] = patternUR[j][k];
          patternUR[j][k] = temp;
        }
      }






      temp = vibratoRateMultiplier[currentPattern];
      vibratoRateMultiplier[currentPattern] = vibratoRateMultiplierUR;
      vibratoRateMultiplierUR = temp;

      temp = tremRateMultiplier[currentPattern];
      tremRateMultiplier[currentPattern] = tremRateMultiplierUR;
      tremRateMultiplierUR = temp;

      temp = swingSetting[currentPattern];
      swingSetting[currentPattern] = swingSettingUR;
      swingSettingUR = temp;


      temp = vibratoPower[currentPattern];
      vibratoPower[currentPattern] = vibratoPowerUR;
      vibratoPowerUR = temp;





      temp = tremRate[currentPattern];
      tremRate[currentPattern] = tremRateUR;
      tremRateUR = temp;

      temp = vibratoRate[currentPattern];
      vibratoRate[currentPattern] = vibratoRateUR;
      vibratoRateUR = temp;


      //optimised to nibbls
      for (int16_t j = 0; j < 32; j++) {
        temp = vibAuto[currentPattern][j];
        vibAuto[currentPattern][j] = vibAutoUR[j];
        vibAutoUR[j] = temp;

        temp = vibAutoWav[currentPattern][j];
        vibAutoWav[currentPattern][j] = vibAutoWavUR[j];
        vibAutoWavUR[j] = temp;

        temp = tremAuto[currentPattern][j];
        tremAuto[currentPattern][j] = tremAutoUR[j];
        tremAutoUR[j] = temp;

        temp = tremAutoWav[currentPattern][j];
        tremAutoWav[currentPattern][j] = tremAutoWavUR[j];
        tremAutoWavUR[j] = temp;

        temp = swingAuto[currentPattern][j];
        swingAuto[currentPattern][j] = swingAutoUR[j];
        swingAutoUR[j] = temp;

        temp = kitAuto[currentPattern][j];
        kitAuto[currentPattern][j] = kitAutoUR[j];
        kitAutoUR[j] = temp;

        temp = wiggleAuto[currentPattern][j];
        wiggleAuto[currentPattern][j] = wiggleAutoUR[j];
        wiggleAutoUR[j] = temp;


        temp = phonesAuto[currentPattern][j];
        phonesAuto[currentPattern][j] = phonesAutoUR[j];
        phonesAutoUR[j] = temp;


        temp = mixAuto[currentPattern][j];
        mixAuto[currentPattern][j] = mixAutoUR[j];
        mixAutoUR[j] = temp;
      }

      for (int16_t j = 0; j < 64; j++) {


        for (int16_t i = 0; i < 7; i++) {

          temp = routeAuto[currentPattern][j][i];
          routeAuto[currentPattern][j][i] = routeAutoUR[j][i];
          routeAutoUR[j][i] = temp;
        }

        temp = stutterSteps[currentPattern][j];
        stutterSteps[currentPattern][j] = stutterStepsUR[j];
        stutterStepsUR[j] = temp;


        temp = bendSteps[currentPattern][j];
        bendSteps[currentPattern][j] = bendStepsUR[j];
        bendStepsUR[j] = temp;

        temp = freezeSteps[currentPattern][j];
        freezeSteps[currentPattern][j] = freezeStepsUR[j];
        freezeStepsUR[j] = temp;
      }


      updateSwing(swingSetting[currentPattern]);

      showMode();
    }
  }
}
