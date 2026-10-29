int16_t faderToChannel(uint8_t number) {
  //returns the channel number of the given fader (ignores TOM CH OH CYM)
  switch (number) {
    case 36 ... 63: return (6); break;
    case 64 ... 91: return (7); break;
    case 92 ... 119: return (8); break;
    case 120 ... 147: return (9); break;
    case 1 ... 14: return (0); break;
    case 15 ... 21: return (1); break;
    default: return (-1); break;
  }
}

void clearTrack(uint8_t channel) {
  saveAuto = true;
  //clears all automation associated with the channel
  for (int16_t i = 0; i < autoThird; i++) {

    if ((faderToChannel(getFader(i)) == channel) && ((getPattern(i) == currentPattern))) {
      autoData[i] = 0;  //clear
    }
  }
}

uint8_t getVibAuto(uint8_t step) {
  if (step < 32) {
    return (vibAuto[currentPattern][step] & B00001111);
  } else {
    return (vibAuto[currentPattern][step - 32] >> 4);
  }
}
void setVibAuto(uint8_t step, uint8_t data) {
  saveAuto = true;
  if (step < 32) {
    for (int16_t i = 0; i < 4; i++) {
      bitWrite(vibAuto[currentPattern][step], i, bitRead(data, i));
    }
  } else {
    for (int16_t i = 4; i < 8; i++) {
      bitWrite(vibAuto[currentPattern][step - 32], i, bitRead(data, i - 4));
    }
  }
}

uint8_t getVibAutoWav(uint8_t step) {
  if (step < 32) {
    return (vibAutoWav[currentPattern][step] & B00001111);
  } else {
    return (vibAutoWav[currentPattern][step - 32] >> 4);
  }
}
void setVibAutoWav(uint8_t step, uint8_t data) {
  saveAuto = true;
  if (step < 32) {
    for (int16_t i = 0; i < 4; i++) {
      bitWrite(vibAutoWav[currentPattern][step], i, bitRead(data, i));
    }
  } else {
    for (int16_t i = 4; i < 8; i++) {
      bitWrite(vibAutoWav[currentPattern][step - 32], i, bitRead(data, i - 4));
    }
  }
}

uint8_t getTremAuto(uint8_t step) {
  if (step < 32) {
    return (tremAuto[currentPattern][step] & B00001111);
  } else {
    return (tremAuto[currentPattern][step - 32] >> 4);
  }
}
void setTremAuto(uint8_t step, uint8_t data) {
  saveAuto = true;
  if (step < 32) {
    for (int16_t i = 0; i < 4; i++) {
      bitWrite(tremAuto[currentPattern][step], i, bitRead(data, i));
    }
  } else {
    for (int16_t i = 4; i < 8; i++) {
      bitWrite(tremAuto[currentPattern][step - 32], i, bitRead(data, i - 4));
    }
  }
}

uint8_t getTremAutoWav(uint8_t step) {
  if (step < 32) {
    return (tremAutoWav[currentPattern][step] & B00001111);
  } else {
    return (tremAutoWav[currentPattern][step - 32] >> 4);
  }
}
void setTremAutoWav(uint8_t step, uint8_t data) {
  saveAuto = true;
  if (step < 32) {
    for (int16_t i = 0; i < 4; i++) {
      bitWrite(tremAutoWav[currentPattern][step], i, bitRead(data, i));
    }
  } else {
    for (int16_t i = 4; i < 8; i++) {
      bitWrite(tremAutoWav[currentPattern][step - 32], i, bitRead(data, i - 4));
    }
  }
}

uint8_t getSwingAuto(uint8_t step) {
  if (step < 32) {
    return (swingAuto[currentPattern][step] & B00001111);
  } else {
    return (swingAuto[currentPattern][step - 32] >> 4);
  }
}
void setSwingAuto(uint8_t step, uint8_t data) {
  saveAuto = true;
  if (step < 32) {
    for (int16_t i = 0; i < 4; i++) {
      bitWrite(swingAuto[currentPattern][step], i, bitRead(data, i));
    }
  } else {
    for (int16_t i = 4; i < 8; i++) {
      bitWrite(swingAuto[currentPattern][step - 32], i, bitRead(data, i - 4));
    }
  }
}

