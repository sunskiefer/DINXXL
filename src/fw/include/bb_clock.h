#pragma once

void sendMidiRealtime(uint8_t realtime);

float counterToBPM(int counter) {
  // Counter increments every 5ms, counter value represents the duration of 4 beats
  float durationOf4BeatsInMinutes = (counter * 5) / 60000.0;  // Convert duration to minutes
  float beatsPerMinute = (4.0 / durationOf4BeatsInMinutes);   // Calculate BPM
  return beatsPerMinute;
}




const uint8_t remapBpm[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };

void tempoEntry(uint8_t input) {
  input = remapBpm[input];
  if (tempoPhase < 3) tempoPhase++;
  switch (tempoPhase) {
    case 1:
      tempTempo2 = 0;
      tempTempo1 = input;
      break;
    case 2: tempTempo2 = input; break;
    case 3:
      //calculate tempo
      if (input < 10) {
        //we pressed a third button, use 3 digit BPM
        tempo = (input + (tempTempo1 * 100) + (tempTempo2 * 10));
        updateSwing(swingSetting[currentPattern]);
        uClock.setTempo(tempo);
        if (millis() > 2000) {
          EEPROMupdate(26, lowByte(tempo));
          EEPROMupdate(27, highByte(tempo));
        }
      } else {
        //we only pressed 2 buttons before tempo butt was released, use 2 digit BPM
        tempo = (tempTempo1 * 10) + (tempTempo2);
        updateSwing(swingSetting[currentPattern]);
        uClock.setTempo(tempo);

        if (millis() > 2000) {
          EEPROMupdate(26, lowByte(tempo));
          EEPROMupdate(27, highByte(tempo));
        }
      }

      tempoPhase = 0;
      break;
  }
}

void analogPulseRead() {
  if (digitalRead(SYNC_IN_PIN) != analogPulseLast) {
    analogPulseLast = !analogPulseLast;
    if (!analogPulseLast) {

      if (OnePpq) {
        triggerStep();
      } else {

        //Pin Rising //first pulse since startup or lost signal
        if (analogCounter == analogMax) {
          seqReset();
          startSeq();
        }
        triggerStep();
        tempo = counterToBPM(analogCounter << 2);
        updateSwing(swingSetting[currentPattern]);
        uClock.setTempo(tempo);
        analogQuarter = analogCounter / 4;
        analogHalf = analogCounter / 2;
        analogThreeQuarters = analogCounter * .75;
        stepTicks = analogCounter / 4;
        analogCounter = 0;
      }
    }
  }
}


void updateSwing(uint8_t number) {
  swingSetting[currentPattern] = number;
  swing = (stepTicks / 14) * swingSetting[currentPattern];
}

void sendSync() {
  if (OnePpq != lastOnePpq) {
    syncOutCounter = 0;
    lastOnePpq = OnePpq;
  }
  digitalWrite(SYNC_OUT_PIN, LOW);
  digitalWrite(SYNC_OUT_PIN, HIGH);
  pulseCounter = 2;
}

void midiClockAdvance() {
  stepTicks = stepTickCounter;

  stepTickCounter = 0;
  tempo = counterToBPM(stepTicks << 4);

  updateSwing(swingSetting[currentPattern]);
  uClock.setTempo(tempo);
  midiClockTicks = 0;
  triggerStep();
}

#if defined(__MK20DX256__)  // Code specific to Teensy 3.1/3.2
bool firstTick;
uint8_t bpm_blink_timer = 1;
void handle_bpm_led(uint32_t* tick) {
  // BPM led indicator
  if (!(*tick % (6))) {  // each quarter led on

    if (!midiSlave && !firstTick && !analogSlave) {
      triggerStep();
    }
  }

  firstTick = false;
}

#elif defined(STM32F1) || defined(STM32F4)  // Code specific to STM32

bool firstTick;
uint8_t bpm_blink_timer = 1;
void handle_bpm_led(uint32_t tick) {
  // BPM led indicator
  if (!(tick % (6))) {  // each quarter led on

    if (!midiSlave && !firstTick && !analogSlave) {
      triggerStep();
    }

    firstTick = false;
  }
}

#endif

#if defined(STM32F1) || defined(STM32F4)
void ClockOut96PPQN(uint32_t tick) {
  // Send MIDI_CLOCK to external gears
  sendMidiRealtime(0xF8);
  handle_bpm_led(tick);
}
#else
void ClockOut96PPQN(uint32_t* tick) {
  // Send MIDI_CLOCK to external gears
  sendMidiRealtime(0xF8);
#if defined(__MK20DX256__)  // Teensy uses pointer variant
  handle_bpm_led(tick);
#endif
}
#endif

void onClockStart() {
  firstTick = true;
  sendMidiRealtime(0xFA);
}

void onClockStop() {
  sendMidiRealtime(0xFC);
}
