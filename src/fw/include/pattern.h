#include "pattern_visualizer.h"

void armPatt() {
  //called when patt changed and rec is high
  if (!pattArmed) {
    pattArmed = true;
    if (!run) {
      pattChainLength = 0;
    }
  }
}

void nextPattern() {

    
  if (!chainFreeze) {
    if (pattArmed) {
      pattChainStep++;
    } else {
      //is there a chain of at least one patt?
      if (pattChainLength > 0) {
        pattChainStep++;  //
      }
    }
    if ((!pattArmed)) {
      //playback
      //
      if (pattChainLength) {
        if (pattChainStep > pattChainLength) {
          pattChainStep = 0;
        }
        loadPattern(pattChain[pattChainStep]);

            if (lastPattern != pattChain[pattChainStep]) {
      allStepsZeroFlag = true;
      lastPattern = pattChain[pattChainStep];
    }


      }
    }
  }
  //user requested a pattern - override the chain
  if (pendingPattern > -1) {
    loadPattern(pendingPattern);
    if (lastPattern != pendingPattern) {
      allStepsZeroFlag = true;
      lastPattern = pendingPattern;
    }
    pendingPattern = -1;
  }
  if ((!chainFreeze) && (pattArmed)) {
    //record
    if (pattChainStep > pattChainLength) {
      pattChainLength++;
    }  //only extend chain if we're adding to the end, otherwise punch in
    //otherwise just punch
    pattChain[pattChainStep] = currentPattern;
  }
  if (mode == 2) {
    showMode();
  } else {
    showPattern();
  }
  markPatternVisualizerDirty();
}

void rotate(bool direction) {
  byte sample;

  if (direction) {
    //forward, sample last step, shift others to the right and drop sampled step at the first step.
    if (selectedVoice < 6) {
      sample = bitRead(pattern[currentPattern][0][(selectedPage << 4) + 15], selectedVoice);
      for (int i = 0; i <15; i++ ){
        bitWrite(pattern[currentPattern][0][(selectedPage << 4) + (15-i)], selectedVoice, bitRead(pattern[currentPattern][0][(selectedPage << 4) + (14- i)], selectedVoice));
      }
      bitWrite(pattern[currentPattern][0][(selectedPage << 4)], selectedVoice, sample);
    } else {
      sample = getStep(selectedVoice - 6, (selectedPage << 4) + 15);
      for (int i = 0; i <15; i++) { setStep(selectedVoice - 6, (selectedPage << 4) + (15-i), getStep(selectedVoice - 6, (selectedPage << 4) + (14-i))); }
      setStep(selectedVoice - 6, (selectedPage << 4), sample);

      sample = getNote(selectedVoice - 6, (selectedPage << 4) + 15);
      for (int i = 0; i <15; i++) { setNote(selectedVoice - 6, (selectedPage << 4) + (15-i), getNote(selectedVoice - 6, (selectedPage << 4) + (14-i))); }
      setNote(selectedVoice - 6, (selectedPage << 4), sample);

      if (selectedVoice == 8) {
        sample = getNote2((selectedPage << 4) + 15);
        for (int i = 0; i <15; i++) { setNote2((selectedPage << 4) + (15-i), getNote2((selectedPage << 4) + (14-i))); }
        setNote2((selectedPage << 4), sample);
      }
    }
  } else {
    //backward, sample first step, shift others to the left and drop sampled step at the last step.
    if (selectedVoice < 6) {
      sample = bitRead(pattern[currentPattern][0][(selectedPage << 4)], selectedVoice);
      for (int i = 0; i < 15; i++) {
        bitWrite(pattern[currentPattern][0][(selectedPage << 4) + i], selectedVoice, bitRead(pattern[currentPattern][0][(selectedPage << 4) + i + 1], selectedVoice));
      }
      bitWrite(pattern[currentPattern][0][(selectedPage << 4) + 15], selectedVoice, sample);
    } else {
      sample = getStep(selectedVoice - 6, (selectedPage << 4));
      for (int i = 0; i < 15; i++) { setStep(selectedVoice - 6, (selectedPage << 4) + i, getStep(selectedVoice - 6, (selectedPage << 4) + i + 1)); }
      setStep(selectedVoice - 6, (selectedPage << 4) + 15, sample);

      sample = getNote(selectedVoice - 6, (selectedPage << 4));
      for (int i = 0; i < 15; i++) { setNote(selectedVoice - 6, (selectedPage << 4) + i, getNote(selectedVoice - 6, (selectedPage << 4) + i + 1)); }
      setNote(selectedVoice - 6, (selectedPage << 4) + 15, sample);

      if (selectedVoice == 8) {
        sample = getNote2((selectedPage << 4));
        for (int i = 0; i < 15; i++) { setNote2((selectedPage << 4) + i, getNote2((selectedPage << 4) + i + 1)); }
        setNote2((selectedPage << 4) + 15, sample);
      }
    }
  }
  showPattern();
  markPatternVisualizerDirty();
}