uint8_t getKitAuto(uint8_t step) {
  if (step < 32) {
    return (kitAuto[currentPattern][step] & B00001111);
  } else {
    return (kitAuto[currentPattern][step - 32] >> 4);
  }
}
void setKitAuto(uint8_t step, uint8_t data) {
  saveAuto = true;
  if (step < 32) {
    for (int16_t i = 0; i < 4; i++) {
      bitWrite(kitAuto[currentPattern][step], i, bitRead(data, i));
    }
  } else {
    for (int16_t i = 4; i < 8; i++) {
      bitWrite(kitAuto[currentPattern][step - 32], i, bitRead(data, i - 4));
    }
  }
}

uint8_t getWiggleAuto(uint8_t step) {
  if (step < 32) {
    return (wiggleAuto[currentPattern][step] & B00001111);
  } else {
    return (wiggleAuto[currentPattern][step - 32] >> 4);
  }
}
void setWiggleAuto(uint8_t step, uint8_t data) {
  saveAuto = true;
  if (step < 32) {
    for (int16_t i = 0; i < 4; i++) {
      bitWrite(wiggleAuto[currentPattern][step], i, bitRead(data, i));
    }
  } else {
    for (int16_t i = 4; i < 8; i++) {
      bitWrite(wiggleAuto[currentPattern][step - 32], i, bitRead(data, i - 4));
    }
  }
}

uint8_t getPhonesAuto(uint8_t step) {
  if (step < 32) {
    return (phonesAuto[currentPattern][step] & B00001111);
  } else {
    return (phonesAuto[currentPattern][step - 32] >> 4);
  }
}
void setPhonesAuto(uint8_t step, uint8_t data) {
  saveAuto = true;
  if (step < 32) {
    for (int16_t i = 0; i < 4; i++) {
      bitWrite(phonesAuto[currentPattern][step], i, bitRead(data, i));
    }
  } else {
    for (int16_t i = 4; i < 8; i++) {
      bitWrite(phonesAuto[currentPattern][step - 32], i, bitRead(data, i - 4));
    }
  }
}

uint8_t getMixAuto(uint8_t step) {
  if (step < 32) {
    return (mixAuto[currentPattern][step] & B00001111);
  } else {
    return (mixAuto[currentPattern][step - 32] >> 4);
  }
}
void setMixAuto(uint8_t step, uint8_t data) {
  saveAuto = true;
  if (step < 32) {
    for (int16_t i = 0; i < 4; i++) {
      bitWrite(mixAuto[currentPattern][step], i, bitRead(data, i));
    }
  } else {
    for (int16_t i = 4; i < 8; i++) {
      bitWrite(mixAuto[currentPattern][step - 32], i, bitRead(data, i - 4));
    }
  }
}


