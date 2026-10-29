void movedFader(uint8_t number, uint8_t data) {

  data = 15 - data;
  moved = 0;

  bool plocking;

  //ploc
  if ((!rec) && (plocStep)) {
    plocking = true;
  } else {
    plocking = false;
  }

  uint8_t offset = 0;
  if (selectedSynth == 0) offset = 0;        //virtual third row
  if (selectedSynth == 1) offset = 28;       //virtual third row
  else if (selectedSynth == 2) offset = 56;  //virtual 4th row
  else if (selectedSynth == 3) offset = 84;  //virtual 5th row


  switch (number) {



    case 0:
      moved = 1;
      if (!plocking) opl[currentKit][1] = data;
      break;  //vol
    case 1:
      moved = 2;
      if (!plocking) {
        opl[currentKit][2] = data;
        if (mode == 1) {
          showNumber((data >> 1));
        }
      }
      break;  //wav
    case 2:
      moved = 3;
      if (!plocking) opl[currentKit][3] = data;
      break;  //feedback
    case 3:
      moved = 4;
      if (!plocking) opl[currentKit][4] = data;
      break;  //mult
    case 4:
      moved = 5;
      if (!plocking) opl[currentKit][5] = data;
      break;  //tune
    case 5:
      moved = 6;
      if (!plocking) opl[currentKit][6] = data;
      break;  //fall
    case 6:
      moved = 7;
      if (!plocking) opl[currentKit][7] = data;
      break;  //att
    case 14:
      moved = 8;
      if (!plocking) opl[currentKit][8] = data;
      break;  //decay

      /////////// .  /////////// .  /////////// .  /////////// .  ///////////

    case 15:
      moved = 9;
      if (!plocking) opl[currentKit][9] = data;
      break;  //vol
    case 16:
      moved = 10;
      if (!plocking) opl[currentKit][10] = data;
      if (mode == 1) {
        showNumber((data >> 1));
      }
      break;  //wav
    case 17:
      moved = 11;
      if (!plocking) opl[currentKit][11] = data;
      break;  //mult
    case 18:
      moved = 12;
      if (!plocking) opl[currentKit][12] = data;
      break;  //fall
    case 19:
      moved = 13;
      if (!plocking) opl[currentKit][13] = data;
      break;  //att
    case 20:
      moved = 14;
      if (!plocking) opl[currentKit][14] = data;
      break;  //rel

      /////////// .  /////////// .  /////////// .  /////////// .  /////////// .

    case 28:
      moved = 15;
      if (!plocking) opl[currentKit][15] = data;
      break;  //vol
    case 29:
      moved = 16;
      if (!plocking) {
        opl[currentKit][16] = data;
        if (mode == 1) {

          if ((data >> 1 == 3) || (data >> 1 == 4) || (data >> 1 == 5)) {
            data = 4;
          }
          showNumber((data >> 1));
        }
      }
      break;  //wav
    case 30:
      moved = 19;
      if (!plocking) {
        opl[currentKit][19] = data;
        opl[currentKit][151] = DRUM_SHARED_MULT_SNARE;
      }
      break;  //mult
    case 31:
      moved = 17;
      if (!plocking) opl[currentKit][17] = data;
      break;  //tune
    case 32:
      moved = 18;
      if (!plocking) opl[currentKit][18] = data;
      break;  //fall
    case 33:
      moved = 20;
      if (!plocking) opl[currentKit][20] = data;
      break;  //att
    case 34:
      moved = 21;
      if (!plocking) opl[currentKit][21] = data;
      break;  //rel

      /////////// .  /////////// .  /////////// .  /////////// .  /////////// .

    case 42:
      if (selectedVoice == 5) {
        moved = 22;
        if (!plocking) {
          opl[currentKit][22] = data;
        }
      } else if (selectedVoice == 2) {
        moved = 23;
        if (!plocking) {
          opl[currentKit][23] = data;
        }
      } else {
        moved = 24;
        if (!plocking) opl[currentKit][24] = data;
      }
      break;  //vol
    case 43:
      if (selectedVoice == 2) {
        moved = 25;
        if (!plocking) {
          opl[currentKit][25] = data;
        }
      } else {
        moved = 26;
        if (!plocking) opl[currentKit][26] = data;
      }
      if (mode == 1) {
        showNumber((data >> 1));
      }
      break;  //wav
    case 44:
      if (selectedVoice == 2) {
        moved = 29;
        if (!plocking) {
          opl[currentKit][29] = data;
        }
      } else {
        moved = 148;
        if (!plocking) {
          opl[currentKit][148] = data;
          opl[currentKit][151] = DRUM_SHARED_MULT_HATS;
        }
      }
      break;  //mult
    case 45:
      if (selectedVoice == 2) {
        moved = 27;
        if (!plocking) {
          opl[currentKit][27] = data;
        }
      } else {
        moved = 149;
        if (!plocking) opl[currentKit][149] = data;
      }
      break;  //tune
    case 46:
    
      if (selectedVoice == 2) {
        moved = 28;
        if (!plocking) {
          opl[currentKit][28] = data;
        }
      } else {
        moved = 150;
        if (!plocking) opl[currentKit][150] = data;
      }
      
      break;  //fall  //was moved=88.. error?
    case 47:
      if (selectedVoice == 5) {
        moved = 30;
        if (!plocking) {
          opl[currentKit][30] = data;
        }
      } else if (selectedVoice == 2) {
        moved = 31;
        if (!plocking) opl[currentKit][31] = data;
      } else {
        moved = 32;
        if (!plocking) opl[currentKit][32] = data;
      }
      break;  //att
    case 48:
      if (selectedVoice == 5) {
        moved = 33;
        if (!plocking) {
          opl[currentKit][33] = data;
        }
      } else if (selectedVoice == 2) {
        moved = 34;
        if (!plocking) opl[currentKit][34] = data;
      } else {
        moved = 35;
        if (!plocking) opl[currentKit][35] = data;
      }
      break;  //rel

    /////////// .  /////////// .  /////////// .
    /////////// .   SECOND ROW .             .
    /////////// .  /////////// .  /////////// .
    case 7:
      moved = offset + 36;
      if (!plocking) opl[currentKit][offset + 36] = data;
      showNumberSynth(map(opl[currentKit][offset + 39], 0, 15, 0, 4));  //show algo
      break;                                                            //vol
    case 8:
      moved = offset + 37;
      if (!plocking) {
        opl[currentKit][offset + 37] = data;
      }
      if (mode == 1) {
        showNumber((data >> 1));
      }
      break;  //wav
    case 9:
      moved = offset + 38;
      if (!plocking) opl[currentKit][offset + 38] = data;
      break;  //feed
    case 10:
      moved = offset + 39;
      if (!plocking) {
        opl[currentKit][offset + 39] = data;
      }
      if (mode == 1) {
        showNumberSynth(map(data, 0, 15, 0, 4));
      }
      break;  //algo
    case 11:
      moved = offset + 40;
      if (!plocking) opl[currentKit][offset + 40] = data;
      break;  ////mult
    case 12:
      moved = offset + 41;
      if (!plocking) opl[currentKit][offset + 41] = data;
      showNumberSynth(map(opl[currentKit][offset + 39], 0, 15, 0, 4));  //show algo
      break;                                                            //att
    case 13:
      moved = offset + 42;
      if (!plocking) opl[currentKit][offset + 42] = data;
      showNumberSynth(map(opl[currentKit][offset + 39], 0, 15, 0, 4));  //show algo
      break;                                                            //rel

    case 21:
      moved = offset + 43;
      if (!plocking) opl[currentKit][offset + 43] = data;
      showNumberSynth(map(opl[currentKit][offset + 39], 0, 15, 0, 4));  //show algo
      break;                                                            //vol
    case 22:
      moved = offset + 44;
      if (!plocking) {
        opl[currentKit][offset + 44] = data;
      }
      if (mode == 1) {
        showNumber((data >> 1));
      }
      break;  //wav
    case 23:
      moved = offset + 45;
      if (!plocking) opl[currentKit][offset + 45] = data;
      break;  //tune
    case 24:
      moved = offset + 46;
      if (!plocking) opl[currentKit][offset + 46] = data;
      break;  //trem
    case 25:
      moved = offset + 47;
      if (!plocking) opl[currentKit][offset + 47] = data;
      break;  ////vibra
    case 26:
      moved = offset + 48;
      if (!plocking) opl[currentKit][offset + 48] = data;
      showNumberSynth(map(opl[currentKit][offset + 39], 0, 15, 0, 4));  //show algo
      break;                                                            //att
    case 27:
      moved = offset + 49;
      if (!plocking) opl[currentKit][offset + 49] = data;
      showNumberSynth(map(opl[currentKit][offset + 39], 0, 15, 0, 4));  //show algo
      break;                                                            //rel

    case 35:
      moved = offset + 50;
      if (!plocking) opl[currentKit][offset + 50] = data;
      showNumberSynth(map(opl[currentKit][offset + 39], 0, 15, 0, 4));  //show algo
      break;                                                            //vol
    case 36:
      moved = offset + 51;
      if (!plocking) {
        opl[currentKit][offset + 51] = data;
      }
      if (mode == 1) {
        showNumber((data >> 1));
      }
      break;  //wav
    case 37:
      moved = offset + 52;
      if (!plocking) opl[currentKit][offset + 52] = data;
      break;  //fdb
    case 38:
      moved = offset + 53;
      if (!plocking) opl[currentKit][offset + 53] = data;
      break;  //tune
    case 39:
      moved = offset + 54;
      if (!plocking) opl[currentKit][offset + 54] = data;
      break;  ////mult
    case 40:
      moved = offset + 55;
      if (!plocking) opl[currentKit][offset + 55] = data;
      showNumberSynth(map(opl[currentKit][offset + 39], 0, 15, 0, 4));  //show algo
      break;                                                            //att
    case 41:
      moved = offset + 56;
      if (!plocking) opl[currentKit][offset + 56] = data;
      showNumberSynth(map(opl[currentKit][offset + 39], 0, 15, 0, 4));  //show algo
      break;                                                            //rel

    case 49:
      moved = offset + 57;
      if (!plocking) opl[currentKit][offset + 57] = data;
      showNumberSynth(map(opl[currentKit][offset + 39], 0, 15, 0, 4));  //show algo
      break;                                                            //vol
    case 50:
      moved = offset + 58;
      if (!plocking) {
        opl[currentKit][offset + 58] = data;
      }
      if (mode == 1) {
        showNumber((data >> 1));
      }
      break;  //wav
    case 51:
      moved = offset + 59;
      if (!plocking) opl[currentKit][offset + 59] = data;
      break;  //mult
    case 52:
      moved = offset + 60;
      if (!plocking) opl[currentKit][offset + 60] = data;
      break;  //trem
    case 53:
      moved = offset + 61;
      if (!plocking) opl[currentKit][offset + 61] = data;
      break;  ////vibra
    case 54:
      moved = offset + 62;
      if (!plocking) opl[currentKit][offset + 62] = data;
      showNumberSynth(map(opl[currentKit][offset + 39], 0, 15, 0, 4));  //show algo
      break;                                                            //att
    case 55:
      moved = offset + 63;
      if (!plocking) opl[currentKit][offset + 63] = data;
      showNumberSynth(map(opl[currentKit][offset + 39], 0, 15, 0, 4));  //show algo
      break;                                                            //rel
  }

  sendFaderCC(moved, data);

  //wiggler attach/detach
  if ((mode == 2) && (function == 3) && (wiggleHeld)) {
    detachedAFader = true;
    randomer[currentPattern][moved] = !clearHeld;
    saveAuto = true;
  } else {

    //ploc
    if (plocking) {
      aStorePloc(currentPattern,moved, data, plocStep - 1,selectedPage);
      plocked = true;
    }



    //auto
    if ((rec) && (!clearHeld)) {

      aStore(moved, data,polyStep[0]);
    } else if (clearHeld) {
      clearFader(moved);
    }
  }


}

