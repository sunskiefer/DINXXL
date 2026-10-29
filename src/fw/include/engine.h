const int16_t pitchFalls[] = { -20, -13, -9, -6, -4, -2, -1, 0, 0, 1, 2, 4, 6, 9, 13, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//0   1   2   3   4   5   6   (7   8)   9   10   11   12   13   14   15


void loadRoutingPreset(int16_t preset) {
  EEPROMupdate(110, preset);
  routePresetLoad();

  for (int i = 0; i < 7; i++) {
    routing[i] = routingPreset[preset][i];
  }
  updateRouting();

  EEPROMupdate(3, routing[0]);
  EEPROMupdate(4, routing[1]);
  EEPROMupdate(5, routing[2]);
  EEPROMupdate(6, routing[3]);
  EEPROMupdate(7, routing[4]);
  EEPROMupdate(18, routing[5]);
  EEPROMupdate(19, routing[6]);

  wipe();
}

void saveRoutingPreset(int16_t preset) {

  for (int i = 0; i < 7; i++) {
    routingPreset[preset][i] = routing[i];
  }
  routePresetStore();

  wipe();
}


const uint8_t shuffleThresh = 95;

void updateCurrentKit() {
  //adds the bank offset
  currentKit = kit[currentPattern] + (kitVariation[currentPattern][kit[currentPattern]] * 10);
}

void shuffleKit() {

  for (int16_t i = 0; i < 152; i++) {
    if (random(100) > shuffleThresh) {
      opl[currentKit][i] += random(3) - random(3);
    }
  }
}

void shufflePattern() {

  for (int16_t i = 0; i < 64; i++) {
    for (int16_t j = 0; j < 4; j++) {

      //flip drum bits
      if (random(100) > shuffleThresh) {
        pattern[currentPattern][0][i] = !pattern[currentPattern][0][i];
      }
      if (random(100) > shuffleThresh) {
        pattern[currentPattern][1][i] = !pattern[currentPattern][1][i];
      }

      //flip synth step
      if (getStep(j, i)) {
        if (random(100) > shuffleThresh) {
          setStep(j, i, 0);
        }
      } else {
        if (random(100) > shuffleThresh) {
          setStep(j, i, 1);
        }
      }
      //change synth pitch
      if (random(100) > shuffleThresh) {
        setNote(j, i, getNote(j, i) + random(3) - random(3));
      }
    }
  }
}


void volTick() {
  int16_t tempVol;

  for (volAlternator = 0; volAlternator < 22; volAlternator++) {


    if (volLast[volAlternator] != volNext[volAlternator]) {

      if (volNext[volAlternator] > volLast[volAlternator]) {
        volLast[volAlternator]++;
      } else {
        volLast[volAlternator]--;
      }

      tempVol = volLast[volAlternator];

      switch (volAlternator) {

        case 0:
          opl2.setVolume(0, OPERATOR1, tempVol);
          break;  //A OP1

        case 1:
          opl2.setVolume(0, OPERATOR2, tempVol);
          break;  // A OP2

        case 2:
          opl2.setVolume(3, OPERATOR1, tempVol);
          break;  // A OP3

        case 3:
          opl2.setVolume(3, OPERATOR2, tempVol);
          break;  // A OP4

          //// /// / / / / / / / // / //////// /

        case 4:
          opl2.setVolume(1, OPERATOR1, tempVol);
          break;  //B OP1

        case 5:
          opl2.setVolume(1, OPERATOR2, tempVol);
          break;  // B OP2

        case 6:
          opl2.setVolume(4, OPERATOR1, tempVol);
          break;  // B OP3

        case 7:
          opl2.setVolume(4, OPERATOR2, tempVol);
          break;  // B OP4


        //// /// / / / / / / / // / //////// /
        case 8:
          opl2.setVolume(2, OPERATOR1, tempVol);
          opl2.setVolume(10, OPERATOR1, tempVol);
          break;  //A OP1

        case 9:
          opl2.setVolume(2, OPERATOR2, tempVol);
          opl2.setVolume(10, OPERATOR2, tempVol);
          break;  // A OP2

        case 10:
          opl2.setVolume(5, OPERATOR1, tempVol);
          opl2.setVolume(13, OPERATOR1, tempVol);
          break;  // A OP3

        case 11:
          opl2.setVolume(5, OPERATOR2, tempVol);
          opl2.setVolume(13, OPERATOR2, tempVol);
          break;  // A OP4

          //// /// / / / / / / / // / //////// /

        case 12:
          opl2.setVolume(9, OPERATOR1, tempVol);
          opl2.setVolume(11, OPERATOR1, tempVol);
          break;  //B OP1

        case 13:
          opl2.setVolume(9, OPERATOR2, tempVol);
          opl2.setVolume(11, OPERATOR2, tempVol);
          break;  // B OP2

        case 14:
          opl2.setVolume(12, OPERATOR1, tempVol);
          opl2.setVolume(14, OPERATOR1, tempVol);
          break;  // B OP3

        case 15:
          opl2.setVolume(12, OPERATOR2, tempVol);
          opl2.setVolume(14, OPERATOR2, tempVol);
          break;  // B OP4

        case 16:
          opl2.setVolume(6, OPERATOR2, tempVol);
          break;

        case 17: opl2.setVolume(6, OPERATOR1, tempVol); break;  //bd mod

        case 18: opl2.setVolume(7, OPERATOR2, tempVol); break;  //sd

        case 19: opl2.setVolume(8, OPERATOR2, tempVol); break;  //crash

        case 20: opl2.setVolume(8, OPERATOR1, tempVol); break;  //tom

        case 21: opl2.setVolume(7, OPERATOR1, tempVol); break;  //hats
      }
    }
  }
}

void setMetronome(bool on) {

  if (!on) {
    volLast[21] = 255;  //should refresh hhat volume
  }
}




void engineTick() {



  for (int i = 0; i < 15; i++) {

    if (newBend[i]) {

      newBend[i] = false;
      switch (i) {
        //percussive
        case 0: opl2.setFNumber(6, fNumberBend[0]); break;
        case 1: opl2.setFNumber(7, fNumberBend[1]); break;
        case 2: opl2.setFNumber(8, fNumberBend[2]); break;
        //synth
        case 3: opl2.setFNumber(0, fNumberBend[3]); break;
        case 4: opl2.setFNumber(3, fNumberBend[4]); break;
        case 5: opl2.setFNumber(1, fNumberBend[5]); break;
        case 6: opl2.setFNumber(4, fNumberBend[6]); break;
        case 7: opl2.setFNumber(2, fNumberBend[7]); break;
        case 8: opl2.setFNumber(5, fNumberBend[8]); break;
        case 9: opl2.setFNumber(9, fNumberBend[9]); break;
        case 10: opl2.setFNumber(12, fNumberBend[10]); break;

        case 11: opl2.setFNumber(10, fNumberBend[11]); break;
        case 12: opl2.setFNumber(13, fNumberBend[12]); break;
        case 13: opl2.setFNumber(11, fNumberBend[13]); break;
        case 14: opl2.setFNumber(14, fNumberBend[14]); break;
      }
    }
  }



  pitchFallCounter++;
  if (pitchFallCounter >= pitchFallRate) {
    pitchFallCounter = 0;
    for (int16_t i = 0; i < 5; i++) {

      pitchNow[i] += pitchFalls[pitchFall[i]];
      if (pitchNow[i] < 90) pitchNow[i] = 90;
      if (pitchNow[i] > 4023) pitchNow[i] = 4023;
    }
  }
}

void oplUpdate() {
  uint8_t offset = 0;
  int16_t temp;
  int16_t pendingSnareMult = -1;
  int16_t pendingHatMult = -1;
  int16_t pendingCymOp2Mult = -1;

  if (!freeze) {
    for (int16_t i = 0; i < 152; i++) {

      if (resetRegisters || (opl[currentKit][i] != oplLast[i]) || (randomer[currentPattern][i] && !wiggledOnThisStep[i])) {

        wiggledOnThisStep[i] = 1;  //set high if we've wiggled this register on this step (only wiggle once per step)


        switch (i) {

          case 0:


            if (metroStep) {

              //open hat metronome on the first beat of a page
              if (polyStep[0] % 16 == 0 && polyStep[0] <= 48) {
                opl2.setVolume(7, OPERATOR1, 4);
                opl2.setRelease(7, OPERATOR1, 13);
              } else {
                opl2.setVolume(7, OPERATOR1, 16);
                opl2.setRelease(7, OPERATOR1, 14);
              }
            }


            opl2.setDrums(bitRead(opl[currentKit][i], 4), bitRead(opl[currentKit][i], 3), bitRead(opl[currentKit][i], 2), bitRead(opl[currentKit][i], 1), bitRead(opl[currentKit][i], 0));

            //clear all bits to avoid double triggers on nudge
            opl[currentKit][i] &= 0x60;  // 01100000b
            break;                       //drum trigger
            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            ///////////////////////////      KICK CARRIER     ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

          case 1: volNext[16] = getRandy(i) + 63 - ((getRandy(i) + opl[currentKit][i]) << 2); break;  //kick vol1
          case 2: opl2.setWaveForm(6, OPERATOR2, ((getRandy(i) + opl[currentKit][i]) >> 1)); break;   //wav
          case 3: opl2.setFeedback(6, getRandy(i) + ((opl[currentKit][i]) >> 1)); break;              //feedback
          case 4:
            temp = getRandy(i) + opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) {
              temp = map(temp, 0, 15, 1, 15);
            }
            opl2.setMultiplier(6, OPERATOR2, temp);
            break;                                                                           //mult
          case 5: pitchRoot[0] = ((getRandy(i) + opl[currentKit][i]) << 6); break;           //tune
          case 6: pitchFall[0] = constrain(getRandy(i) + opl[currentKit][i], 0, 15); break;  //fall
          case 7:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 4, 9);
            opl2.setAttack(6, OPERATOR2, temp);
            break;  //att
          case 8:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 4, 9);
            opl2.setRelease(6, OPERATOR2, temp);
            break;  //rel

            ////////////////////////////////      KICK MODULATOR     //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

          case 9: volNext[17] = 63 - ((opl[currentKit][i]) << 2); break;  //vol
          case 10:
            temp = (getRandy(i) + opl[currentKit][i]) >> 1;
            opl2.setWaveForm(6, OPERATOR1, temp);
            break;  //wav

          case 11:
            temp = ((getRandy(i) + opl[currentKit][i]));
            temp = constrain(temp, 0, 15);
            pitchRoot[1] = temp << 6;

            break;                                                                            //mult
          case 12: pitchFall[1] = constrain(getRandy(i) + opl[currentKit][i], 0, 15); break;  //fall
          case 13:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 4, 9);
            opl2.setAttack(6, OPERATOR1, temp);
            break;  //att
          case 14:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 4, 9);
            opl2.setRelease(6, OPERATOR1, temp);
            break;  //rel

            /////////////////////////////////      SNARE     ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

          case 15: volNext[18] = 63 - ((getRandy(i) + opl[currentKit][i]) << 2); break;  //vol
          case 16:
            temp = (getRandy(i) + opl[currentKit][i]) >> 1;
            temp = constrain(temp, 0, 7);
            if ((temp == 3) || (temp == 4) || (temp == 5)) {
              temp = 2;
              opl2.setWaveForm(7, OPERATOR2, 2);
            } else {
              opl2.setWaveForm(7, OPERATOR2, temp);
            }
          case 19:
            temp = ((getRandy(i) + opl[currentKit][i]));
            temp = constrain(temp, 1, 15);
            pendingSnareMult = temp;
            break;                                                                            //mult
          case 17: pitchRoot[2] = ((getRandy(i) + opl[currentKit][i]) << 6); break;           //tune
          case 18: pitchFall[2] = constrain(getRandy(i) + opl[currentKit][i], 0, 15); break;  //fall
          case 20:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 4, 9);
            opl2.setAttack(7, OPERATOR2, temp);
            break;  //att
          case 21:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 6, 10);
            opl2.setRelease(7, OPERATOR2, temp);
            break;  //rel

          //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


          /// HATS +cymbal ////
          case 24:
            temp = 63 - ((getRandy(i) + opl[currentKit][i]) << 2);
            volNext[21] = temp;
            break;  //VOL
          case 26:
            temp = (getRandy(i) + opl[currentKit][i]) >> 1;
            if (temp < 7) {
              opl2.setWaveForm(7, OPERATOR1, temp);
              opl2.setWaveForm(8, OPERATOR2, temp);
            }
            break;  //WAV  (ALSO CRASH)
          case 148:
            temp = getRandy(i) + opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            pendingHatMult = temp;
            pendingCymOp2Mult = 15 - temp;
            break;                                                                             //MULT (ALSO CRASH)
          case 149: pitchRoot[4] = ((getRandy(i) + opl[currentKit][i]) << 6); break;           //TUNE  (ALSO CRASH)
          case 150: pitchFall[4] = constrain(getRandy(i) + opl[currentKit][i], 0, 15); break;  //FALL  (ALSO CRASH)
          case 32:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 4, 9);
            opl2.setAttack(7, OPERATOR1, temp);
            break;  //Attack
          case 35:
            hatDecay = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) {
              hatDecay = map(hatDecay, 0, 15, 6, 12);
            }
            hatDecayOpen = (hatDecay >> 1) + 2;
            break;  //DECAY

          //cymbal only
          case 22: volNext[19] = 63 - ((getRandy(i) + opl[currentKit][i]) << 2); break;
          case 30:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 4, 9);
            opl2.setAttack(8, OPERATOR2, temp);
            break;
          case 33:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 6, 10);
            opl2.setRelease(8, OPERATOR2, temp);
            break;

          //TOMS ///
          case 23: volNext[20] = 63 - ((getRandy(i) + opl[currentKit][i]) << 2); break;  //VOL
          case 25:
            temp = (getRandy(i) + opl[currentKit][i]) >> 1;
            if (temp < 7) opl2.setWaveForm(8, OPERATOR1, temp);
            break;  //WAV
          case 29:
            temp = getRandy(i) + opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            opl2.setMultiplier(8, OPERATOR1, temp);
            break;                                                                                   //MULT
          case 27: pitchRoot[3] = (getRandy(i) + opl[currentKit][i]) << 6; break;                    //TUNE
          case 28: pitchFall[3] = temp = constrain(getRandy(i) + opl[currentKit][i], 0, 15); break;  //FALL
          case 31:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 4, 9);
            opl2.setAttack(8, OPERATOR1, temp);
            break;  //ATTACK
          case 34:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 6, 10);
            opl2.setRelease(8, OPERATOR1, temp);
            break;  //DECAY

          //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          /// second row synth1-2
          //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          case 64: offset = 4;
          case 36:
            updateVol(offset);

            break;  // vol
          case 65: offset = 1;
          case 37: opl2.setWaveForm(offset, OPERATOR1, (getRandy(i) + opl[currentKit][i]) >> 1); break;  //wav
          case 66: offset = 1;
          case 38: opl2.setFeedback(offset, (getRandy(i) + opl[currentKit][i]) >> 1); break;  //feed
          case 67: offset = 1;
          case 39: setAlgo(offset, getRandy(i) + map(opl[currentKit][i], 0, 15, 0, 4)); break;  //algo
          case 68: offset = 1;
          case 40: opl2.setMultiplier(offset, OPERATOR1, (getRandy(i) + opl[currentKit][i])); break;  //mult
          case 69: offset = 1;
          case 41:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setAttack(offset, OPERATOR1, temp);
            break;  //att
          case 70: offset = 1;
          case 42:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setRelease(offset, OPERATOR1, temp);
            break;  //rel
          /////////////////////// .                   /////////////////////////
          case 71: offset = 4;
          case 43:
            updateVol(offset + 1);

            break;  // vol
          case 72: offset = 1;
          case 44: opl2.setWaveForm(offset, OPERATOR2, (getRandy(i) + opl[currentKit][i]) >> 1); break;  //wav
          case 73: offset = 1;
          case 45: opl2.setMultiplier(offset, OPERATOR2, (getRandy(i) + opl[currentKit][i])); break;  //mult
          case 74: offset = 2;
          case 46:
            tremDepth[offset] = getRandy(i) + opl[currentKit][i];
            tremDepth[offset] = constrain(tremDepth[offset], 0, 15);
            tremDepth[offset] /= 15;
            break;  //trem
          case 75: offset = 2;
          case 47:
            vibratoDepth[offset] = getRandy(i) + opl[currentKit][i];
            vibratoDepth[offset] = constrain(vibratoDepth[offset], 0, 15);
            vibratoDepth[offset] /= 15;
            break;  //vib
          case 76: offset = 1;
          case 48:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setAttack(offset, OPERATOR2, temp);
            break;  //att
          case 77: offset = 1;
          case 49:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setRelease(offset, OPERATOR2, temp);
            break;  //rel
          /////////////////////// .                   /////////////////////////
          case 78: offset = 4;
          case 50:
            updateVol(offset + 2);

            break;  // vol
          case 79: offset = 1;
          case 51: opl2.setWaveForm(3 + offset, OPERATOR1, (getRandy(i) + opl[currentKit][i]) >> 1); break;  //wav
          case 80: offset = 1;
          case 52: opl2.setFeedback(3 + offset, (getRandy(i) + opl[currentKit][i]) >> 1); break;  ////feed
          case 81: offset = 1;
          case 53:
            tuneBase[offset] = getRandy(i) + opl[currentKit][i];
            tuneBase[offset] /= 15;
            tuneBase[offset] += .5;
            if (offset) {
              freqNext[3] = -1;
            } else {
              freqNext[1] = -1;
            }
            break;  //tune
          case 82: offset = 1;
          case 54: opl2.setMultiplier(3 + offset, OPERATOR1, (getRandy(i) + opl[currentKit][i])); break;  //mult
          case 83: offset = 1;
          case 55:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setAttack(3 + offset, OPERATOR1, temp);
            break;  //att
          case 84: offset = 1;
          case 56:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setRelease(3 + offset, OPERATOR1, temp);
            break;  //rel
          /////////////////////// .                   /////////////////////////
          case 85: offset = 4;
          case 57:
            updateVol(12 + offset + 3);

            break;  // vol
          case 86: offset = 1;
          case 58: opl2.setWaveForm(3 + offset, OPERATOR2, (getRandy(i) + opl[currentKit][i]) >> 1); break;  //wav
          case 87: offset = 1;
          case 59: multBase[offset] = (getRandy(i) + opl[currentKit][i]); break;  //mult
          case 88: offset = 2;
          case 60:
            tremDepth[offset + 1] = getRandy(i) + opl[currentKit][i];
            tremDepth[offset + 1] = constrain(tremDepth[offset + 1], 0, 15);
            tremDepth[offset + 1] /= 15;
            break;  //trem
          case 89: offset = 2;
          case 61:
            vibratoDepth[offset + 1] = getRandy(i) + opl[currentKit][i];
            vibratoDepth[offset + 1] = constrain(vibratoDepth[offset + 1], 0, 15);
            vibratoDepth[offset + 1] /= 15;
            break;  //vibra
          case 90: offset = 1;
          case 62:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setAttack(3 + offset, OPERATOR2, temp);
            break;  //att
          case 91: offset = 1;
          case 63:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setRelease(3 + offset, OPERATOR2, temp);
            break;  //rel
          //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          /// second row synth3
          //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          case 92:
            updateVol(8);

            break;  // vol
          case 93:
            opl2.setWaveForm(2, OPERATOR1, (getRandy(i) + opl[currentKit][i]) >> 1);
            opl2.setWaveForm(10, OPERATOR1, (getRandy(i) + opl[currentKit][i]) >> 1);
            break;  //wav
          case 94:
            opl2.setFeedback(2, (getRandy(i) + opl[currentKit][i]) >> 1);
            opl2.setFeedback(10, (getRandy(i) + opl[currentKit][i]) >> 1);
            break;  //feed
          case 95:
            setAlgo(2, getRandy(i) + map(opl[currentKit][i], 0, 15, 0, 4));
            setAlgo(10, getRandy(i) + map(opl[currentKit][i], 0, 15, 0, 4));
            break;  //algo
          case 96:
            opl2.setMultiplier(2, OPERATOR1, (getRandy(i) + opl[currentKit][i]));
            opl2.setMultiplier(10, OPERATOR1, (getRandy(i) + opl[currentKit][i]));
            break;  //mult
          case 97:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setAttack(2, OPERATOR1, temp);
            opl2.setAttack(10, OPERATOR1, temp);
            break;  //att
          case 98:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setRelease(2, OPERATOR1, temp);
            opl2.setRelease(10, OPERATOR1, temp);
            break;  //rel
          /////////////////////// .                   /////////////////////////
          case 99:
            updateVol(9);

            break;  // vol
          case 100:
            opl2.setWaveForm(2, OPERATOR2, (getRandy(i) + opl[currentKit][i]) >> 1);
            opl2.setWaveForm(10, OPERATOR2, (getRandy(i) + opl[currentKit][i]) >> 1);
            break;  //wav
          case 101:
            opl2.setMultiplier(2, OPERATOR2, (getRandy(i) + opl[currentKit][i]));
            opl2.setMultiplier(10, OPERATOR2, (getRandy(i) + opl[currentKit][i]));
            break;  //mult
          case 102:
            tremDepth[4] = getRandy(i) + opl[currentKit][i];
            tremDepth[4] = constrain(tremDepth[4], 0, 15);
            tremDepth[4] /= 15;
            break;  //trem
          case 103:
            vibratoDepth[4] = getRandy(i) + opl[currentKit][i];
            vibratoDepth[4] = constrain(vibratoDepth[4], 0, 15);
            vibratoDepth[4] /= 15;
            break;  //vib
          case 104:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setAttack(2, OPERATOR2, temp);
            opl2.setAttack(10, OPERATOR2, temp);
            break;  //att
          case 105:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setRelease(2, OPERATOR2, temp);
            opl2.setRelease(10, OPERATOR2, temp);
            break;  //rel
          //// .                   /////////////////////////
          case 106:
            updateVol(10);

            break;  // vol
          case 107:
            opl2.setWaveForm(5, OPERATOR1, (getRandy(i) + opl[currentKit][i]) >> 1);
            opl2.setWaveForm(13, OPERATOR1, (getRandy(i) + opl[currentKit][i]) >> 1);
            break;  //wav
          case 108:
            opl2.setFeedback(5, (getRandy(i) + opl[currentKit][i]) >> 1);
            opl2.setFeedback(13, (getRandy(i) + opl[currentKit][i]) >> 1);
            break;  ////feed
          case 109:
            tuneBase[2] = getRandy(i) + opl[currentKit][i];
            tuneBase[2] /= 15;
            tuneBase[2] += .5;
            freqLast[4] = -1;
            freqLast[5] = -1;
            freqLast[8] = -1;
            freqLast[9] = -1;
            break;  //tune
          case 110:
            opl2.setMultiplier(5, OPERATOR1, (getRandy(i) + opl[currentKit][i]));
            opl2.setMultiplier(13, OPERATOR1, (getRandy(i) + opl[currentKit][i]));
            break;  //mult
          case 111:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setAttack(5, OPERATOR1, temp);
            opl2.setAttack(13, OPERATOR1, temp);
            break;  //att
          case 112:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setRelease(5, OPERATOR1, temp);
            opl2.setRelease(13, OPERATOR1, temp);
            break;  //rel
          //// .         /////////////////////////
          case 113:
            updateVol(11);

            break;  // vol
          case 114:
            opl2.setWaveForm(5, OPERATOR2, (getRandy(i) + opl[currentKit][i]) >> 1);
            opl2.setWaveForm(13, OPERATOR2, (getRandy(i) + opl[currentKit][i]) >> 1);
            break;                                                            //wav
          case 115: multBase[2] = (getRandy(i) + opl[currentKit][i]); break;  //mult
          case 116:
            tremDepth[5] = getRandy(i) + opl[currentKit][i];
            tremDepth[5] = constrain(tremDepth[5], 0, 15);
            tremDepth[5] /= 15;
            break;  //trem
          case 117:
            vibratoDepth[5] = getRandy(i) + opl[currentKit][i];
            vibratoDepth[5] = constrain(vibratoDepth[5], 0, 15);
            vibratoDepth[5] /= 15;
            break;  //vibra
          case 118:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setAttack(5, OPERATOR2, temp);
            opl2.setAttack(13, OPERATOR2, temp);
            break;  //att
          case 119:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setRelease(5, OPERATOR2, temp);
            opl2.setRelease(13, OPERATOR2, temp);
            break;  //rel
          //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          /// second row synth4
          //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          case 120:
            updateVol(12);

            break;  // vol
          case 121:
            opl2.setWaveForm(9, OPERATOR1, (getRandy(i) + opl[currentKit][i]) >> 1);
            opl2.setWaveForm(11, OPERATOR1, (getRandy(i) + opl[currentKit][i]) >> 1);
            break;  //wav
          case 122:
            opl2.setFeedback(9, (getRandy(i) + opl[currentKit][i]) >> 1);
            opl2.setFeedback(11, (getRandy(i) + opl[currentKit][i]) >> 1);
            break;  //feed
          case 123:
            setAlgo(9, getRandy(i) + map(opl[currentKit][i], 0, 15, 0, 4));
            setAlgo(11, getRandy(i) + map(opl[currentKit][i], 0, 15, 0, 4));
            break;  //algo
          case 124:
            opl2.setMultiplier(9, OPERATOR1, (getRandy(i) + opl[currentKit][i]));
            opl2.setMultiplier(11, OPERATOR1, (getRandy(i) + opl[currentKit][i]));
            break;  //mult
          case 125:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setAttack(9, OPERATOR1, temp);
            opl2.setAttack(11, OPERATOR1, temp);
            break;  //att
          case 126:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setRelease(9, OPERATOR1, temp);
            opl2.setRelease(11, OPERATOR1, temp);
            break;  //rel
          /////////////////////// .                   /////////////////////////
          case 127:
            updateVol(13);

            break;  // vol
          case 128:
            opl2.setWaveForm(9, OPERATOR2, (getRandy(i) + opl[currentKit][i]) >> 1);
            opl2.setWaveForm(11, OPERATOR2, (getRandy(i) + opl[currentKit][i]) >> 1);
            break;  //wav
          case 129:
            opl2.setMultiplier(9, OPERATOR2, (getRandy(i) + opl[currentKit][i]));
            opl2.setMultiplier(11, OPERATOR2, (getRandy(i) + opl[currentKit][i]));
            break;  //mult
          case 130:
            tremDepth[6] = getRandy(i) + opl[currentKit][i];
            tremDepth[6] = constrain(tremDepth[6], 0, 15);
            tremDepth[6] /= 15;
            break;  //trem
          case 131:
            vibratoDepth[6] = getRandy(i) + opl[currentKit][i];
            vibratoDepth[6] = constrain(vibratoDepth[6], 0, 15);
            vibratoDepth[6] /= 15;
            break;  //vib
          case 132:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setAttack(9, OPERATOR2, temp);
            opl2.setAttack(11, OPERATOR2, temp);
            break;  //att
          case 133:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setRelease(9, OPERATOR2, temp);
            opl2.setRelease(11, OPERATOR2, temp);
            break;  //rel
          /////////////////////// .                   /////////////////////////
          case 134:
            updateVol(14);

            break;  // vol
          case 135:
            opl2.setWaveForm(12, OPERATOR1, (getRandy(i) + opl[currentKit][i]) >> 1);
            opl2.setWaveForm(14, OPERATOR1, (getRandy(i) + opl[currentKit][i]) >> 1);
            break;  //wav
          case 136:
            opl2.setFeedback(12, (getRandy(i) + opl[currentKit][i]) >> 1);
            opl2.setFeedback(14, (getRandy(i) + opl[currentKit][i]) >> 1);
            break;  ////feed
          case 137:
            fat = getRandy(i) + opl[currentKit][i];
            fat = constrain(fat, 0, 15);
            freqLast[7] = -1;
            freqLast[6] = -1;
            break;  //tune
          case 138:
            opl2.setMultiplier(12, OPERATOR1, (getRandy(i) + opl[currentKit][i]));
            opl2.setMultiplier(14, OPERATOR1, (getRandy(i) + opl[currentKit][i]));
            break;  //mult
          case 139:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setAttack(12, OPERATOR1, temp);
            opl2.setAttack(14, OPERATOR1, temp);
            break;  //att
          case 140:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setRelease(12, OPERATOR1, temp);
            opl2.setRelease(14, OPERATOR1, temp);
            break;  //rel
          /////////      /////////////////////////
          case 141:
            updateVol(15);

            break;  // vol
          case 142:
            opl2.setWaveForm(12, OPERATOR2, (getRandy(i) + opl[currentKit][i]) >> 1);
            opl2.setWaveForm(14, OPERATOR2, (getRandy(i) + opl[currentKit][i]) >> 1);
            break;                                                            //wav
          case 143: multBase[3] = (getRandy(i) + opl[currentKit][i]); break;  //mult
          case 144:
            tremDepth[7] = getRandy(i) + opl[currentKit][i];
            tremDepth[7] = constrain(tremDepth[7], 0, 15);
            tremDepth[7] /= 15;
            break;  //trem
          case 145:
            vibratoDepth[7] = getRandy(i) + opl[currentKit][i];
            vibratoDepth[7] = constrain(vibratoDepth[7], 0, 15);
            vibratoDepth[7] /= 15;
            break;  //vibra
          case 146:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setAttack(12, OPERATOR2, temp);
            opl2.setAttack(14, OPERATOR2, temp);
            break;  //att
          case 147:
            temp = getRandy(i) + 15 - opl[currentKit][i];
            temp = constrain(temp, 0, 15);
            if (safe) temp = map(temp, 0, 15, 1, 15);
            opl2.setRelease(12, OPERATOR2, temp);
            opl2.setRelease(14, OPERATOR2, temp);
            break;  //rel
        }


        oplLast[i] = opl[currentKit][i];
      }
    }
  }

  // Apply shared drum multiplier with priority based on last-moved source.
  if (pendingCymOp2Mult >= 0) {
    opl2.setMultiplier(8, OPERATOR2, pendingCymOp2Mult);
  }
  if (pendingSnareMult >= 0 || pendingHatMult >= 0) {
    int16_t chosen = -1;
    if (opl[currentKit][151] == DRUM_SHARED_MULT_SNARE) {
      chosen = (pendingSnareMult >= 0) ? pendingSnareMult : pendingHatMult;
    } else {
      chosen = (pendingHatMult >= 0) ? pendingHatMult : pendingSnareMult;
    }
    if (chosen >= 0) {
      opl2.setMultiplier(7, OPERATOR1, chosen);
    }
  }

  resetRegisters = false;

  if ((pitchNowLast[1] != pitchNow[1])) {
    if (!bend) opl2.setMultiplier(6, OPERATOR1, ((pitchNow[1] >> 6) - offsetOct[currentPattern][0]));
    pitchNowLast[1] = pitchNow[1];
  }
  if ((pitchNowLast[0] != pitchNow[0])) {
    if (!bend) opl2.setFNumber(6, pitchNow[0] / (4 - (1 + offsetOct[currentPattern][0])));
    pitchNowLast[0] = pitchNow[0];
  }
  if ((pitchNowLast[2] != pitchNow[2])) {
    if (!bend) opl2.setFNumber(7, pitchNow[2] / (4 - (1 + offsetOct[currentPattern][1])));
    pitchNowLast[2] = pitchNow[2];
  }
  if ((pitchNowLast[3] != pitchNow[3])) {
    if (!bend) opl2.setFNumber(8, pitchNow[3] / (4 - (1 + offsetOct[currentPattern][2])));
    pitchNowLast[3] = pitchNow[2];
  }

  if ((pitchNowLast[4] != pitchNow[4])) {
    if (!bend) opl2.setMultiplier(8, OPERATOR2, (pitchNow[4] - offsetOct[currentPattern][3]));
    pitchNowLast[4] = pitchNow[4];
  }

  if (newTrem) {

    newTrem = false;

    updateVol(1);
    updateVol(3);
    updateVol(5);
    updateVol(7);
    updateVol(9);
    updateVol(11);
    updateVol(13);
    updateVol(15);
  }


  if (newVol) {

    for (int16_t i = 0; i < 16; i++) {
      updateVol(i);
    }

    newVol = false;
  }
}