void vibTremSwingAuto() {
  //check for recorded keyChanges
  if (getKeyChange(polyStep[0])) {

    key = getKeyChange(polyStep[0]) - 1;
    if (keyChangeEdit) showMode();  //update piano leds (avoid stuck leds)
  }
  if ((mode == 0) && (heldTop) && (rec2)) {
    if (getKitAuto(polyStep[0]) != lastTop + 1) {
      setKitAuto(polyStep[0], lastTop + 1);
      saveAuto = true;
    }
  }

  bool routingChanged = false;
  for (int16_t i = 0; i < 7; i++) {
    if ((routeAuto[currentPattern][polyStep[0]][i]) && (routing[i] != routeAuto[currentPattern][polyStep[0]][i] - 1)) {
      routing[i] = routeAuto[currentPattern][polyStep[0]][i] - 1;
      routingChanged = true;
    }
  }

  if (routingChanged) {
    if ((mode == 2) && (function == 15)) showMode();  //update top LED
    updateRouting();
  }

  if ((getPhonesAuto(polyStep[0])) && (kit[currentPattern] != getPhonesAuto(polyStep[0]) - 1)) {
    phonesVol = getPhonesAuto(polyStep[0]) - 1;
    setPhonesVol();
    if ((mode == 2) && (function == 13)) showMode();  //update top LED
  }

  if ((getMixAuto(polyStep[0])) && (kit[currentPattern] != getMixAuto(polyStep[0]) - 1)) {
    mixVol = getMixAuto(polyStep[0]) - 1;
    setMixVol();
    if ((mode == 2) && (function == 14)) showMode();  //update top LED
  }

  if ((getKitAuto(polyStep[0])) && (kit[currentPattern] != getKitAuto(polyStep[0]) - 1)) {
    changeKit(getKitAuto(polyStep[0]) - 1);
  }

  //overWrite trem/vibrato rate if a top button is held.
  if ((mode == 2) && (heldTop)) {
    if (function == 13) {  //phones
      if ((rec) && (getPhonesAuto(polyStep[0])) && (getPhonesAuto(polyStep[0]) - 1 != lastTop)) {
        setPhonesAuto(polyStep[0], lastTop + 1);
      }
    }
    if (function == 14) {  //mix
      if ((rec) && (getMixAuto(polyStep[0])) && (getMixAuto(polyStep[0]) - 1 != lastTop)) {
        setMixAuto(polyStep[0], lastTop + 1);
      }
    }

    if (function == 3) {  //wiggle
      if ((rec) && (getWiggleAuto(polyStep[0])) && (getWiggleAuto(polyStep[0]) - 1 != lastTop)) {
        setWiggleAuto(polyStep[0], lastTop + 1);
      }
    }

    if (function == 4) {  //swing
      if ((rec) && (getSwingAuto(polyStep[0])) && (getSwingAuto(polyStep[0]) - 1 != lastTop)) {
        setSwingAuto(polyStep[0], lastTop + 1);
      }
    }


    else if (function == 11) {  //trem
      if (tremHeld) {
        if ((rec) && (getTremAutoWav(polyStep[0])) && (getTremAutoWav(polyStep[0]) - 1 != lastTop)) {
          setTremAutoWav(polyStep[0], lastTop + 1);
        }
      } else {
        if ((rec) && (getTremAuto(polyStep[0])) && (getTremAuto(polyStep[0]) - 1 != lastTop)) {
          setTremAuto(polyStep[0], lastTop + 1);
        }
      }
    }

    else if (function == 12) {  //vibrato
      if (vibHeld) {
        if ((rec) && (getVibAutoWav(polyStep[0])) && (getVibAutoWav(polyStep[0]) - 1 != lastTop)) {
          setVibAutoWav(polyStep[0], lastTop + 1);
        }
      } else {
        if ((rec) && (getVibAuto(polyStep[0])) && (getVibAuto(polyStep[0]) - 1 != lastTop)) {
          setVibAuto(polyStep[0], lastTop + 1);
        }
      }
    }
  }


  if (getWiggleAuto(polyStep[0])) {
    //there is something here! is it dirrent from the current rate?
    if (getWiggleAuto(polyStep[0]) - 1 != randomDepth[currentPattern]) {
      randomDepth[currentPattern] = getWiggleAuto(polyStep[0]) - 1;
      if ((mode == 2) && (function == 3)) showMode();  //update top LED
    }
  }

  if (getSwingAuto(polyStep[0])) {
    //there is something here! is it dirrent from the current rate?
    if (getSwingAuto(polyStep[0]) - 1 != swingSetting[currentPattern]) {
      swingSetting[currentPattern] = getSwingAuto(polyStep[0]) - 1;
      updateSwing(swingSetting[currentPattern]);
      if ((mode == 2) && (function == 4)) showMode();  //update top LED
    }
  }

  if (getTremAuto(polyStep[0])) {
    //there is something here! is it dirrent from the current rate?
    if (getTremAuto(polyStep[0]) - 1 != tremRate[currentPattern]) {
      tremRate[currentPattern] = getTremAuto(polyStep[0]) - 1;
      newTremRate = true;
      if ((mode == 2) && (function == 11)) showMode();  //update top LED
    }
  }

  if (getVibAuto(polyStep[0])) {
    //there is something here! is it dirrent from the current rate?
    if (getVibAuto(polyStep[0]) - 1 != vibratoRate[currentPattern]) {
      vibratoRate[currentPattern] = getVibAuto(polyStep[0]) - 1;
      newVibRate = true;
      if ((mode == 2) && (function == 12)) showMode();  //update top LED
    }
  }


  if (getVibAutoWav(polyStep[0])) {
    //there is something here!
    vibWav = getVibAutoWav(polyStep[0]) - 1;
    if ((mode == 2) && (function == 12)) showMode();  //update top LED
  }

  if (getTremAutoWav(polyStep[0])) {
    //there is something here!
    tremWav = getTremAutoWav(polyStep[0]) - 1;
    if ((mode == 2) && (function == 11)) showMode();  //update top LED
  }

  //reset trem/vib counters and indexes if the speed changed.

  if (newTremRate) {
    newTremRate = false;
    tremIndex = 0;
  }

  if (newVibRate) {
    newVibRate = false;
    vIndex = 0;
  }
}