void rotateOld(bool direction) {
  //rotate the selected instrument @ selected page by 1 step.(direction=0=left direction=1=right)
  for (int16_t page = 0; page < 4; page++) {
    if (direction) {
      //forward
      if (selectedVoice < 6) {  //drums
        //buffer current page (rotated)
        temp16[i] = bitRead(pattern[currentPattern][0][x16[page] + 15], selectedVoice);
        for (int16_t i = 0; i < 15; i++) {
          temp16[i + 1] = bitRead(pattern[currentPattern][0][x16[page] + i], selectedVoice);
        }
        //overWrite page with buffer
        for (int16_t i = 0; i < 16; i++) {
          bitWrite(pattern[currentPattern][0][x16[page] + i], selectedVoice, temp16[i]);
        }
      } else {  //synths
        //steps
        //buffer current page (rotated)
        temp16[i] = getStep(selectedVoice - 6, x16[page] + 15);
        for (int16_t i = 0; i < 15; i++) {
          temp16[i + 1] = getStep(selectedVoice - 6, x16[page] + i);
        }
        //overWrite page with buffer
        for (int16_t i = 0; i < 16; i++) {
          setStep(selectedVoice - 6, x16[page] + i, temp16[i]);
        }
        //notes
        //buffer current page (rotated)
        temp16[i] = getNote(selectedVoice - 6, x16[page] + 15);
        for (int16_t i = 0; i < 15; i++) {
          temp16[i + 1] = getNote(selectedVoice - 6, x16[page] + i);
        }
        //overWrite page with buffer
        for (int16_t i = 0; i < 16; i++) {
          setNote(selectedVoice - 6, x16[page] + i, temp16[i]);
        }
        if (selectedVoice == 8) {
          temp16[i] = getStep2(x16[page] + 15);
          for (int16_t i = 0; i < 15; i++) {
            temp16[i + 1] = getStep2(x16[page] + i);
          }
          for (int16_t i = 0; i < 16; i++) {
            setStep2(x16[page] + i, temp16[i]);
          }
          temp16[i] = getNote2(x16[page] + 15);
          for (int16_t i = 0; i < 15; i++) {
            temp16[i + 1] = getNote2(x16[page] + i);
          }
          for (int16_t i = 0; i < 16; i++) {
            setNote2(x16[page] + i, temp16[i]);
          }
        }  //synth 3 dual track
      }
    } else {
      //backward
      if (selectedVoice < 6) {  //drums
        //buffer current page (rotated)
        temp16[15] = bitRead(pattern[currentPattern][0][x16[page]], selectedVoice);
        for (int16_t i = 0; i < 15; i++) {
          temp16[i] = bitRead(pattern[currentPattern][0][x16[page] + i + 1], selectedVoice);
        }
        //overWrite page with buffer
        for (int16_t i = 0; i < 16; i++) {
          bitWrite(pattern[currentPattern][0][x16[page] + i], selectedVoice, temp16[i]);
        }
      } else {  //synths
        //steps
        //buffer current page (rotated)
        temp16[15] = getStep(selectedVoice - 6, x16[page]);
        for (int16_t i = 0; i < 15; i++) {
          temp16[i] = getStep(selectedVoice - 6, x16[page] + i + 1);
        }
        //overWrite page with buffer
        for (int16_t i = 0; i < 16; i++) {
          setStep(selectedVoice - 6, x16[page] + i, temp16[i]);
        }
        //notes
        //buffer current page (rotated)
        temp16[15] = getNote(selectedVoice - 5, x16[page]);
        for (int16_t i = 0; i < 15; i++) {
          temp16[i] = getNote(selectedVoice - 6, x16[page] + i + 1);
        }
        //overWrite page with buffer
        for (int16_t i = 0; i < 16; i++) {
          setNote(selectedVoice - 6, x16[page] + i, temp16[i]);
        }
        if (selectedVoice == 8) {
          temp16[15] = getStep2(x16[page]);
          for (int16_t i = 0; i < 15; i++) {
            temp16[i] = getStep2(x16[page] + i + 1);
          }
          for (int16_t i = 0; i < 16; i++) {
            setStep2(x16[page] + i, temp16[i]);
          }
          temp16[15] = getNote2(x16[page]);
          for (int16_t i = 0; i < 15; i++) {
            temp16[i] = getNote2(x16[page] + i + 1);
          }
          for (int16_t i = 0; i < 16; i++) {
            setNote2(x16[page] + i, temp16[i]);
          }
        }  //synth 3 dual track
      }
    }
  }
  dontChangePage = true;
  showMode();
}