int16_t randyRequests = 15;



int16_t getRandy(uint8_t number) {


  float wiggleAmp = randomDepth[currentPattern];
  wiggleAmp /= 9;
  bool wiggleSkip = false;

  if (wiggleOnEnabledStep) {
    switch (number) {
      //check which track we are requesting random for (and skip if wiggle only on active step is enabled)
      ///////////////////////////      KICK      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      case 1 ... 14:
        if (!bitRead(pattern[currentPattern][0][polyStep[0]], 0)) {
          wiggleSkip = true;
        }
        break;
      /////////////////////////////////      SNARE     ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      case 15 ... 21:
        if (!bitRead(pattern[currentPattern][0][polyStep[1]], 1)) {
          wiggleSkip = true;
        }
        break;
      /// HATS +cymbal ////
      case 24:
      case 26:
      case 148:
      case 149:
      case 150:
      case 32:
      case 35:
        if ((!bitRead(pattern[currentPattern][0][polyStep[3]], 3)) && (!bitRead(pattern[currentPattern][0][polyStep[4]], 4) && (!bitRead(pattern[currentPattern][0][polyStep[5]], 5)))) {
          wiggleSkip = true;
        }
        break;
      //cymbal only
      case 22:
      case 30:
      case 33:
        if (!bitRead(pattern[currentPattern][0][polyStep[5]], 5)) {
          wiggleSkip = true;
        }
        break;
      //TOMS ///
      case 23:
      case 25:
      case 29:
      case 27:
      case 31:
      case 34:
        if (!bitRead(pattern[currentPattern][0][polyStep[2]], 2)) {
          wiggleSkip = true;
        }
        break;
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /// second row synth1-2
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      case 36 ... 63:
        if (!getStep(0, polyStep[6])) {
          wiggleSkip = true;
        }
        break;

      case 64 ... 91:
        if (!getStep(1, polyStep[7])) {
          wiggleSkip = true;
        }
        break;
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /// second row synth3
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      case 92 ... 119:
        if (!getStep(2, polyStep[8])) {
          wiggleSkip = true;
        }
        break;
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      /// second row synth4
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      case 120 ... 147:
        if (!getStep(3, polyStep[9])) {
          wiggleSkip = true;
        }
        break;
    }
  }

  if (wiggleSkip) {
    if (randomer[currentPattern][number]) {
      return (lastWiggle[number] * wiggleAmp);
    } else {
      return (0);
    }
  } else {
    randyRequests++;
    randyRead++;
    if (randyRead > 15) randyRead = 0;
    lastWiggle[number] = randy[randyRead];
    if (randomer[currentPattern][number]) {
      return (lastWiggle[number] * wiggleAmp);
    } else {
      return (0);
    }
  }
}