void writeAutoAtStep(int16_t pat, int16_t slot, uint8_t fader, uint8_t data, uint8_t step, uint8_t page) {

  saveAuto = 1;

  uint8_t saveStep = step;
  while (saveStep > 15) {
    saveStep -= 16;
  }

  data = constrain(data, 0, 15);

  setFader(slot, fader);                                           //write fader
  autoData[slot + autoThird] = (byte)((((page)) << 4) | data);     //page and data
  autoData[slot + autoTwoThirds] = (byte)((pat << 4) | saveStep);  //pattern and step
}

void writeAuto(int16_t slot, uint8_t fader, uint8_t data) {

  saveAuto = 1;

  uint8_t saveStep = polyStep[0];
  while (saveStep > 15) {
    saveStep -= 16;
  }

  data = constrain(data, 0, 15);

  setFader(slot, fader);                                                      //write fader
  autoData[slot + autoThird] = (byte)((((polyStep[0] >> 4)) << 4) | data);     //page and data
  autoData[slot + autoTwoThirds] = (byte)((currentPattern << 4) | saveStep);  //pattern and step
}

void clearFader(uint8_t fader) {
  saveAuto = true;
  for (int16_t i = 0; i < autoThird; i++) {

    if ((getFader(i) == fader) && ((getPattern(i) == currentPattern))) {
      autoData[i] = 0;  //clear
    }
  }
}

void clearAutoPAttern() {
  clearNudges();
  saveAuto = true;
  for (int16_t i = 0; i < 64; i++) {
    bendSteps[currentPattern][i] = 0;
    freezeSteps[currentPattern][i] = 0;
    stutterSteps[currentPattern][i] = 0;
  }

  //optimised to nibles
  for (int16_t i = 0; i < 32; i++) {
    vibAuto[currentPattern][i] = 0;
    vibAutoWav[currentPattern][i] = 0;
    tremAutoWav[currentPattern][i] = 0;
    tremAuto[currentPattern][i] = 0;
    kitAuto[currentPattern][i] = 0;
    phonesAuto[currentPattern][i] = 0;
    mixAuto[currentPattern][i] = 0;
    swingAuto[currentPattern][i] = 0;
    wiggleAuto[currentPattern][i] = 0;
  }
  for (int16_t i = 0; i < 64; i++) {
    for (int16_t j = 0; j < 7; j++) {
      routeAuto[currentPattern][i][j] = 0;
    }
  }
  for (int16_t i = 0; i < autoThird; i++) {
    if (getPattern(i) == currentPattern) autoData[i] = 0;  //clear
  }
  loadPattern(currentPattern);
}


void readAuto() {

  uint8_t autoStep;
  if (stutter && !drill) {
    autoStep = stutterStepPoly[0];
  } else {
    autoStep = polyStep[0];
  }

  for (int16_t i = 0; i < autoThird; i++) {
    //any enabled slots?
    if (getEnabled(i)) {
      //Yes, is it the current page?
      if (getPage(i) == (autoStep >> 4)) {

        //Yes, is it the current step?

        uint8_t temp = autoStep;
        while (temp > 15) {
          temp -= 16;
        }
        if (getAutoStep(i) == temp) {

          //Yes, is it the current pattern?
          if (getPattern(i) == currentPattern) {


            //apply automation

            if ((getFader(i) >= 36) && (getFader(i) <= 63)) {
              if (!mute[6]) {
                opl[currentKit][getFader(i)] = getData(i);
              }  //apply synth  automation only if not muted
            }

            else if ((getFader(i) >= 64) && (getFader(i) <= 91)) {
              if (!mute[7]) {
                opl[currentKit][getFader(i)] = getData(i);
              }  //apply synth automation only if not muted
            }

            else if ((getFader(i) >= 92) && (getFader(i) <= 119)) {
              if (!mute[8]) {
                opl[currentKit][getFader(i)] = getData(i);
              }  //apply synth automation only if not muted
            }


            else if ((getFader(i) >= 120) && (getFader(i) <= 147)) {
              if (!mute[9]) {
                opl[currentKit][getFader(i)] = getData(i);
              }  //apply synth automation only if not muted
            }

            else if ((getFader(i) < 36) || (getFader(i) > 147)) {
              opl[currentKit][getFader(i)] = getData(i);
            }


            if (!stutter) sendFaderCC(getFader(i), getData(i));
          }
        }
      }
    }
  }
}


