
#define SERIAL_RX_BUFFER_SIZE 256
#define SERIAL_TX_BUFFER_SIZE 256

#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include "eeprom_utils.h"
#include "globals.h"

#include "_generated_prototypes.h"
#include "automation.h"
#include "buttons.h"
#include "buttons_function.h"
#include "buttons_kit.h"
#include "buttons_step.h"
#include "bb_clock.h"
#include "engine.h"
#include "fader.h"
#include "keyChange.h"
#include "leds.h"
#include "loop.h"
#include "mem_load.h"
#include "mem_save.h"
#include "memory.h"
#include "midi_engine.h"
#include "mux.h"
#include "pattern.h"
#include "pattern_visualizer.h"
#include "seq.h"
#include "synth.h"
#include "undoRedo.h"
#include "vgmSt.h"
#include "vgmTeensy.h"
#include "volume.h"
#include "ISR.h"
#if defined(USBD_USE_MIDI)
#include "stm32/usb/usbd_midi_if.h"
#endif

void boot();

void setup() {


#if defined(__MK20DX256__)  // Code specific to Teensy 3.1/3.2

  startSkip = EEPROM.read(111);

  pinMode(23, INPUT_PULLUP);  
  delay(1);
  earlyProto = (!digitalRead(23));

  Serial.begin(115200);

#if defined(USB_SERIAL) || defined(USB_MIDI_SERIAL) || defined(USB_SERIAL_HID) || defined(USB_MIDI_AUDIO_SERIAL)
  const uint32_t serial_wait_start = millis();
  while (!Serial && (millis() - serial_wait_start) < 2000) {
    // Wait for the USB CDC connection so debug prints show up on the host.
  }
#endif

  uClock.setDrift(0);
  analogReadRes(12);
  pinMode(RUN_PIN, INPUT_PULLUP);  //boot play

#elif defined(STM32F1) || defined(STM32F4)  // Code specific to STM32

  //SDA/SCL
  pinMode(PB6, OUTPUT);
  pinMode(PB7, OUTPUT);
  pinMode(RUN_PIN, INPUT_PULLUP);  // play
  analogReadResolution(12);
#if defined(USBD_USE_MIDI)
  USBMIDI_init();
#endif

  // Give immediate power-on feedback before the SD-backed EEPROM load. The SD
  // session keeps REC lit without further LED SPI traffic while the card is busy.
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  mx.begin();
  mx.control(BlastLedMatrix::INTENSITY, brightness);
  mx.clear();
  mx.setPoint(1, 3, 1);  // LED_rec
  ledsReady = true;

  // Restore persisted settings from SD-backed EEPROM mirror.
  EEPROMLoad();
#endif

  Serial1.begin(31250);  //MIDI

  pinMode(FM_RESET_PIN, OUTPUT);  //FM RESET
  pinMode(FM_CS_PIN, OUTPUT);     //FM-CS
  pinMode(A0_PIN, OUTPUT);        //A0
  pinMode(A1_PIN, OUTPUT);        //A1
  pinMode(FM_RW_PIN, OUTPUT);     //FM-RW
  pinMode(DATA_0_PIN, OUTPUT);    //DATA0
  pinMode(DATA_1_PIN, OUTPUT);    //DATA1
  pinMode(DATA_2_PIN, OUTPUT);    //DATA2
  pinMode(DATA_3_PIN, OUTPUT);    //DATA3
  pinMode(DATA_4_PIN, OUTPUT);    //DATA4
  pinMode(DATA_5_PIN, OUTPUT);    //DATA5
  pinMode(DATA_6_PIN, OUTPUT);    //DATA6
  pinMode(DATA_7_PIN, OUTPUT);    //DATA7
  pinMode(SD_CS_PIN, OUTPUT);     //SD CS

  //FADERS
  pinMode(MUX1_PIN, INPUT);  //MUX1
  pinMode(MUX2_PIN, INPUT);  //MUX2
  pinMode(MUX3_PIN, INPUT);  //MUX3
  pinMode(MUX6_PIN, INPUT);  //MUX6

  //BUTTONS
  pinMode(MUX5_PIN, INPUT_PULLUP);  //MUX5
  pinMode(MUX4_PIN, INPUT_PULLUP);  //MUX4

  pinMode(SYNC_DETECT_PIN, INPUT_PULLUP);  //SYNC DETECT (LOW=CONNECTED)
  pinMode(SYNC_IN_PIN, INPUT_PULLUP);      //SYNC IN (GOES LOW ON PULSE)
  pinMode(SYNC_OUT_PIN, OUTPUT);           //SYNC OUT

  mx.begin();
  mx.control(BlastLedMatrix::INTENSITY, brightness);
  ledsReady = true;

  opl2.init();
  opl2.setPercussion(true);
  opl2.setWaveFormSelect(1);
  opl2.writeHigh(5, 1);  //.  new mode on

  opl2.setMaintainSustain(0, OPERATOR1, 0);
  opl2.setMaintainSustain(0, OPERATOR2, 0);
  opl2.setMaintainSustain(3, OPERATOR1, 0);
  opl2.setMaintainSustain(3, OPERATOR2, 0);

  opl2.setMaintainSustain(1, OPERATOR1, 0);
  opl2.setMaintainSustain(1, OPERATOR2, 0);
  opl2.setMaintainSustain(4, OPERATOR1, 0);
  opl2.setMaintainSustain(4, OPERATOR2, 0);

  opl2.setMaintainSustain(2, OPERATOR1, 0);
  opl2.setMaintainSustain(2, OPERATOR2, 0);
  opl2.setMaintainSustain(5, OPERATOR1, 0);
  opl2.setMaintainSustain(5, OPERATOR2, 0);

  opl2.setMaintainSustain(9, OPERATOR1, 0);
  opl2.setMaintainSustain(9, OPERATOR2, 0);
  opl2.setMaintainSustain(12, OPERATOR1, 0);
  opl2.setMaintainSustain(12, OPERATOR2, 0);

  if (!startSkip) {
    showVersion();
    allOn();
  }
  loadSettings();  //EEPROM

  for (int16_t i = 0; i < 32; i++) {
    buttLast[i] = 1;
  }
  prepareFaders();



#if defined(__MK20DX256__)
  tickTimer.begin(SysTick_Handler, ISRCLOCK);
  tickTimer.priority(200);
#elif defined(STM32F1) || defined(STM32F4)
  ITimer0.attachInterruptInterval(ISRCLOCK, SysTick_Handler);
  sdTimerPrimed = true;
#endif


  if (!startSkip) {
    boot();  //check special boot buttons

    clearBottom();
    clearTop();
  }


  mux(14);
  delay(1);
  if (!digitalRead(MUX4_PIN)) {
    r3 = !r3;
    EEPROMupdate(113, r3);
    clearTop();
    topRow(1 + r3);
    delay(500);
    clearTop();
    delay(500);
    topRow(1 + r3);
    delay(500);
    clearTop();
    delay(500);
    topRow(1 + r3);
    delay(500);
    clearTop();
    delay(500);
  }

  //TEMPO held at startup? VGM mode
  mux(8);
  delay(1);
  if (!digitalRead(MUX5_PIN)) {

    #if defined(__MK20DX256__)  // Code specific to Teensy 3.1/3.2
    load();
   #endif

    updateRouting();
     pinMode(RUN_PIN, INPUT_PULLUP);  //boot play
     initAmp();
    vgm();
   
    mode = 4;
  } else {



    // Auto-load the last song/kit from SD at boot.
    checkCard();
    load();

    pinMode(RUN_PIN, INPUT_PULLUP);  //boot play
    seqReset();
    uClock.init();
    uClock.setClock96PPQNOutput(ClockOut96PPQN);
    uClock.setOnClockStartOutput(onClockStart);
    uClock.setOnClockStopOutput(onClockStop);
    updateRouting();
    visualiserTimeout = millis() + visualiserTimeoutMS;

    updateCurrentKit();
    stopSeq();
    seqReset();
    engineTick();
    fire();

    mode = 0;

    if (startSkip == 1) {
      mode = 3;
      showMode();
    }

    else if (startSkip == 2) {
      mode = 2;
      showMode();
    }
    EEPROMupdate(111, 0);
  }
  initAmp();
}