void randyTick() {
  if (randyRequests) {
    randyRequests--;
    randyWrite++;
    if (randyWrite > 15) randyWrite = 0;
    randy[randyWrite] = random(-9, 9);
  }
}


void punchBend() {
  //bend automation
  if ((rec) && (mode == 2) && (function == 9)) {

    if ((stepTickCounter < stepTicks >> 1)) {
      bendSteps[currentPattern][polyStep[0]] = 1;
      saveAuto = true;
    }
  }
}

void startBend() {

  if (!bend) {

    //percussive
    fNumberRoot[0] = fNumberBend[0] = opl2.getFNumber(6);
    fNumberRoot[1] = fNumberBend[1] = opl2.getFNumber(7);
    fNumberRoot[2] = fNumberBend[2] = opl2.getFNumber(8);

    //synth
    fNumberRoot[3] = fNumberBend[3] = opl2.getFNumber(0);
    fNumberRoot[4] = fNumberBend[4] = opl2.getFNumber(3);
    fNumberRoot[5] = fNumberBend[5] = opl2.getFNumber(1);
    fNumberRoot[6] = fNumberBend[6] = opl2.getFNumber(4);
    fNumberRoot[7] = fNumberBend[7] = opl2.getFNumber(2);
    fNumberRoot[8] = fNumberBend[8] = opl2.getFNumber(5);
    fNumberRoot[9] = fNumberBend[9] = opl2.getFNumber(9);
    fNumberRoot[10] = fNumberBend[10] = opl2.getFNumber(12);


    fNumberRoot[11] = fNumberBend[11] = opl2.getFNumber(10);
    fNumberRoot[12] = fNumberBend[12] = opl2.getFNumber(13);
    fNumberRoot[13] = fNumberBend[13] = opl2.getFNumber(11);
    fNumberRoot[14] = fNumberBend[14] = opl2.getFNumber(14);

    bend = 1;
  }
}

