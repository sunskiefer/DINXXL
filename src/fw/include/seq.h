#pragma once

// --- Helpers for faster step wrap & probability ---
#include "pattern_visualizer.h"
// Recompute when pattern changes / at startup / after load.
static uint8_t gMaxStepCur[10];     // per-voice, inclusive 0..63
static int16_t gCachedPattern = -1; // track which pattern gMaxStepCur belongs to

static inline void markMaxStepCacheDirty() {
  gCachedPattern = -1;
}

static inline void recomputeMaxStepsForCurrentPattern() {
  for (int v = 0; v < 10; ++v) {
    // (pages<<4) + lastStep  == pages*16 + lastStep
    uint8_t ms = (uint8_t)((pages[currentPattern][v] << 4) + lastStepPoly[currentPattern][v]);
    // Clamp defensively to 63; protects odd files/edits.
    gMaxStepCur[v] = (ms > 63) ? 63 : ms;
  }
  gCachedPattern = currentPattern;
}

// --- DROP-IN REPLACEMENT ---
void seqStep() {
  // CALLED EACH TIME THE SEQ ADVANCES A STEP

  // reset "wiggled this step" flags
  for (int16_t i = 0; i < 152; i++) wiggledOnThisStep[i] = 0;



if (OnePpq) {
  // 1 PPQ: pulse every step
  setLed(LED_tempo, 1);
  tempoFlick = 10;
  sendSync();
} else {
  // 4-step division (like 4 PPQ): pulse every 4th step
  syncOutCounter = (syncOutCounter + 1) & 3; // modulo 4
  if (syncOutCounter == 0) {
    setLed(LED_tempo, 1);
    tempoFlick = 10;
    sendSync();
  }
}

  // Ensure max-step cache is valid (cheap check)
  if (gCachedPattern != currentPattern) {
    recomputeMaxStepsForCurrentPattern();
  }

  // advance all voice step counters and wrap
  for (int v = 0; v < 10; v++) {
    uint8_t s = (uint8_t)(polyStep[v] + 1);
    if (s > gMaxStepCur[v]) {
      s = 0;
      if (v == 0) {
        // polyStep[0] wrapped -> next pattern
        nextPattern();
        // pattern might have changed: refresh max steps
        recomputeMaxStepsForCurrentPattern();
      }
    }
    polyStep[v] = s;
  }

  if (allStepsZeroFlag) {
    for (int v = 0; v < 10; v++) polyStep[v] = 0;
    allStepsZeroFlag = false;
  }

  // write/automate trem/vibrato/swing
  vibTremSwingAuto();
  autoDone = false;

  // --- Stutter automation record/play ---
  const uint8_t step0 = (uint8_t)polyStep[0];

  if ((rec) && (mode == 2) && (function == 8) && stutterNumber) {
    stutterSteps[currentPattern][step0] = stutterNumber;
  }

  const uint8_t st = stutterSteps[currentPattern][step0];
  if (st && !stutterPressed) {
    // Each programmed stutter belongs to this sequencer step. Restarting the
    // automatic stutter here re-captures the voices' current positions, which
    // is essential when adjacent steps use the same stutter rate.
    stutterStepLast = st;
    setStutter(st, true);
  } else if (!st && stutterStepLast && !stutterPressed) {
    stutterStepLast = 0;
    setStutter(0, true);
  }

  // --- Bend automation ---
  if ((rec) && (mode == 2) && (function == 9) && bendHeld) {
    bendSteps[currentPattern][step0] = 1;
  }

  if (autoBendEnabled) {
    const uint8_t b = bendSteps[currentPattern][step0];
    if (b && !bendStepLast) {
      startBend(); bendStepLast = 1;
    } else if (!b && bendStepLast) {
      stopBend(); bendStepLast = 0;
    }
  }

  // --- Freeze automation ---
  if (rec && freezeHeld) {
    freezeSteps[currentPattern][step0] = 1;
  }

  if (autoFreezeEnabled) {
    if (freezeSteps[currentPattern][step0]) {
      freezeTimer = 100;
    } else if (!freezeHeld && freeze) {
      opl2.freeze(0);
      freeze = false;
      resetRegisters = true;
    }
  }

  // safety clamp (usually redundant since we wrap above)
  for (int v = 0; v < 10; v++) {
    if (polyStep[v] > 63) polyStep[v] = 63;
  }

  patternVisualizerSendStepPositions(polyStep, 10);

  if (!stutter || (stutter && firstStutter)) {
    firstStutter = false;
    stutterCounter = 0;
    fire();
  }
}

void nextTargetStep() {
  // advance one step with wrap by page length
  uint8_t maxIdx = 15;
  switch (pages[currentPattern][selectedVoice]) {
    default:
    case 0: maxIdx = 15; break;
    case 1: maxIdx = 31; break;
    case 2: maxIdx = 47; break;
    case 3: maxIdx = 63; break;
  }
  uint8_t ts = (uint8_t)(targetStep + 1);
  if (ts > maxIdx) ts = 0;
  targetStep = ts;
}

void triggerStep() {
  // every step

  if (!midiSlave && !analogSlave) {
    // tighten bounds check, fewer branches
    if ((stepTickCounter > (stepTicks + 1)) || (stepTickCounter + 1 < stepTicks)) {
      stepTicks = stepTickCounter;
    }
    stepTickCounter = 0;
  }

  // adapt to evolving tempo
  updateSwing(swingSetting[currentPattern]);

  // compute getReady using bit tests instead of %
  if (polyStep[0] & 1) {
    // odd
    getReady = (!wonkySwing) ? 1 : (uint8_t)(1 + swing);
  } else {
    // even
    getReady = (wonkySwing) ? 1 : (uint8_t)(1 + swing);
  }
}

