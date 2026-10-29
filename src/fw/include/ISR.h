// ---- fast integer "map": i = (x / denom) * 63  ->  i ≈ (x * inv) >> SHIFT ----
// We recompute inv only when denom changes (rare vs ISR frequency).
static inline uint8_t fastIdx(uint32_t x, uint32_t inv, uint8_t SHIFT) {
  uint32_t y = (x * inv) >> SHIFT;   // 0..63 (or close), no division in ISR
  return (y > 63u) ? 63u : (uint8_t)y;
}

// Precompute (1<<SHIFT)/den with rounding. Call this only on changes.
static inline uint32_t recip_u32(uint32_t den, uint8_t SHIFT) {
  if (den == 0) den = 1; // guard
  uint64_t one = (uint64_t)1 << SHIFT;
  return (uint32_t)((one + (den >> 1)) / den);
}

// Cache for vib/trem mapping based on current pattern settings
static uint16_t lastStepTicks = 0xFFFF;
static uint8_t  lastVibRate   = 0xFF;
static uint8_t  lastTremRate  = 0xFF;
static uint8_t  lastVibMult   = 0xFF;
static uint8_t  lastTremMult  = 0xFF;
static uint16_t lastVibStepTicks  = 0xFFFF;
static uint16_t lastTremStepTicks = 0xFFFF;

// For rates that use "ticks*N" period we keep reciprocal of (stepTicks * factor)
static uint32_t vibInv = 0, tremInv = 0;
static uint8_t  vibSHIFT = 16, tremSHIFT = 16; // 16 is plenty for 63-scale

// Preview counters to keep LFOs running while the transport is stopped
static uint16_t previewStepCounter = 0;
static uint16_t previewStepTicks   = 1;
static uint16_t previewStepHalf    = 1;

// Cached thresholds and reciprocals for the fast one-step/half-step LFO rates.
static uint16_t stepTicksHalf = 0;
static uint32_t stepTicksInv = 0, stepTicksHalfInv = 0;

// Helper to refresh cached reciprocals and thresholds when needed.
static inline void refreshTempoCaches() {
  if (lastStepTicks != stepTicks) {
    lastStepTicks = stepTicks;
    const uint16_t safeTicks = stepTicks ? (uint16_t)stepTicks : 1u;
    stepTicksHalf = (uint16_t)(safeTicks >> 1);
    const uint16_t safeHalf = stepTicksHalf ? stepTicksHalf : 1u;
    stepTicksInv = recip_u32(safeTicks, 16);
    stepTicksHalfInv = recip_u32(safeHalf, 16);
  }
}

// vib/trem rate → factor (period in steps). Matches your switch() cases.
static inline uint8_t vibFactor(uint8_t r) {
  static const uint8_t F[10] = {32,16,12,8,6,4,3,2,1,1}; // 8,9 handled specially
  return (r <= 7) ? F[r] : 1;
}
static inline uint8_t tremFactor(uint8_t r) {
  static const uint8_t F[10] = {32,16,12,8,6,4,3,2,1,1}; // 8,9 handled specially
  return (r <= 7) ? F[r] : 1;
}

// Recompute vibrato reciprocal if relevant inputs changed
static inline void refreshVibRecip() {
  uint8_t vr = vibratoRate[currentPattern];
  uint8_t vm = vibratoRateMultiplier[currentPattern];
  uint16_t ticks = (stepTicks > 0) ? (uint16_t)stepTicks : 1u;
  if (vr != lastVibRate || vm != lastVibMult || ticks != lastVibStepTicks) {
    lastVibRate = vr;
    lastVibMult = vm;
    lastVibStepTicks = ticks;
    if (vr <= 7) {
      uint32_t den = (uint32_t)ticks * vibFactor(vr);
      vibInv = recip_u32(den, vibSHIFT);
    } else {
      // Rates 8/9 use stepTickCounter based mapping; no reciprocal needed.
      vibInv = 0;
    }
  }
}

// Recompute tremolo reciprocal if relevant inputs changed
static inline void refreshTremRecip() {
  uint8_t tr = tremRate[currentPattern];
  uint8_t tm = tremRateMultiplier[currentPattern];
  uint16_t ticks = (stepTicks > 0) ? (uint16_t)stepTicks : 1u;
  if (tr != lastTremRate || tm != lastTremMult || ticks != lastTremStepTicks) {
    lastTremRate = tr;
    lastTremMult = tm;
    lastTremStepTicks = ticks;
    if (tr <= 7) {
      uint32_t den = (uint32_t)ticks * tremFactor(tr);
      tremInv = recip_u32(den, tremSHIFT);
    } else {
      tremInv = 0;
    }
  }
}

// Quick table for targetStep max indices by page (15,31,47,63)
static inline uint8_t pageMaxIdx(uint8_t pageLenCode) {
  static const uint8_t M[4] = {15,31,47,63};
  return M[ pageLenCode & 3 ];
}

