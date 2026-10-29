/*


  EEPROM MAPPING:

  1 Mix Vol
  2 virginChip
  3 routing[0]
  4 routing[1]
  5 routing[2]
  6 routing[3]
  7 routing[4]
  8 safe

  9 MIDI CH 10 (drums)
  10 MIDI CH1
  11 MIDI CH2
  12 MIDI CH3
  13 MIDI CH4



  15 pitchFallRate (1=1 0=10)

  16 lastSong
  17 lastFolder

  18 routing[5]
  19 routing[6]

  20 Phones Vol

  21 stutterDefault


  22 stepRec

23 metronome


  14 saveKitAlso

25 lastPattern

26/27/28 tempo

29 quantise



MIDI NOTES
30 1 BD
31 2 SD
32 3 TOM
33 4 CH
34 5 OH
35 6 CYM
36 wiggleOnEnabledStep
37 ignore midi notes 
38 restartMode 
39 routingPreset magic Number = should be 123 or reset presets to default.
40-109 routing preset
110 = routing preset number;
111=sd reset - don't touch
112= chase all pages

113=r3

114=OnePpq
115=reserved
116=MIDI mutes channel
117=MIDI solos channel
118=MIDI program change channel
*/

#include <stddef.h>
#include "globals.h"

constexpr uint8_t EEPROM_ADDR_CH_MUTES = 116;
constexpr uint8_t EEPROM_ADDR_CH_SOLOS = 117;
constexpr uint8_t EEPROM_ADDR_CH_PROGRAM = 118;

void clearSong() {

  loadingDefault = true;
  load();
  loadingDefault = false;
}
void checkCard() {
  // The STM32F405 SPI divider turns this 12 MHz request into about 10.5 MHz.
  // That is twice the ultra-conservative setting, but still half the actual
  // clock used by the former marginal 24 MHz configuration.
#if defined(STM32F1) || defined(STM32F4)
  static const uint32_t kSdClockHz = 12000000;
#endif
  static const uint8_t kMaxAttempts = 10;
  startSD();

  sdCardReady = false;
  digitalWrite(SD_CS_PIN, HIGH);
  delay(250);  // allow slow cards to complete internal power-up

  for (uint8_t attempt = 0; attempt < kMaxAttempts; ++attempt) {
#if defined(STM32F1) || defined(STM32F4)
    SD.end();
#endif
    digitalWrite(SD_CS_PIN, HIGH);
    delay(20 + static_cast<uint16_t>(attempt) * 20);
#if defined(STM32F1) || defined(STM32F4)
    sdCardReady = SD.begin(kSdClockHz, SD_CS_PIN);
#else
    sdCardReady = SD.begin(SD_CS_PIN);
#endif
    if (sdCardReady) break;
  }
  endSD();
}





void factorySettings() {

#if defined(STM32F1) || defined(STM32F4)  // Example macros for STM32 boards
  for (int16_t i = 0; i < 2048; i++) {
    EEPROMArray[i] = 0;
  }
#endif

  //tempo
  EEPROMupdate(26, 240);
  EEPROMupdate(27, 0);

  EEPROMupdate(15, 0);
  pitchFallRate = fallRateFast;

  EEPROMupdate(16, 0);
  currentSong = 0;
  EEPROMupdate(17, 0);
  currentFolder = 0;

  EEPROMupdate(20, 5);
  phonesVol = 5;
  EEPROMupdate(1, 5);
  mixVol = 5;

  EEPROMupdate(3, B11110000);
  EEPROMupdate(4, B11110000);
  EEPROMupdate(5, B11110000);
  EEPROMupdate(6, B11110000);
  EEPROMupdate(7, B11110000);
  EEPROMupdate(18, B11110000);
  EEPROMupdate(19, B11110000);

  routing[0] = routing[1] = routing[2] = routing[3] = routing[4] = routing[5] = routing[6] = B11110000;
  updateRouting();

  EEPROMupdate(9, 10);
  EEPROMupdate(10, 1);
  EEPROMupdate(11, 2);
  EEPROMupdate(12, 3);
  EEPROMupdate(13, 4);
  EEPROMupdate(EEPROM_ADDR_CH_MUTES, 16);
  EEPROMupdate(EEPROM_ADDR_CH_SOLOS, 16);
  EEPROMupdate(EEPROM_ADDR_CH_PROGRAM, 16);


  for (int16_t i = 0; i < 6; i++) { EEPROMupdate(30 + i, drumNotesFactory[i]); }

  ch10 = 10;  //default
  ch1 = 1;    //defaultif(ch10>16)ch10=16//default
  ch2 = 2;    //default
  ch3 = 3;    //default
  ch4 = 4;    //default
  chMuteMidi = 16;
  chSoloMidi = 16;
  chProgramChange = 16;

  EEPROMupdate(8, 1);
  safe = true;

  EEPROMupdate(22, 0);  //stepRec

  EEPROMupdate(23, 0);  //metro

}