void resetFaders() {

  for (int16_t i = 1; i < 149; i++) {
    opl[currentKit][i] = 0;
  }
}


void sendFaderCC(uint8_t number, uint8_t data) {



  //drum CC = 20-55
  if ((number < 36) || (number == 148) || (number == 149) || (number == 150)) {
    if (number < 148) {
      switch (number) {
        case 1 ... 14:
          if (!mute[0]) sendControlChange(number + 19, data << 3, ch10);
          break;
        case 15 ... 21:
          if (!mute[1]) sendControlChange(number + 19, data << 3, ch10);
          break;
        case 23:
        case 25:
        case 29:
        case 27:
        case 28:
        case 31:
        case 34:
          if (!mute[2]) sendControlChange(number + 19, data << 3, ch10);
          break;
        case 24:
        case 26:
        case 32:
        case 35:
          if (!mute[3] || !mute[4] || !mute[5]) sendControlChange(number + 19, data << 3, ch10);
          break;
        case 22:
        case 30:
        case 33:
          if (!mute[5]) sendControlChange(number + 19, data << 3, ch10);
          break;
      }

    } else {
      switch (number) {
        case 148:
        case 149:
          if (!mute[3] || !mute[4] || !mute[5]) sendControlChange(number - 93, data << 3, ch10);
          break;
        case 150:
          if (!mute[5]) sendControlChange(number - 93, data << 3, ch10);
          break;
      }
    }
  }

  //synth1
  if ((number > 35) && (number < 36 + 28)) {
    if (!mute[6]) sendControlChange(number - 16, data << 3, ch1);
  }
  //synth2
  if ((number > 35 + 28) && (number < 36 + 56)) {
    if (!mute[7]) sendControlChange(number - 16 - 28, data << 3, ch2);
  }
  //synth3
  if ((number > 35 + 56) && (number < 36 + 84)) {
    if (!mute[8]) sendControlChange(number - 16 - 56, data << 3, ch3);
  }
  //synth4
  if ((number > 35 + 84) && (number < 148)) {
    if (!mute[9]) sendControlChange(number - 16 - 84, data << 3, ch4);
  }
}