void SysTick_Handler(void) {
  // This check is cheap; the two divisions only run after a tempo change.
  refreshTempoCaches();

  // nudge countdown
  for (int i = 0; i < 10; i++) {
    uint16_t c = nudgeDelayCountDown[i];
    if (c) {
      c--;
      nudgeDelayCountDown[i] = c;
      if (!c) nudged[i] = true;
    }
  }

  if (plocStep && nudgeDelayTimer) {
    if (--nudgeDelayTimer == 0) nudgeMode = 1;
  }

  if (wiping) { wipeCounter++; }

  if (doubleClickTimer) {
    if (--doubleClickTimer == 0) lastDouble = -1;
  }

  // expire held midi out notes
  for (int i = 0; i < 11; i++) {
    if (midiTimeout[i] > 1) midiTimeout[i]--;
  }

  // tap tempo timeout
  if (tapCounter < 2000) {
    if (++tapCounter == 2000) tempoTaps = 0;
  }

  // Analog Slave ticks
  if (analogSlave) {
    if (analogCounter < analogMax) {
      analogCounter++;
      if (analogCounter == analogHalf || analogCounter == analogQuarter || analogCounter == analogThreeQuarters) {
        triggerStep();
      }
      if (analogCounter == analogMax) stopSeq();
    }
  }

  if (run) {
    // stutter
    if (stutter) {
      if (++stutterCounter >= stutterRate) {
        stutterCounter = 0;
        if (wiggleHeld && stutter == 1) {
          for (int i = 0; i < 10; i++) stutterStepPoly[i] = getRandom();
        }
        fire();
      }
    }

    // --- targetStep preview (mid-step) ---
    stepTickCounter++;
    if (stepTickCounter > stepTicksHalf) {
      uint8_t maxI = pageMaxIdx(pages[currentPattern][selectedVoice]);
      uint8_t ps   = (uint8_t)polyStep[selectedVoice];
      targetStep   = (ps < maxI) ? (uint8_t)(ps + 1) : 0;
    } else {
      targetStep = (uint8_t)polyStep[selectedVoice];
    }
    if (!quantise) targetStep = (uint8_t)polyStep[selectedVoice];
  }

  // Maintain a synthetic step counter so LFOs keep moving while stopped
  uint16_t stepTicksSafe = (stepTicks > 0) ? (uint16_t)stepTicks : 1u;
  if (run) {
    previewStepCounter = (uint16_t)stepTickCounter;
    previewStepTicks   = stepTicksSafe;
    previewStepHalf    = (uint16_t)(stepTicksHalf ? stepTicksHalf : 1);
  } else {
    previewStepTicks = stepTicksSafe;
    previewStepHalf  = (uint16_t)((stepTicksSafe >> 1) ? (stepTicksSafe >> 1) : 1);
    if (previewStepCounter >= previewStepTicks) {
      previewStepCounter %= previewStepTicks;
    }
    previewStepCounter++;
    if (previewStepCounter >= previewStepTicks) {
      previewStepCounter = 0;
    }
  }

  // SLOW STUFF
  if (flicker) {
    if (--flicker == 0) { flip = true; }
  }
  if (runBounce > 0) runBounce--;
  engineTickHot = true;

  // swing delay expiration -> signal main loop
  if (getReady) {
    if (--getReady == 0) { newStepHot = true; }
  }

  // kill analog pulse after 20ms
  if (pulseCounter) {
    if (--pulseCounter == 0) {
      digitalWrite(SYNC_OUT_PIN, LOW);
    }
  }

  // ---- Bend engine ----
  if (bend) {
    if (++bendCounter > (uint8_t)(10 - bendy)) {
      bendCounter = 0;
      for (int i = 0; i < 15; i++) {
        newBend[i] = true;
        int16_t f = fNumberBend[i];
        if (f > 0) {
          f += (funcHeld ? bendy : -bendy);
          if (f < 0) f = 0;
          fNumberBend[i] = f;
        }
      }
    }
  }

  // ---- Vibrato & Tremolo LFOs (no divisions in ISR) ----
  if (!bend) {
    // Multiplier handling: 0 -> /8 via subcounter; 1 -> +1; 2 -> +8
    uint8_t vibMul = vibratoRateMultiplier[currentPattern];
    if (vibMul == 0) {
      if (++vibSubCounter > 7) { vibSubCounter = 0; vibTickCounter++; }
    } else {
      vibTickCounter += (vibMul == 2) ? 8 : 1;
    }

    uint8_t tremMul = tremRateMultiplier[currentPattern];
    if (tremMul == 0) {
      if (++tremSubCounter > 7) { tremSubCounter = 0; tremTickCounter++; }
    } else {
      tremTickCounter += (tremMul == 2) ? 8 : 1;
    }

    // (Re)compute reciprocals only when settings changed
    refreshVibRecip();
    refreshTremRecip();

    // ---- Vibrato index vIndex (0..63) ----
    uint8_t vr = vibratoRate[currentPattern];
    if (vr <= 7) {
      // period = stepTicks * factor(vr)
      if ((uint32_t)vibTickCounter > (uint32_t)stepTicks * vibFactor(vr)) vibTickCounter = 0;
      vIndex = fastIdx((uint32_t)vibTickCounter << 6, vibInv, vibSHIFT); // x scaled to 0..den*64
    } else if (vr == 8) {
      // cycle = 1 step
      if (vibMul == 2) {
        uint8_t idx = vIndex + 8; vIndex = (idx >= 64) ? (idx - 64) : idx;
      }
      uint16_t stc = run ? (uint16_t)stepTickCounter : previewStepCounter;
      vIndex = fastIdx((uint32_t)stc << 6, stepTicksInv, vibSHIFT);
    } else { // vr == 9 : cycle = 1/2 step (two halves)
      uint16_t half = run ? (uint16_t)(stepTicksHalf ? stepTicksHalf : 1) : previewStepHalf;
      uint16_t stc  = run ? (uint16_t)stepTickCounter : previewStepCounter;
      if (vibMul == 2) {
        uint8_t idx = vIndex + 32; vIndex = (idx >= 64) ? (idx - 64) : idx;
      }
      if (stc < half) {
        vIndex = fastIdx(((uint32_t)stc) << 6, stepTicksHalfInv, vibSHIFT);
      } else {
        uint16_t d = (uint16_t)(stc - half);
        vIndex = fastIdx(((uint32_t)d) << 6, stepTicksHalfInv, vibSHIFT);
      }
    }

    // vibrato value
    vibWav = (vibWav > 9) ? 9 : vibWav; // constrain 0..9
    int16_t vibSample = (int16_t)wav[vibWav][vIndex] - 63;
    vibrato = (int16_t)(vibSample * vibratoPowers[vibratoPower[currentPattern]]);

    // ---- Tremolo index tremIndex (0..63) ----
    uint8_t tr = tremRate[currentPattern];
    if (tr <= 7) {
      if ((uint32_t)tremTickCounter > (uint32_t)stepTicks * tremFactor(tr)) tremTickCounter = 0;
      tremIndex = fastIdx((uint32_t)tremTickCounter << 6, tremInv, tremSHIFT);
    } else if (tr == 8) {
      if (tremMul == 2) { uint8_t idx = tremIndex + 8;  tremIndex = (idx >= 64) ? (idx - 64) : idx; }
      if (tremMul == 1) {
        uint16_t stc = run ? (uint16_t)stepTickCounter : previewStepCounter;
        tremIndex = fastIdx((uint32_t)stc << 6, stepTicksInv, tremSHIFT);
      } else {
        if (tremTickCounter > stepTicks) tremTickCounter = 0;
        tremIndex = fastIdx((uint32_t)tremTickCounter << 6, stepTicksInv, tremSHIFT);
      }
    } else { // tr == 9
      if (tremMul == 2) { uint8_t idx = tremIndex + 32; tremIndex = (idx >= 64) ? (idx - 64) : idx; }
      uint16_t half = run ? (uint16_t)(stepTicksHalf ? stepTicksHalf : 1) : previewStepHalf;
      uint16_t stc  = run ? (uint16_t)stepTickCounter : previewStepCounter;
      if (tremMul == 1) {
        if (stc < half) tremIndex = fastIdx(((uint32_t)stc) << 6, stepTicksHalfInv, tremSHIFT);
        else            tremIndex = fastIdx(((uint32_t)(stc - half)) << 6, stepTicksHalfInv, tremSHIFT);
      } else {
        if (tremTickCounter > half) tremTickCounter = 0;
        tremIndex = fastIdx(((uint32_t)tremTickCounter) << 6, stepTicksHalfInv, tremSHIFT);
      }
    }

    tremWav = (tremWav > 9) ? 9 : tremWav;  // constrain 0..9
    uint8_t tval = (uint8_t)(wav[tremWav][tremIndex] >> 1); // /2
    if (tval != tremoloLast) {
      tremoloLast = tval;
      tremolo = tval;
      newTrem = true;
    }
  }

  // scan counters for function 8/9 UI
  if (mode == MODE_FUNCTION && (function == 8 || function == 9)) {
    scanCount2++;
  }

  // blink tempo led finish
  if (tempoFlick) {
    if (--tempoFlick == 0) { killTempoLed = true; }
  }

  // show mode after number
  if (showTimer) {
    if (--showTimer == 0) { showModeFlag = true; }
  }

  // flasher/chaser
  if (++flashTimer == 10 || flashTimer == 30) {
    newFlash = true;
    if (flashTimer == 10) {
      flasher = 1;
    } else {
      flasher = 0;
      flashTimer = 0;
      if (++chaseStep > 3) chaseStep = 0;
    }
  }
}