void stopBend() {

  if (bend) {

    //percussive
    for (int i = 0; i < 15; i++) {
      fNumberBend[i] = fNumberRoot[i];
    }


    opl2.setFNumber(6, fNumberRoot[0]);
    opl2.setFNumber(7, fNumberRoot[1]);
    opl2.setFNumber(8, fNumberRoot[2]);

    //synth
    opl2.setFNumber(0, fNumberRoot[3]);
    opl2.setFNumber(3, fNumberRoot[4]);
    opl2.setFNumber(1, fNumberRoot[5]);
    opl2.setFNumber(4, fNumberRoot[6]);
    opl2.setFNumber(2, fNumberRoot[7]);
    opl2.setFNumber(5, fNumberRoot[8]);
    opl2.setFNumber(9, fNumberRoot[9]);
    opl2.setFNumber(12, fNumberRoot[10]);

    opl2.setFNumber(10, fNumberRoot[11]);
    opl2.setFNumber(13, fNumberRoot[12]);
    opl2.setFNumber(11, fNumberRoot[13]);
    opl2.setFNumber(14, fNumberRoot[14]);

    bend = false;
  }
}

void resetBendAutomation() {
  if (bend) {
    stopBend();
  }

  bend = false;
  bendHeld = false;
  bendStepLast = 0;
  bendCounter = 0;

  for (int i = 0; i < 16; i++) {
    newBend[i] = false;
  }
}

