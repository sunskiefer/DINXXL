#include <math.h>

#include "pattern_visualizer.h"

constexpr float MIDI_PITCH_BEND_RANGE_SEMITONES = 2.0f;
constexpr float MIDI_PITCH_BEND_UNIT = 1.0f / 8192.0f;

void setMidiPitchBend(uint8_t voice, int16_t bendValue) {
  if (voice >= 4) { return; }
  if (midiPitchBendValue[voice] == bendValue) { return; }
  midiPitchBendValue[voice] = bendValue;
  if (bendValue == 0) {
    midiPitchBendFactor[voice] = 1.0f;
    return;
  }
  const float semitoneOffset = static_cast<float>(bendValue) * MIDI_PITCH_BEND_RANGE_SEMITONES * MIDI_PITCH_BEND_UNIT;
  if (fabsf(semitoneOffset) < 0.0001f) {
    midiPitchBendFactor[voice] = 1.0f;
  } else {
    midiPitchBendFactor[voice] = powf(2.0f, semitoneOffset / 12.0f);
  }
}

// Function to return the constrained note number in the specified mode
int constrainNoteToMode(int incomingNote, int rootKey, int mode) {
  rootKey += 60;
  // Define the intervals for the chromatic, major, and other scales in semitones
  const uint8_t chromaticScaleIntervals[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  const uint8_t majorScaleIntervals[] = { 2, 2, 1, 2, 2, 2, 1 };  // Ionian
  const uint8_t minorScaleIntervals[] = { 2, 1, 2, 2, 1, 2, 2 };  // Aeolian
  const uint8_t dorianScaleIntervals[] = { 2, 1, 2, 2, 2, 1, 2 };
  const uint8_t lydianScaleIntervals[] = { 2, 2, 2, 1, 2, 2, 1 };
  const uint8_t mixolydianScaleIntervals[] = { 2, 2, 1, 2, 2, 1, 2 };
  const uint8_t bluesScaleIntervals[] = { 3, 2, 1, 1, 3, 2 };  // Added Blues
  const uint8_t melodicMinorScaleIntervals[] = { 2, 1, 2, 2, 2, 2, 1 };
  const uint8_t harmonicMinorScaleIntervals[] = { 2, 1, 2, 2, 1, 3, 1 };
  const uint8_t gypsyScaleIntervals[] = { 2, 1, 3, 1, 1, 3, 1 };  // Added Gypsy

  // Array of pointers to the scale interval arrays
  const uint8_t* scalePatterns[] = {
    chromaticScaleIntervals,
    majorScaleIntervals,
    minorScaleIntervals,
    dorianScaleIntervals,
    lydianScaleIntervals,
    mixolydianScaleIntervals,
    bluesScaleIntervals,  // Replacing Locrian with Blues
    melodicMinorScaleIntervals,
    harmonicMinorScaleIntervals,
    gypsyScaleIntervals  // Replacing Phrygian with Gypsy
  };

  // Lengths of the scale patterns, adjusted for added scales
  int scaleLengths[] = { 12, 7, 7, 7, 7, 7, 6, 7, 7, 7 };  // Blues and Gypsy have different lengths

  // Select the scale pattern based on the mode

  const uint8_t* selectedScaleIntervals = scalePatterns[mode];
  int scaleLength = scaleLengths[mode];  // Lengths vary, especially for Chromatic and Blues

  // If mode is Chromatic, return the note directly
  if (mode == 0) {
    return incomingNote;
  }

  // Calculate the cumulative intervals from the root for each note in the scale
  int cumulativeInterval[scaleLength];
  cumulativeInterval[0] = 0;  // Initialize with the root note
  for (int i = 1; i < scaleLength; ++i) {
    cumulativeInterval[i] = cumulativeInterval[i - 1] + selectedScaleIntervals[i - 1];
  }

  // Calculate the number of semitones from the root key to the incoming note
  int semitoneOffsetFromRoot = (incomingNote - rootKey) % 12;
  if (semitoneOffsetFromRoot < 0) semitoneOffsetFromRoot += 12;  // Ensure positive offset

  // Find the closest scale note to the incoming note
  int closestScaleNote = 0;
  int minDiff = 12;  // Maximum possible difference in semitones
  for (int i = 0; i < scaleLength; ++i) {
    int currentDiff = abs(semitoneOffsetFromRoot - cumulativeInterval[i] % 12);
    if (currentDiff < minDiff) {
      minDiff = currentDiff;
      closestScaleNote = cumulativeInterval[i];
    }
  }

  // Calculate and return the MIDI note number of the closest scale note
  int constrainedNote = rootKey + closestScaleNote;
  while (constrainedNote >= 12) constrainedNote -= 12;  // Ensure the note is within 0-11
  constrainedNote += (incomingNote / 12) * 12;          // Adjust for the correct octave
  return constrainedNote;
}



void updateVol(uint8_t number) {
  int16_t tempVol = 0;

  switch (number) {

    case 0:
      tempVol = 63 - ((getRandy(36) + opl[currentKit][36]) << 2);
      break;  //A OP1

    case 1:
      tempVol = 63 - ((getRandy(43) + opl[currentKit][43]) << 2);
      tempVol += map(tremDepth[0] * tremolo, 0, 63, 0, 63 - tempVol);


      break;  // A OP2

    case 2:
      tempVol = 63 - ((getRandy(50) + opl[currentKit][50]) << 2);
      break;  // A OP3

    case 3:
      tempVol = 63 - ((getRandy(57) + opl[currentKit][57]) << 2);
      //tempVol-=tremDepth[1]*tremolo;
      tempVol += map(tremDepth[1] * tremolo, 0, 63, 0, 63 - tempVol);
      break;  // A OP4

      //// /// / / / / / / / // / //////// /

    case 4:
      tempVol = 63 - ((getRandy(64) + opl[currentKit][64]) << 2);
      break;  //B OP1

    case 5:
      tempVol = 63 - ((getRandy(71) + opl[currentKit][71]) << 2);
      tempVol += map(tremDepth[2] * tremolo, 0, 63, 0, 63 - tempVol);
      break;  // B OP2

    case 6:
      tempVol = 63 - ((getRandy(78) + opl[currentKit][78]) << 2);
      break;  // B OP3

    case 7:
      tempVol = 63 - ((getRandy(85) + opl[currentKit][85]) << 2);
      tempVol += map(tremDepth[3] * tremolo, 0, 63, 0, 63 - tempVol);
      break;  // B OP4


    //// /// / / / / / / / // / //////// /
    case 8:
      tempVol = 63 - ((getRandy(92) + opl[currentKit][92]) << 2);
      break;  //A OP1

    case 9:
      tempVol = 63 - ((getRandy(99) + opl[currentKit][99]) << 2);
      tempVol += map(tremDepth[4] * tremolo, 0, 63, 0, 63 - tempVol);
      break;  // A OP2

    case 10:
      tempVol = 63 - ((getRandy(106) + opl[currentKit][106]) << 2);
      break;  // A OP3

    case 11:
      tempVol = 63 - ((getRandy(113) + opl[currentKit][113]) << 2);
      tempVol += map(tremDepth[5] * tremolo, 0, 63, 0, 63 - tempVol);
      break;  // A OP4

      //// /// / / / / / / / // / //////// /

    case 12:
      tempVol = 63 - ((getRandy(120) + opl[currentKit][120]) << 2);
      break;  //B OP1

    case 13:
      tempVol = 63 - ((getRandy(127) + opl[currentKit][127]) << 2);
      tempVol += map(tremDepth[6] * tremolo, 0, 63, 0, 63 - tempVol);
      break;  // B OP2

    case 14:
      tempVol = 63 - ((getRandy(134) + opl[currentKit][134]) << 2);
      break;  // B OP3

    case 15:
      tempVol = 63 - ((getRandy(141) + opl[currentKit][141]) << 2);
      tempVol += map(tremDepth[7] * tremolo, 0, 63, 0, 63 - tempVol);
      break;  // B OP4
  }
  volNext[number] = tempVol;
}




void updateFreq() {
  //synth 1
  const float bend0 = midiPitchBendFactor[0];
  float base0 = note2freq[constrainNoteToMode(currentNote[0] + key, musicRoot, musicModes[currentPattern])];
  freqNext[0] = base0 * bend0;
  freqNext[1] = base0 * tuneBase[0] * bend0;
  //synth 2
  const float bend1 = midiPitchBendFactor[1];
  float base1 = note2freq[constrainNoteToMode(currentNote[1] + key, musicRoot, musicModes[currentPattern])];
  freqNext[2] = base1 * bend1;
  freqNext[3] = base1 * tuneBase[1] * bend1;
  //synth 3A
  const float bend2 = midiPitchBendFactor[2];
  float base2a = note2freq[constrainNoteToMode(currentNote[2] + key, musicRoot, musicModes[currentPattern])];
  freqNext[4] = base2a * bend2;
  freqNext[5] = base2a * tuneBase[2] * bend2;
  //synth 3B
  float base2b = note2freq[constrainNoteToMode(currentNote[4] + key, musicRoot, musicModes[currentPattern])];
  freqNext[8] = base2b * bend2;
  freqNext[9] = base2b * tuneBase[2] * bend2;
  //synth 4
  const float bend3 = midiPitchBendFactor[3];
  float base3 = note2freq[constrainNoteToMode(currentNote[3] + key, musicRoot, musicModes[currentPattern])];
  freqNext[6] = base3 * bend3;






  //modulate op4 mult if we're in a 4op algo
  if (algo[0]) {
    multLfo[0] = map(vibrato, 0, 100, 0, 10) * vibratoDepth[1];
  } else {
    multLfo[0] = 0;
  }
  if (algo[1]) {
    multLfo[1] = map(vibrato, 0, 100, 0, 10) * vibratoDepth[3];
  } else {
    multLfo[1] = 0;
  }
  if (algo[2]) {
    multLfo[2] = map(vibrato, 0, 100, 0, 10) * vibratoDepth[5];
  } else {
    multLfo[2] = 0;
  }
  if (algo[3]) {
    multLfo[3] = map(vibrato, 0, 100, 0, 10) * vibratoDepth[7];
  } else {
    multLfo[3] = 0;
  }


  //synth 1

  //vibrato
  freqNext[0] += vibratoDepth[0] * vibrato;

  if (freqNext[0] != freqLast[0]) {
    opl2.setFrequency(0, freqNext[0]);
    freqLast[0] = freqNext[0];
  }

  freqNext[1] += vibratoDepth[1] * vibrato;
  if (freqNext[1] != freqLast[1]) {
    opl2.setFrequency(3, freqNext[1]);
    freqLast[1] = freqNext[1];
  }

  //synth 2
  freqNext[2] += vibratoDepth[2] * vibrato;
  if (freqNext[2] != freqLast[2]) {
    opl2.setFrequency(1, freqNext[2]);
    freqLast[2] = freqNext[2];
  }
  freqNext[3] += vibratoDepth[3] * vibrato;
  if (freqNext[3] != freqLast[3]) {
    opl2.setFrequency(4, freqNext[3]);
    freqLast[3] = freqNext[3];
  }

  //synth 3
  freqNext[4] += vibratoDepth[4] * vibrato;
  if (freqNext[4] != freqLast[4]) {
    opl2.setFrequency(2, freqNext[4]);
    freqLast[4] = freqNext[4];
  }
  freqNext[5] += vibratoDepth[5] * vibrato;
  if (freqNext[5] != freqLast[5]) {
    opl2.setFrequency(5, freqNext[5]);
    freqLast[5] = freqNext[5];
  }

  ///second voice3
  freqNext[8] += vibratoDepth[4] * vibrato;
  if (freqNext[8] != freqLast[8]) {
    opl2.setFrequency(10, freqNext[8]);
    freqLast[8] = freqNext[8];
  }

  freqNext[9] += vibratoDepth[5] * vibrato;

  if (freqNext[9] != freqLast[9]) {
    opl2.setFrequency(13, freqNext[9]);
    freqLast[9] = freqNext[9];
  }

  //synth 4
  freqNext[7] = freqNext[6];
  freqNext[6] += vibratoDepth[6] * vibrato;
  freqNext[7] += vibratoDepth[7] * vibrato;


  if (freqNext[6] != freqLast[6]) {
    opl2.setFrequency(9, freqNext[6]);
    opl2.setFrequency(12, freqNext[6]);
    opl2.setFrequency(11, freqNext[7] * fatMultipliers[fat]);
    opl2.setFrequency(14, freqNext[7] * fatMultipliers[fat]);
    freqLast[6] = freqNext[6];
  }
}



bool getStep(uint8_t voice, int16_t stepNumber) {
  stepNumber = constrain(stepNumber, 0, 63);
  bool temp = 0;
  switch (voice) {
    case 0: temp = bitRead(pattern[currentPattern][0][stepNumber], 6); break;
    case 1: temp = bitRead(pattern[currentPattern][0][stepNumber], 7); break;
    case 2: temp = bitRead(pattern[currentPattern][1][stepNumber], 0); break;
    case 3: temp = bitRead(pattern[currentPattern][1][stepNumber], 1); break;
  }
  return (temp);
}


bool getStep2(int16_t stepNumber) {
  stepNumber = constrain(stepNumber, 0, 63);
  return (bitRead(pattern[currentPattern][1][stepNumber], 2));
}


void setStep(uint8_t voice, int16_t stepNumber, bool value) {

  stepNumber = constrain(stepNumber, 0, 63);
  bool changed = (getStep(voice, stepNumber) != value);
  switch (voice) {
    case 0: bitWrite(pattern[currentPattern][0][stepNumber], 6, value); break;
    case 1: bitWrite(pattern[currentPattern][0][stepNumber], 7, value); break;
    case 2: bitWrite(pattern[currentPattern][1][stepNumber], 0, value); break;
    case 3: bitWrite(pattern[currentPattern][1][stepNumber], 1, value); break;
  }
  if (changed) {
    markPatternVisualizerDirty();
  }
}

void setStep2(int16_t stepNumber, bool value) {
  stepNumber = constrain(stepNumber, 0, 63);
  bool changed = (bitRead(pattern[currentPattern][1][stepNumber], 2) != static_cast<uint8_t>(value));
  bitWrite(pattern[currentPattern][1][stepNumber], 2, value);
  if (changed) {
    markPatternVisualizerDirty();
  }
}



uint8_t getNote2(uint8_t stepNumber) {

  stepNumber = constrain(stepNumber, 0, 63);

  uint8_t temp = 0;

  bitWrite(temp, 0, bitRead(synth[currentPattern][3][stepNumber], 0));
  bitWrite(temp, 1, bitRead(synth[currentPattern][3][stepNumber], 1));
  bitWrite(temp, 2, bitRead(synth[currentPattern][3][stepNumber], 2));
  bitWrite(temp, 3, bitRead(synth[currentPattern][3][stepNumber], 3));
  bitWrite(temp, 4, bitRead(synth[currentPattern][3][stepNumber], 4));
  bitWrite(temp, 5, bitRead(synth[currentPattern][3][stepNumber], 5));

  return (temp);
}

uint8_t getNote(uint8_t voice, int16_t stepNumber) {

  stepNumber = constrain(stepNumber, 0, 63);


  uint8_t temp = 0;

  switch (voice) {

    case 0:
      bitWrite(temp, 0, bitRead(synth[currentPattern][0][stepNumber], 0));
      bitWrite(temp, 1, bitRead(synth[currentPattern][0][stepNumber], 1));
      bitWrite(temp, 2, bitRead(synth[currentPattern][0][stepNumber], 2));
      bitWrite(temp, 3, bitRead(synth[currentPattern][0][stepNumber], 3));
      bitWrite(temp, 4, bitRead(synth[currentPattern][0][stepNumber], 4));
      bitWrite(temp, 5, bitRead(synth[currentPattern][0][stepNumber], 5));
      break;

    case 1:
      bitWrite(temp, 0, bitRead(synth[currentPattern][0][stepNumber], 6));
      bitWrite(temp, 1, bitRead(synth[currentPattern][0][stepNumber], 7));
      bitWrite(temp, 2, bitRead(synth[currentPattern][1][stepNumber], 0));
      bitWrite(temp, 3, bitRead(synth[currentPattern][1][stepNumber], 1));
      bitWrite(temp, 4, bitRead(synth[currentPattern][1][stepNumber], 2));
      bitWrite(temp, 5, bitRead(synth[currentPattern][1][stepNumber], 3));

      break;

    case 2:
      bitWrite(temp, 0, bitRead(synth[currentPattern][1][stepNumber], 4));
      bitWrite(temp, 1, bitRead(synth[currentPattern][1][stepNumber], 5));
      bitWrite(temp, 2, bitRead(synth[currentPattern][1][stepNumber], 6));
      bitWrite(temp, 3, bitRead(synth[currentPattern][1][stepNumber], 7));
      bitWrite(temp, 4, bitRead(synth[currentPattern][2][stepNumber], 0));
      bitWrite(temp, 5, bitRead(synth[currentPattern][2][stepNumber], 1));

      break;

    case 3:
      bitWrite(temp, 0, bitRead(synth[currentPattern][2][stepNumber], 2));
      bitWrite(temp, 1, bitRead(synth[currentPattern][2][stepNumber], 3));
      bitWrite(temp, 2, bitRead(synth[currentPattern][2][stepNumber], 4));
      bitWrite(temp, 3, bitRead(synth[currentPattern][2][stepNumber], 5));
      bitWrite(temp, 4, bitRead(synth[currentPattern][2][stepNumber], 6));
      bitWrite(temp, 5, bitRead(synth[currentPattern][2][stepNumber], 7));
      break;
  }

  return (temp);
}

void setNote(uint8_t voice, uint8_t stepNumber, uint8_t value) {

  stepNumber = constrain(stepNumber, 0, 63);
  const uint8_t maskedValue = static_cast<uint8_t>(value & 0x3F);
  const bool changed = (getNote(voice, stepNumber) != maskedValue);

  if (!bend) {
    switch (voice) {

      case 0:
        bitWrite(synth[currentPattern][0][stepNumber], 0, bitRead(maskedValue, 0));
        bitWrite(synth[currentPattern][0][stepNumber], 1, bitRead(maskedValue, 1));
        bitWrite(synth[currentPattern][0][stepNumber], 2, bitRead(maskedValue, 2));
        bitWrite(synth[currentPattern][0][stepNumber], 3, bitRead(maskedValue, 3));
        bitWrite(synth[currentPattern][0][stepNumber], 4, bitRead(maskedValue, 4));
        bitWrite(synth[currentPattern][0][stepNumber], 5, bitRead(maskedValue, 5));
        break;

      case 1:
        bitWrite(synth[currentPattern][0][stepNumber], 6, bitRead(maskedValue, 0));
        bitWrite(synth[currentPattern][0][stepNumber], 7, bitRead(maskedValue, 1));
        bitWrite(synth[currentPattern][1][stepNumber], 0, bitRead(maskedValue, 2));
        bitWrite(synth[currentPattern][1][stepNumber], 1, bitRead(maskedValue, 3));
        bitWrite(synth[currentPattern][1][stepNumber], 2, bitRead(maskedValue, 4));
        bitWrite(synth[currentPattern][1][stepNumber], 3, bitRead(maskedValue, 5));
        break;

      case 2:
        bitWrite(synth[currentPattern][1][stepNumber], 4, bitRead(maskedValue, 0));
        bitWrite(synth[currentPattern][1][stepNumber], 5, bitRead(maskedValue, 1));
        bitWrite(synth[currentPattern][1][stepNumber], 6, bitRead(maskedValue, 2));
        bitWrite(synth[currentPattern][1][stepNumber], 7, bitRead(maskedValue, 3));
        bitWrite(synth[currentPattern][2][stepNumber], 0, bitRead(maskedValue, 4));
        bitWrite(synth[currentPattern][2][stepNumber], 1, bitRead(maskedValue, 5));
        break;

      case 3:
        bitWrite(synth[currentPattern][2][stepNumber], 2, bitRead(maskedValue, 0));
        bitWrite(synth[currentPattern][2][stepNumber], 3, bitRead(maskedValue, 1));
        bitWrite(synth[currentPattern][2][stepNumber], 4, bitRead(maskedValue, 2));
        bitWrite(synth[currentPattern][2][stepNumber], 5, bitRead(maskedValue, 3));
        bitWrite(synth[currentPattern][2][stepNumber], 6, bitRead(maskedValue, 4));
        bitWrite(synth[currentPattern][2][stepNumber], 7, bitRead(maskedValue, 5));
        break;
    }
    if (changed) {
      markPatternVisualizerDirty();
    }
  }
}

void setNote2(uint8_t stepNumber, uint8_t value) {

  stepNumber = constrain(stepNumber, 0, 63);
  const uint8_t maskedValue = static_cast<uint8_t>(value & 0x3F);
  const bool changed = (getNote2(stepNumber) != maskedValue);

  if (!bend) {

    bitWrite(synth[currentPattern][3][stepNumber], 0, bitRead(maskedValue, 0));
    bitWrite(synth[currentPattern][3][stepNumber], 1, bitRead(maskedValue, 1));
    bitWrite(synth[currentPattern][3][stepNumber], 2, bitRead(maskedValue, 2));
    bitWrite(synth[currentPattern][3][stepNumber], 3, bitRead(maskedValue, 3));
    bitWrite(synth[currentPattern][3][stepNumber], 4, bitRead(maskedValue, 4));
    bitWrite(synth[currentPattern][3][stepNumber], 5, bitRead(maskedValue, 5));
    if (changed) {
      markPatternVisualizerDirty();
    }
  }
}





void setAlgo(uint8_t voice, int16_t data) {
  data = constrain(data, 0, 4);

  if (voice == 0) {
    if ((data == 0) || (data == 1) || (data == 3)) {
      algo[0] = 1;
    } else {
      algo[0] = 0;
    }
  }
  if (voice == 1) {
    if ((data == 0) || (data == 1) || (data == 3)) {
      algo[1] = 1;
    } else {
      algo[1] = 0;
    }
  }
  if (voice == 10) {
    if ((data == 0) || (data == 1) || (data == 3)) {
      algo[2] = 1;
    } else {
      algo[2] = 0;
    }
  }
  if (voice == 11) {
    if ((data == 0) || (data == 1) || (data == 3)) {
      algo[3] = 1;
    } else {
      algo[3] = 0;
    }
  }
  switch (data) {
    case 0:  //algo 1
      if (voice == 0) {
        bitWrite(op4Mode, 0, 1);
      } else if (voice == 1) {
        bitWrite(op4Mode, 1, 1);
      } else if (voice == 10) {
        bitWrite(op4Mode, 4, 1);
        bitWrite(op4Mode, 2, 1);
      } else if (voice == 11) {
        bitWrite(op4Mode, 5, 1);
        bitWrite(op4Mode, 3, 1);
      }

      opl2.setSynthMode(voice, 1);
      opl2.setCnt(voice, 0);
      opl2.setCnt(voice + 3, 0);
      break;
    case 1:  //algo 2
      if (voice == 0) {
        bitWrite(op4Mode, 0, 1);
      } else if (voice == 1) {
        bitWrite(op4Mode, 1, 1);
      } else if (voice == 10) {
        bitWrite(op4Mode, 4, 1);
        bitWrite(op4Mode, 2, 1);
      } else if (voice == 11) {
        bitWrite(op4Mode, 5, 1);
        bitWrite(op4Mode, 3, 1);
      }

      opl2.setSynthMode(voice, 1);
      opl2.setCnt(voice, 1);
      opl2.setCnt(voice + 3, 0);
      break;

    case 2:  //algo 3

      if (voice == 0) {
        bitWrite(op4Mode, 0, 1);
      } else if (voice == 1) {
        bitWrite(op4Mode, 1, 1);
      } else if (voice == 10) {
        bitWrite(op4Mode, 4, 1);
        bitWrite(op4Mode, 2, 1);
      } else if (voice == 11) {
        bitWrite(op4Mode, 5, 1);
        bitWrite(op4Mode, 3, 1);
      }

      opl2.setSynthMode(voice, 0);

      opl2.setCnt(voice, 0);
      opl2.setCnt(voice + 3, 1);
      break;
    case 3:  //algo 4
      if (voice == 0) {
        bitWrite(op4Mode, 0, 1);
      } else if (voice == 1) {
        bitWrite(op4Mode, 1, 1);
      } else if (voice == 10) {
        bitWrite(op4Mode, 4, 1);
        bitWrite(op4Mode, 2, 1);
      } else if (voice == 11) {
        bitWrite(op4Mode, 5, 1);
        bitWrite(op4Mode, 3, 1);
      }

      opl2.setSynthMode(voice, 1);
      opl2.setCnt(voice, 1);
      opl2.setCnt(voice + 3, 1);
      break;
    case 4:  //algo 5

      if (voice == 0) {
        bitWrite(op4Mode, 0, 0);
      } else if (voice == 1) {
        bitWrite(op4Mode, 1, 0);
      } else if (voice == 10) {
        bitWrite(op4Mode, 4, 0);
        bitWrite(op4Mode, 2, 0);
      } else if (voice == 11) {
        bitWrite(op4Mode, 5, 0);
        bitWrite(op4Mode, 3, 0);
      }

      opl2.setSynthMode(voice, 0);
      opl2.setCnt(voice, 1);
      opl2.setCnt(voice + 3, 1);
      break;
  }

  opl2.set4Op(op4Mode);
}

void killSynths() {
  if (!mute[6]) {
    heldNotes[0] = 0;
    opl2.setKeyOn(0, 0);
    opl2.setKeyOn(3, 0);
  }
  if (!mute[7]) {
    heldNotes[1] = 0;
    opl2.setKeyOn(1, 0);
    opl2.setKeyOn(4, 0);
  }
  if (!mute[8]) {
    heldNotes[2] = 0;
    opl2.setKeyOn(2, 0);
    opl2.setKeyOn(5, 0);
    opl2.setKeyOn(10, 0);
    opl2.setKeyOn(13, 0);
  }
  if (!mute[9]) {
    heldNotes[3] = 0;
    opl2.setKeyOn(9, 0);
    opl2.setKeyOn(12, 0);
    opl2.setKeyOn(11, 0);
    opl2.setKeyOn(14, 0);
  }
}

//how many synth bottom buttons are held (used for second track of synth3)
//
int16_t synthHeld;

void triggerSynthWithButton(uint8_t number, bool value) {

  uint8_t voice = selectedVoice - 6;


  //record the note.
  if ((value) && (rec)) {

    if (voice != 2) {
      //mono voices
      setStep(voice, targetStep, 1);
      setNudgeAtStep(selectedVoice);
      setNote(voice, targetStep, number + punchOffset);

      //trigger the note


      triggerSynth(selectedVoice - 6, 0, number + punchOffset, 1);

      if ((!run) && (stepRec)) { nextTargetStep(); }
    } else {
      //duo voice
      if (synthHeld == 0) {
        //first keyPress overWrite channel1
        setStep(voice, targetStep, 1);
        setNudgeAtStep(selectedVoice);
        setStep2(targetStep, 0);
        setNote(voice, targetStep, number + punchOffset);

        triggerSynth(selectedVoice - 6, 0, number + punchOffset, 1);

      } else {
        //second and consecutive keyPress overWrite channel2
        setStep2(targetStep, 1);
        setNote2(targetStep, number + punchOffset);

        triggerSynth(selectedVoice - 6, 0, number + punchOffset, 1);
        if ((!run) && (stepRec)) { nextTargetStep(); }
      }
    }
  }



  //synth monitor mode
  if ((value) && (!rec) && (monitorSynth)) {

    triggerSynth(selectedVoice - 6, 0, number + punchOffset, 1);
  }


  //toggle synth steps
  if ((!monitorSynth) && (value) && (!rec) && (!topHeldCounter)) {


    //turn step on
    if (!getStep(selectedVoice - 6, number + x16[selectedPage])) {
      setStep(selectedVoice - 6, number + x16[selectedPage], 1);
    } else {
      //turn step off
      setStep(selectedVoice - 6, number + x16[selectedPage], 0);
      //(on both tracks if synth 3 4)
      if (selectedVoice == 8) setStep2(number + x16[selectedPage], 0);
    }

    if (!run) { triggerSynth(selectedVoice - 6, 0, getNote(voice, number + x16[selectedPage]), 1); }
  }

  showPattern();


  if (value) {
    synthHeld++;
  } else {
    synthHeld--;
    if (synthHeld < 1) {
      synthHeld = 0;
      //advance step in stepRec if last key is lifted for voice3
      if ((voice == 2) && (rec) && (!run) && (stepRec)) { nextTargetStep(); }
    }
  }
}



void triggerSynthWithMidi(uint8_t voice, uint8_t note, uint8_t velocity) {
  uint8_t oct = 0;
  while (note < 36) { note += 12; }
  while (note > 99) {
    note -= 12;
    oct++;
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////                        NOTE ON                                        /////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (velocity) {
    //punch to step
    if (plocStep) {
      setStep(voice, plocStep - 1 + x16[selectedPage], 1);
      setNote(voice, plocStep - 1 + x16[selectedPage], note - 36);
      triggerSynth(voice, oct, note - 36, 1);
      plocked = true;
      showPattern();
    } else if ((rec) && (run)) {
      if (voice != 2) {
        //mono voices
        setStep(voice, targetStep, 1);
        setNudgeAtStep(voice + 6);
        setNote(voice, targetStep, note - 36);
      } else {
        //duo voice
        if (heldNotes[voice] == 0) {
          //first keyPress overWrite channel1
          setStep(voice, targetStep, 1);
          setNudgeAtStep(voice + 6);
          setStep2(targetStep, 0);
          setNote(voice, targetStep, note - 36);
        } else {
          //second and consecutive keyPress overWrite channel2
          setStep2(targetStep, 1);
          setNote2(targetStep, note - 36);
        }
      }
      //trigger the note if we are already on the step we punched to FIXME (might triggerregarless)
      triggerSynth(voice, oct, note - 36, 1);
    }  //stepRec punch
    else if ((rec) && (!run) && (stepRec)) {

      if (voice != 2) {
        //mono voices
        setStep(voice, targetStep, 1);
        setNudgeAtStep(voice + 6);
        setNote(voice, targetStep, note - 36);
        //advance to next step on keyDown for mono voices
        nextTargetStep();
      } else {
        //duo voice
        if (heldNotes[voice] == 0) {
          //first keyPress overWrite channel1
          setStep(voice, targetStep, 1);
          setNudgeAtStep(voice + 6);
          setStep2(targetStep, 0);
          setNote(voice, targetStep, note - 36);
        } else {
          //second and consecutive keyPress overWrite channel2
          setStep2(targetStep, 1);
          setNote2(targetStep, note - 36);
        }
      }
      //trigger the note
      triggerSynth(voice, oct, note - 36, 1);
    } else {
      //monitor mode just trigger
      triggerSynth(voice, oct, note - 36, 1);
    }
    heldNotes[voice]++;
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////                        NOTE OFF                                        /////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  else {
    if (heldNotes[voice]) {
      heldNotes[voice]--;
      if (!heldNotes[voice]) {
        //advance step in stepRec if last key is lifted for voice3
        if ((voice == 2) && (rec) && (!run) && (stepRec)) { nextTargetStep(); }
        heldNotes[voice] = 0;
        triggerSynth(voice, oct, note - 36, 0);
      }
    }
  }
}

void triggerSynth(uint8_t voice, uint8_t oct, uint8_t note, bool on) {
  lastNotePlayed[voice] = note;
  lastOctPlayed[voice] = oct;
  //blink the top led!
  if ((on) && (visu)) flipLed(voice + 22);
  note += 36;
  note += (offsetOct[currentPattern][voice + 6]) * 12;
  const bool allowMidiOut = !suppressIncomingMidiForwarding;
  switch (voice) {
    case 0:
      if (on) {
        if (allowMidiOut) {
          if (midiTimeout[6]) { sendNoteOff(midiLastNoteSynth[0], 127, ch1); }
          midiHotSynth[0] = (12 * oct) + note;
        }
        currentNote[0] = note;
      }
      break;
    case 1:
      if (on) {
        if (allowMidiOut) {
          if (midiTimeout[7]) { sendNoteOff(midiLastNoteSynth[1], 127, ch2); }
          midiHotSynth[1] = (12 * oct) + note;
        }
        currentNote[1] = note;
      }
      break;
    case 2:
      if (on) {
        //toggle between voices 2/5 and 10/13
        voice3Alternator = !voice3Alternator;
        if (voice3Alternator) {
          currentNote[4] = note;

          if (allowMidiOut) {
            if (midiTimeout[8]) { sendNoteOff(midiLastNoteSynth[2], 127, ch3); }
            midiHotSynth[2] = (12 * oct) + note;
          }

        } else {
          if (allowMidiOut) {
            if (midiTimeout[10]) { sendNoteOff(midiLastNoteSynth[5], 127, ch3); }
            midiHotSynth[4] = (12 * oct) + note;
          }
          currentNote[2] = note;
        }
      }
      break;

    case 3:
      if (on) {
        if (allowMidiOut) {
          if (midiTimeout[9]) { sendNoteOff(midiLastNoteSynth[4], 127, ch4); }
          midiHotSynth[3] = (12 * oct) + note;
        }
        currentNote[3] = note;
      }
      break;
  }



  if (on) {
    //retrigger?
    switch (voice) {

      case 0:

        synthHotOffOn[0] = 1;
        synthHotOffOn[3] = 1;
        break;

      case 1:

        synthHotOffOn[1] = 1;
        synthHotOffOn[4] = 1;
        break;


      case 2:

        if (!voice3Alternator) {

          synthHotOffOn[2] = 1;
          synthHotOffOn[5] = 1;
        } else {

          synthHotOffOn[10] = 1;
          synthHotOffOn[13] = 1;
        }
        break;

      case 3:

        synthHotOffOn[9] = 1;
        synthHotOffOn[12] = 1;
        synthHotOffOn[11] = 1;
        synthHotOffOn[14] = 1;

        break;
    }


  }

  else {

    switch (voice) {
      case 0:

        synthHotOff[0] = 1;
        synthHotOff[3] = 1;
        break;

      case 1:

        synthHotOff[1] = 1;
        synthHotOff[4] = 1;
        break;


      case 2:
        if (voice3Alternator) {

          synthHotOff[10] = 1;
          synthHotOff[13] = 1;
        } else {


          synthHotOff[2] = 1;
          synthHotOff[5] = 1;
        }
        break;


      case 3:

        synthHotOff[9] = 1;
        synthHotOff[12] = 1;
        synthHotOff[11] = 1;
        synthHotOff[14] = 1;

        break;
    }
  }
}