uint8_t getPage(int16_t slot) {
  return ((byte)((autoData[slot + autoThird] >> 4) & (byte)0x0F));
}

uint8_t getData(int16_t slot) {
  return ((byte)(autoData[slot + autoThird] & 0x0F));
}

uint8_t getPattern(int16_t slot) {
  return (autoData[slot + autoTwoThirds] >> 4);
}


uint8_t getAutoStep(int16_t slot) {
  return ((byte)(autoData[slot + autoTwoThirds] & 0x0F));
}

bool getEnabled(int16_t slot) {
  if (autoData[slot]) {
    return (1);
  } else {
    return (0);
  }
}

uint8_t getFader(int16_t slot) {
  return (autoData[slot]);
}

void setFader(int16_t slot, uint8_t fader) {
  autoData[slot] = fader;
}

void aStore(uint8_t fader, uint8_t data, uint8_t step) {
  saveAuto = true;
  uint8_t temp = step;
  while (temp > 15) {
    temp -= 16;
  }
  bool done = false;
  //look for same fader at same step & page to overwrite
  for (int16_t i = 0; i < autoThird; i++) {
    if ((getFader(i) == fader) && (getAutoStep(i) == temp) && (getPage(i) == polyStep[0] >> 4) && (getPattern(i) == currentPattern)) {
      //overwrite
      writeAuto(i, fader, data);
      done = true;
      i = autoThird;
    }
  }


  //look for empty slot
  if (!done) {
    for (int16_t i = 0; i < autoThird; i++) {
      if (!getEnabled(i)) {
        writeAuto(i, fader, data);
        done = true;
        i = autoThird;
      }
    }
  }

  //no empty slot, overwrite at random
  if (!done) {
    writeAuto(random(autoThird), fader, data);
  }
}


void aStorePloc(uint8_t pat, uint8_t fader, uint8_t data, uint8_t step, uint8_t page) {
  saveAuto = true;

  bool done = false;

  //look for same fader at same step & page to overwrite
  for (int16_t i = 0; i < autoThird; i++) {
    if ((getFader(i) == fader) && (getAutoStep(i) == step) && (getPage(i) == page) && (getPattern(i) == pat)) {
      //overwrite
      writeAutoAtStep(pat, i, fader, data, step, page);
      done = true;
      i = autoThird;
    }
  }


  //look for empty slot
  if (!done) {
    for (int16_t i = 0; i < autoThird; i++) {
      if (!getEnabled(i)) {
        writeAutoAtStep(pat, i, fader, data, step, page);
        done = true;
        i = autoThird;
      }
    }
  }

  //no empty slot, overwrite at random
  if (!done) {
    writeAutoAtStep(pat, random(autoThird), fader, data, step, page);
  }
}

void clonePlocStep(uint8_t destination) {
  for (int16_t i = 0; i < autoThird; i++) {
    //is there automation to copy from the pick step
    if ((getEnabled(i)) && (getAutoStep(i) == pickStepNumber) && (getPattern(i) == currentPattern) && (getPage(i) == selectedPage)) {
      aStorePloc(currentPattern, getFader(i), getData(i), destination, selectedPage);
    }
  }
}

void clonePlocPage(uint8_t sourcePage, uint8_t destinationPage, uint8_t sourcePattern, uint8_t destinationPattern) {
  int plocCounter = 0;

  for (int16_t i = 0; i < autoThird; i++) {
    //is there automation to copy from the pick step
    if (getEnabled(i) && getPattern(i) == sourcePattern && getPage(i) == sourcePage) {
      aStorePloc(destinationPattern, getFader(i), getData(i), getAutoStep(i), destinationPage);
      plocCounter++;
    }
  }
}