void loadRouting() {
  routing[0] = EEPROMread(3);
  routing[1] = EEPROMread(4);
  routing[2] = EEPROMread(5);
  routing[3] = EEPROMread(6);
  routing[4] = EEPROMread(7);
  routing[5] = EEPROMread(18);
  routing[6] = EEPROMread(19);


  if (EEPROMread(39) == 123) {
    loadRoutingPreset(EEPROMread(110));
  } else {
    for (int i = 0; i < 10; i++) {
      for (int j = 0; j < 7; j++) {
        routingPreset[i][j] = 255;
      }
    }
  }
}

void updateRouting() {

  opl2.setOutput(6, routing[0]);  //kick
  opl2.setOutput(7, routing[1]);  //snare  hats
  opl2.setOutput(8, routing[2]);  // tom  cymb

  opl2.setOutput(0, routing[3]);  //synth1
  opl2.setOutput(3, routing[3]);  //synth1

  opl2.setOutput(1, routing[4]);  //synth2
  opl2.setOutput(4, routing[4]);  //synth2

  opl2.setOutput(2, routing[5]);   //synth3
  opl2.setOutput(5, routing[5]);   //synth3
  opl2.setOutput(10, routing[5]);  //synth3
  opl2.setOutput(13, routing[5]);  //synth3

  opl2.setOutput(9, routing[6]);   //synth4
  opl2.setOutput(12, routing[6]);  //synth4
  opl2.setOutput(11, routing[6]);  //synth4
  opl2.setOutput(14, routing[6]);  //synth4
}