void stopSeq() {
  run = false;
  uClock.stop();
  setLed(LED_run, run);
}

void startSeq() {
  vIndex = tremIndex = 0;
  run = true;
  uClock.start();
  setLed(LED_run, run);
}

void seqReset() {
  firstStutter = true;
  tremTickCounter = 0;
  vibTickCounter  = 0;
  autoDone = false;

  resetBendAutomation();
  rec2 = false;
  if (pattArmed) pattArmed = false;

  // cancel pendingPattern
  if (pendingPattern > -1) pendingPattern = -1;

  if (pattChainLength) {
    loadPattern(pattChain[pattChainStep]);
  }

  // Queued saves must survive mode changes and only execute once transport stops.
  if (!run) {
    if (saveFlag)    { save();    saveFlag = false; }
    if (saveKitFlag) { saveKit(); saveKitFlag = false; }
  }

  tempoAlt = 0;
  for (int i = 0; i < 10; i++) polyStep[i] = 0;

  // we may have changed pattern or lengths; refresh cache
  recomputeMaxStepsForCurrentPattern();

  killSynths();
  showPattern();
}

void fireVoiceWithNudge(uint8_t voice, uint8_t step) {
  float temp = getNudge(step, voice) * (1.0f / 8.0f); // faster than divide
  if (temp != 0.0f) {
    nudgeDelayCountDown[voice] = (uint16_t)(stepTicks * temp);
  } else {
    fireVoiceNow(voice);
  }
}

void fireVoiceNow(uint8_t number) {
  switch (number) {
    case 0:
      bitWrite(opl[currentKit][0], 4, 1);
      pitchNow[0] = pitchRoot[0];
      pitchNow[1] = pitchRoot[1];
      midiHot[0] = true;
      break;  // KICK
    case 1:
      bitWrite(opl[currentKit][0], 3, 1);
      pitchNow[2] = pitchRoot[2];
      midiHot[1] = true;
      break;  // SNARE
    case 2:
      bitWrite(opl[currentKit][0], 2, 1);
      pitchNow[3] = pitchRoot[3];
      midiHot[2] = true;
      break;  // TOM
    case 3:
      opl2.setRelease(7, OPERATOR1, hatDecay);
      bitWrite(opl[currentKit][0], 0, 1);
      midiHot[3] = true;
      break;  // CH
    case 4:
      opl2.setRelease(7, OPERATOR1, hatDecayOpen);
      bitWrite(opl[currentKit][0], 0, 1);
      midiHot[4] = true;
      break;  // OH
    case 5:
      bitWrite(opl[currentKit][0], 1, 1);
      pitchNow[4] = pitchRoot[4];
      midiHot[5] = true;
      break;  // CYMBAL
    case 6: bitWrite(synthTrig, 0, 1); break;
    case 7: bitWrite(synthTrig, 1, 1); break;
    case 8: bitWrite(synthTrig, 2, 1); break;
    case 9: bitWrite(synthTrig, 3, 1); break;
  }
}

void fire() {
  // choose the step set (stutter or normal), no branches in the loop
  const bool useStutter = (stutter > 0) && !drill;
  for (int i = 0; i < 10; i++) {
    fireStep[i] = useStutter ? stutterStepPoly[i] : polyStep[i];
  }

  newStep = true;
  flipStepFlag = true;

  // precompute probabilities for ALL 10 voices (single row lookup)
  const uint8_t* __restrict probRow = prob[ kit[currentPattern] ];
  for (int i = 0; i < 10; i++) {
    probFire[i] = getProb(probRow[i]);
  }

  // clear drum gates and flag synth update
  opl[currentKit][0] = 0;
  bitWrite(opl[currentKit][0], 7, 1);
  synthTrig = 0;

  // voice masks (bits 0..5 in pattern[currentPattern][0][step])
  // voices 0..5 are drum bits 0..5; voices 6..9 are synth lanes 0..3 (getStep)
  const uint8_t drumBitMask[6] = {1, 2, 4, 8, 16, 32};

  // Solo/mute selection
  // solo==0 -> use mute[v]==0
  // solo in 1..10 -> only that voice is allowed
  const int soloIdx = (solo > 0) ? (solo - 1) : -1;

  // --- DRUM VOICES 0..5 ---
  const uint8_t* patRow0 = pattern[currentPattern][0]; // 64-step row
  for (int v = 0; v < 6; v++) {
    if ((soloIdx >= 0 && soloIdx != v) || (soloIdx < 0 && mute[v])) continue;
    const uint8_t step = (uint8_t)fireStep[v];
    if ((patRow0[step] & drumBitMask[v]) && probFire[v]) {
      fireVoiceWithNudge(v, step);
    }
  }

  // --- SYNTH VOICES 6..9 (lanes 0..3) ---
  for (int lane = 0; lane < 4; lane++) {
    const int v = 6 + lane;
    if ((soloIdx >= 0 && soloIdx != v) || (soloIdx < 0 && mute[v])) continue;
    const uint8_t step = (uint8_t)fireStep[v];
    if (getStep(lane, step) && probFire[v]) {
      fireVoiceWithNudge(v, step);
    }
  }

  // metronome on closed hat when rec is on
  if (metro && rec) {
    if ( (polyStep[0] & 3) == 0 ) { // %4==0
      metroStep = true;
      bitWrite(opl[currentKit][0], 0, 1);
    } else {
      metroStep = false;
    }
  } else {
    metroStep = false;
  }
}