void boot() {

  bool skip = false;

  //first boot? - load factorySettings
  if (EEPROMread(2) != 123) {
    factorySettings();
    EEPROMwrite(2, 123);
  }


  //clear - factory settings?
  mux(2);
  if (!digitalRead(MUX5_PIN)) {

    skip = true;

    factorySettings();

    for (int16_t i = 0; i < 4; i++) {
      allOn();
      delay(150);
      clearTop();
      clearBottom();
      delay(150);
    }
  }



  if (!skip) {
    //SONG? (kit+function) boot SD reader

    mux(14);
    if (!digitalRead(MUX5_PIN)) {
      mux(11);
      if (!digitalRead(MUX5_PIN)) {
        allOn();
      }
    }
  }
  if (!skip) {
    // routing=midi channel
    mux(15);
    if (!digitalRead(MUX4_PIN)) {
      channelSetup = 1;
      midiSetupTarget = 0;
      clearTop();
      setLed(16, 1);
      setLed(17, 1);
      setLed(18, 1);
      setLed(19, 1);
      setLed(20, 1);
      setLed(21, 1);
      bottomRow(ch10 - 1);
      specialMode = true;
      skip = true;
    }
  }

  if (!skip) {
    //BEND? (slow/fast fall)

    mux(9);
    if (!digitalRead(MUX4_PIN)) {
      skip = true;

      if (EEPROMread(15)) {
        EEPROMwrite(15, 0);
        pitchFallRate = fallRateFast;
      } else {
        EEPROMwrite(15, 1);
        pitchFallRate = 10;
      }
      delay(800);
      setLed(9, 1);
      delay(100);
      setLed(9, 0);
      delay(200);
      if (pitchFallRate == 10) {
        setLed(9, 1);
        delay(100);
        setLed(9, 0);
      delay(500);
    }
  }

  markPatternVisualizerDirty();
}


  if (!skip) {
    //LENGTH? (safe attack/decay ranges)
    mux(1);
    if (!digitalRead(MUX5_PIN)) {
      skip = true;
      if (EEPROMread(8)) {
        EEPROMwrite(8, 0);
        safe = 0;
      } else {
        EEPROMwrite(8, 1);
        safe = 1;
      }
      delay(800);
      setLed(1, 1);
      delay(100);
      setLed(1, 0);
      delay(200);
      if (!safe) {
        setLed(1, 1);
        delay(100);
        setLed(1, 0);
        delay(500);
      }
    }
  }
}