void changePattern(uint8_t number) {
  if (!pattChainLength) {
    EEPROMupdate(25, number);  //update EEPROM with pattern number if no chain
  }
  if (run) {
    pendingPattern = number;
  } else {
    if (lastPattern != number) {
      allStepsZeroFlag = true;
      lastPattern = number;
    }
    loadPattern(number);
    if (pattArmed) {
      pattChain[0] = number;
    }
  }
}

void setNudge(uint8_t input) {
  plocked = true;
  if (input > 9) input = 9;
  // Calculate the index once to avoid repetition and potential mistakes
  int index = (plocStep - 1) + x16[selectedPage];
  int voiceIndex = selectedVoice < 5 ? selectedVoice : selectedVoice - 5;
  int bitOffset = selectedVoice < 5 ? 0 : 4;
  // Use a loop to set the bits instead of writing each one out
  for (int i = 0; i < 4; i++) {
    bitWrite(nudgeDelay[currentPattern][voiceIndex][index], i + bitOffset, bitRead(input, i));
  }
}

void setNudgeAtStep(uint8_t voice) {
  if (!quantise) {
    //cancel the toggling of the step
    if (voice < 5) {
      bitWrite(nudgeDelay[currentPattern][voice][targetStep], 0, bitRead(nudgeStep, 0));
      bitWrite(nudgeDelay[currentPattern][voice][targetStep], 1, bitRead(nudgeStep, 1));
      bitWrite(nudgeDelay[currentPattern][voice][targetStep], 2, bitRead(nudgeStep, 2));
      bitWrite(nudgeDelay[currentPattern][voice][targetStep], 3, bitRead(nudgeStep, 3));
    } else {
      bitWrite(nudgeDelay[currentPattern][voice - 5][targetStep], 4, bitRead(nudgeStep, 0));
      bitWrite(nudgeDelay[currentPattern][voice - 5][targetStep], 5, bitRead(nudgeStep, 1));
      bitWrite(nudgeDelay[currentPattern][voice - 5][targetStep], 6, bitRead(nudgeStep, 2));
      bitWrite(nudgeDelay[currentPattern][voice - 5][targetStep], 7, bitRead(nudgeStep, 3));
    }
  }
}

void setNudgeAnyValueAtAnyStep(uint8_t anyStep, uint8_t anyValue, uint8_t voice) {

    //cancel the toggling of the step
    if (voice < 5) {
      bitWrite(nudgeDelay[currentPattern][voice][anyStep], 0, bitRead(anyValue, 0));
      bitWrite(nudgeDelay[currentPattern][voice][anyStep], 1, bitRead(anyValue, 1));
      bitWrite(nudgeDelay[currentPattern][voice][anyStep], 2, bitRead(anyValue, 2));
      bitWrite(nudgeDelay[currentPattern][voice][anyStep], 3, bitRead(anyValue, 3));
    } else {
      bitWrite(nudgeDelay[currentPattern][voice - 5][anyStep], 4, bitRead(anyValue, 0));
      bitWrite(nudgeDelay[currentPattern][voice - 5][anyStep], 5, bitRead(anyValue, 1));
      bitWrite(nudgeDelay[currentPattern][voice - 5][anyStep], 6, bitRead(anyValue, 2));
      bitWrite(nudgeDelay[currentPattern][voice - 5][anyStep], 7, bitRead(anyValue, 3));
    }
  
}



uint8_t getNudge(uint8_t step, uint8_t voice) {
  //return nudge at current held step
  if (wiggleNudge) {
    return (map(random(9), 0, 8, 0, randomDepth[currentPattern]));
  } else {
    if (voice < 5) {
      return (lowNibble(nudgeDelay[currentPattern][voice][step]));
    } else {
      return (highNibble(nudgeDelay[currentPattern][voice - 5][step]));
    }
  }
}

void loadPattern(uint8_t input) {
  if (vibratoRate[currentPattern] != vibratoRate[input]) newVibRate = true;
  if (tremRate[currentPattern] != tremRate[input]) newTremRate = true;
  currentPattern = input;
  resetBendAutomation();
  key = keyRoot[currentPattern];
  currentKit = kit[currentPattern];
  updateCurrentKit();
  updateSwing(swingSetting[currentPattern]);
  updateOctaves();
  showPattern();
  markPatternVisualizerDirty();
  tremWav = tremWavPatt[currentPattern];
  vibWav = vibWavPatt[currentPattern];
  if ((mode == 2) && (function == 11)) showMode();  //update top LED
  if ((mode == 2) && (function == 12)) showMode();  //update top LED
  resetRegisters = true;
}

void clearNudges() {
  for (int16_t i = 0; i < 5; i++) {
    for (int16_t j = 0; j < 64; j++) {
      nudgeDelay[currentPattern][i][j] = 0;
    }
  }
}

void changeKit(uint8_t number) {
  if (run) {
    pendingKit = number + 1;
  } else {
    kit[currentPattern] = number;
    updateCurrentKit();
    resetRegisters = true;
  }
}