void loadSettings() {
  // Ensure sane defaults if EEPROM contents have not been initialised yet.
  if (EEPROMread(2) != 123) {
    factorySettings();
  }

  OnePpq = EEPROMread(114) == 1;

  tempo = EEPROMread(26) + (EEPROMread(27) << 8);
  uClock.setTempo(tempo);
  loadRouting();
  updateRouting();


  if (EEPROMread(15)) {
    pitchFallRate = fallRateSlow;
  } else {
    pitchFallRate = fallRateFast;
  }

  currentSong = EEPROMread(16);
  if (currentSong > 15) currentSong = 0;
  currentFolder = EEPROMread(17);
  if (currentFolder > 9) currentFolder = 0;


  phonesVol = EEPROMread(20);
  if (phonesVol > 9) {
    phonesVol = 5;  //phones
  }
  mixVol = EEPROMread(1);
  if (mixVol > 9) {
    mixVol = 5;  //mix
  }

  ch10 = EEPROMread(9);
  if (ch10 > 16) ch10 = 10;  //default
  ch1 = EEPROMread(10);
  if (ch1 > 16) ch1 = 1;  //defaultif(ch10>16)ch10=16//default
  ch2 = EEPROMread(11);
  if (ch2 > 16) ch2 = 2;  //default
  ch3 = EEPROMread(12);
  if (ch3 > 16) ch3 = 3;  //default
  ch4 = EEPROMread(13);
  if (ch4 > 16) ch4 = 4;  //default
  chMuteMidi = EEPROMread(EEPROM_ADDR_CH_MUTES);
  if (!chMuteMidi || chMuteMidi > 16) chMuteMidi = 16;
  chSoloMidi = EEPROMread(EEPROM_ADDR_CH_SOLOS);
  if (!chSoloMidi || chSoloMidi > 16) chSoloMidi = 16;
  chProgramChange = EEPROMread(EEPROM_ADDR_CH_PROGRAM);
  if (!chProgramChange || chProgramChange > 16) chProgramChange = 16;

  if (EEPROMread(8)) {
    safe = true;
  } else {
    safe = false;
  }

  if (EEPROMread(22)) {
    stepRec = 1;
  } else {
    stepRec = 0;
  }

  if (EEPROMread(23)) {
    metro = 1;
  } else {
    metro = 0;
  }

  if (EEPROMread(36)) {
    wiggleOnEnabledStep = 1;
  } else {
    wiggleOnEnabledStep = 0;
  }

  if (EEPROMread(14)) {
    saveKitAlso = 1;
  } else {
    saveKitAlso = 0;
  }

  if (EEPROMread(37)) {
    ignoreNotes = 1;
  } else {
    ignoreNotes = 0;
  }

  if (EEPROMread(29)) {
    quantise = 0;
  } else {
    quantise = 1;
  }

  chaseAllPages = EEPROMread(112);

  for (int16_t i = 0; i < 6; i++) {
    drumNotes[i] = EEPROMread(30 + i);
    if (!drumNotes[i]) { drumNotes[i] = drumNotesFactory[i]; }
  }

  r3 = (EEPROMread(113) > 0) ? 1 : 0;
}


uint8_t lowNibble(uint8_t input) {
  return (input & B00001111);
}

uint8_t highNibble(uint8_t input) {
  return (input >> 4);
}


// Push a single byte onto the write buffer and flush on 512-byte page boundaries.
void buffer(uint8_t data) {
  buffy[buffCounter++] = data;
  if (buffCounter == sizeof(buffy)) {
    fillPage();
  }
}

// Write any pending data in the buffer to the SD card.
void fillPage() {
  if (!buffCounter) return;
  const size_t pendingBytes = static_cast<size_t>(buffCounter);
  if (!sdFile || sdFile.write(buffy, pendingBytes) != pendingBytes) {
    sdWriteOk = false;
  }
  buffCounter = 0;
}