bool getProb(uint8_t number) {


  if (probStack[probCounter + 1] > number) {
    if (probCounter < 19) {
      probCounter++;
    } else {
    }
    return (1);
  } else {
    if (probCounter < 19) {
      probCounter++;
    } else {
    }
    return (0);
  }
}

void clearStutterButtons() {

  for (int16_t i = 0; i < 10; i++) {
    stutterHeld[i] = 0;
  }
}



void sortStutter() {
  uint8_t newStutter = 0;
  //set stutter to the highest held button
  for (int16_t i = 0; i < 10; i++) {
    if (stutterHeld[i]) newStutter = i + 1;
  }
  stutterNumber = newStutter;
  setStutter(newStutter, 0);
  //stutter automation
  if ((rec) && (mode == 2) && (function == 8)) {
    if ((stepTickCounter < stepTicks >> 1)) {
      if (stutterNumber) {
        stutterSteps[currentPattern][polyStep[0]] = stutterNumber;
        saveAuto = true;
      }
    }
  }
}


void setStutter(uint8_t number, bool automation) {

  if (!automation || stutterEnabled) {

    if (number) {
      if (!stutter || automation) {
        firstStutter = true;
        // Capture again for every automated step. Adjacent programmed steps
        // may have the same rate but must repeat their own notes.
        for (int i = 0; i < 10; i++) {
          stutterStepPoly[i] = polyStep[i];
        }
      }

      // 1 = manual stutter, 2 = sequenced stutter. The distinction is used by
      // the automation enable/disable controls and by manual override logic.
      stutter = automation ? 2 : 1;

      stutterCounter = stepTickCounter;
      switch (number) {
        case 1:
          stutterRate = stepTicks << 3;  // Equivalent to stepTicks * 8
          break;
        case 2:
          stutterRate = stepTicks << 2;  // should be stepTicks * 4
          break;
        case 3:
          stutterRate = stepTicks * 3;  // Cannot optimize with bitwise for *3
          break;
        case 4:
          stutterRate = stepTicks << 1;  // Equivalent to stepTicks * 2
          break;
        case 5:
          stutterRate = stepTicks;  // No change needed
          break;
        case 6:
          stutterRate = stepTicks / 1.5;  // Cannot optimize with bitwise, remains as is
          break;
        case 7:
          stutterRate = stepTicks >> 1;  // Equivalent to stepTicks / 2
          break;
        case 8:
          stutterRate = stepTicks / 3;  // Division by 3 cannot be optimized with bitwise
          break;
        case 9:
          stutterRate = stepTicks >> 2;  // Equivalent to stepTicks / 4
          break;
        case 10:
          stutterRate = stepTicks >> 3;  // Equivalent to stepTicks / 8
          break;
      }
    } else if (!automation || stutter == 2) {
      // Ending an automation step must not cancel a manually held stutter.
      stutter = stutterCounter = 0;
    }
  }
}
void updateOctaves() {

  for (int16_t i = 0; i < 5; i++) {
    pitchNowLast[i] = -1;
  }
}


uint8_t getRandom() {

  randomIndex++;
  if (randomIndex > 7) {
    randomIndex = 0;
    seedNewRandomTable = true;
  }
  return (randomTable[randomIndex]);
}