// Bulk helpers keep the hot loop inside buffer()/fillPage().
inline void bufferBytes(const uint8_t* data, size_t len) {
  while (len--) buffer(*data++);
}

inline void bufferFill(uint8_t value, size_t count) {
  while (count--) buffer(value);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t getSd() {

  if (sdFile.available()) {
    return (sdFile.read());
  } else {
    return (0);
  }
}

void clearPattern() {

  clearNudges();

  //change  to 1 last step to 16
  //clear all synth heldNote
  //clear all steps;
  //

  vibratoRateMultiplier[currentPattern] = 4;
  tremRateMultiplier[currentPattern] = 4;
  vibratoPower[currentPattern] = 1;

  tremRate[currentPattern] = 4;
  vibratoRate[currentPattern] = 4;

  for (int16_t i = 0; i < 64; i++) {
    for (int16_t j = 0; j < 5; j++) {
      nudgeDelay[currentPattern][j][i] = 0;
    }
  }

  for (int16_t j = 0; j < 10; j++) {
    lastStepPoly[currentPattern][i] = 15;
    pages[currentPattern][j] = 0;
    for (int16_t i = 0; i < 64; i++) {
      if (j < 6) {
        bitWrite(pattern[currentPattern][0][i], j, 0);
      } else {
        setStep(j - 6, i, 0);
      }
    }
  }



  //reset synth notes and STEPS
  for (int16_t i = 0; i < 4; i++) {
    for (int16_t j = 0; j < 64; j++) {
      setNote(i, j, 12);
      if (i == 0) {
        setNote2(j, 12);
        setStep2(j, 0);
      }
    }
  }
  wipe();
  for (int16_t i = 0; i < 10; i++) {
    offsetOct[currentPattern][i] = 1;
    prob[kit[currentPattern]][i] = 0;
  }
}

static inline void enterSdLedQuietMode() {
  if (sdLedsMuted) {
    return;
  }
  sdLedsMuted = true;
  if (!ledsReady) {
    return;
  }
  mx.clear();
  mx.setPoint(1, 3, 1);  // LED_rec
}

#if defined(__MK20DX256__)
uint32_t freeRam() {
  uint32_t stackTop;
  uint32_t heapTop;

  stackTop = (uint32_t)&stackTop;
  void* heapProbe = malloc(1);
  heapTop = (uint32_t)heapProbe;
  free(heapProbe);

  return stackTop - heapTop;
}
#endif

void startSD() {
  // nested-safe: only reconfigure hardware on first entry
  if (sdSessionDepth++ > 0) {
    return;
  }
  enterSdLedQuietMode();

#if defined(__MK20DX256__)  // Code specific to Teensy 3.1/3.2



  tickTimer.end();

  SPI.begin();
  SPI.setMOSI(7);  // Audio shield has MOSI on pin 7
  SPI.setMISO(8);  // Audio shield has MOSI on pin 7
  SPI.setSCK(13);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

#elif defined(STM32F1) || defined(STM32F4)  // Code specific to STM32
  if (sdTimerPrimed) {
    ITimer0.stopTimer();
  }

  //disables the display driver and enables SD

  SPI.begin();
  SPI.setMOSI(PA7);
  SPI.setMISO(PA6);
  // The SD library owns its MODE0 transactions. Do not wrap the whole session
  // in a conflicting MODE3 transaction.
#endif
}

void endSD() {
  // nested-safe: only release hardware on final exit
  if (sdSessionDepth == 0) {
    return;
  }
  if (--sdSessionDepth > 0) {
    return;
  }
#if defined(__MK20DX256__)  // Code specific to Teensy 3.1/3.2

  if (freeRam() < 6000) {
    EEPROMupdate(111, startSkip);
    CPU_RESTART;
  }


  SPI.endTransaction();
  SPI.end();
  SPI.begin();
  mx.begin();
  mx.control(BlastLedMatrix::INTENSITY, brightness);
  sdLedsMuted = false;
  showMode();
  tickTimer.begin(SysTick_Handler, ISRCLOCK);
#elif defined(STM32F1) || defined(STM32F4)  // Code specific to STM32
  mx.begin();
  mx.control(BlastLedMatrix::INTENSITY, brightness);
  sdLedsMuted = false;
  showMode();
  if (sdTimerPrimed) {
    ITimer0.restartTimer();
  }
#endif
}
