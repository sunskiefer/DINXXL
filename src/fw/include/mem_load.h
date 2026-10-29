#include <string.h>
#include "scene_types.h"
#include "packed_storage.h"

extern SceneBank scenes;

// ===== Fast buffered byte reader for SD files (drop-in helper) =====
struct SdByteReader {
  File* f = nullptr;
  static const size_t BUFSZ = 2048;           // tune 1024..4096 as you like
  uint8_t buf[BUFSZ];
  size_t pos = 0;
  size_t len = 0;
  bool underflow = false;

  inline void begin(File& file) {
    f   = &file;
    pos = 0;
    len = 0;
    underflow = false;
  }

  inline bool refill() {
    if (!f) {
      underflow = true;
      len = 0;
      return false;
    }
    len = f->read(buf, BUFSZ);
    pos = 0;
    if (len == 0) {
      underflow = true;
      return false;
    }
    underflow = false;
    return true;
  }

  inline bool ensure() {
    if (pos < len) return true;
    return refill();
  }

  inline uint8_t get() {
    if (!ensure()) {
      return 0; // EOF safeguard (your format shouldn't hit this)
    }
    return buf[pos++];
  }

  inline void readBytes(uint8_t* dst, size_t count) {
    while (count) {
      if (!ensure()) {
        memset(dst, 0, count);
        return;
      }
      size_t chunk = len - pos;
      if (chunk > count) chunk = count;
      memcpy(dst, buf + pos, chunk);
      dst += chunk;
      pos += chunk;
      count -= chunk;
    }
  }

  inline void skip(size_t count) {
    while (count) {
      if (!ensure()) return;
      size_t chunk = len - pos;
      if (chunk > count) chunk = count;
      pos += chunk;
      count -= chunk;
    }
  }

  inline bool exhausted() const {
    return underflow;
  }
};

// Small safe snprintf helper
static inline void makePath(char* dst, size_t dstsz, const char* fmt,
                            uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
  snprintf(dst, dstsz, fmt, a, b, c, d);
}

// Overload for 3 params
static inline void makePath(char* dst, size_t dstsz, const char* fmt,
                            uint16_t a, uint16_t b, uint16_t c) {
  // supports up to 3 %u in fmt; extra args ignored if fmt has fewer.
  snprintf(dst, dstsz, fmt, a, b, c);
}

// Overload for 2 params
static inline void makePath(char* dst, size_t dstsz, const char* fmt,
                            uint16_t a, uint16_t b) {
  snprintf(dst, dstsz, fmt, a, b);
}

// Overload for 1 param
static inline void makePath(char* dst, size_t dstsz, const char* fmt,
                            uint16_t a) {
  snprintf(dst, dstsz, fmt, a);
}

// ======= DROP-IN REPLACEMENT: load() =================================
void load() {
  startSD();
  const uint16_t folderIdx = static_cast<uint16_t>(currentFolder + 1);
  const uint16_t songIdx   = static_cast<uint16_t>(currentSong + 1);

  for (int16_t i = 0; i < 10; i++) {
    loadKit(0, i);
    loadKit(1, i);
    loadKit(2, i);
    loadKit(3, i);
  }

  /////////////////////////
  /////  DRUM PATTERN /////
  /////////////////////////
  {
    char path[96];
    for (int16_t i = 0; i < 16; i++) {

      makePath(path, sizeof(path),
               "/SONGS/BANK%u/SONG%u/PATD%u.BB",
               folderIdx,
               songIdx,
               static_cast<uint16_t>(i + 1));

      File f = SD.open(path, FILE_READ);
      if (f) {
        SdByteReader reader; reader.begin(f);
        // Temporarily remap getSd() to the buffered reader for this scope
        #ifdef getSd
        #undef getSd
        #endif
        #define getSd() reader.get()

        for (int16_t j = 0; j < 2; j++) {
          reader.readBytes(pattern[i][j], 64);
        }
        pagesLegacy[i] = getSd();
        lastStepPoly[i][0] = getSd();
        swingSetting[i] = getSd();
        if (swingSetting[i] > 9) swingSetting[i] = 0;
        vibratoRate[i] = getSd();
        if (vibratoRate[i] > 9) vibratoRate[i] = 5;
        tremRate[i] = getSd();
        if (tremRate[i] > 9) tremRate[i] = 5;
        kit[i] = getSd();
        if (kit[i] > 9) kit[i] = 0;

        for (int16_t j = 0; j < 6; j++) {
          offsetOct[i][j] = getSd();
          if (offsetOct[i][j] > 3) offsetOct[i][j] = 1;  // default
          if (offsetOct[i][j] == 3) offsetOct[i][j] = -1;
        }
        reader.readBytes(lastStepPoly[i], 10);
        for (int16_t j = 0; j < 10; j++) {
          if (lastStepPoly[i][j] > 15) {
            lastStepPoly[i][j] = 15;
          }
        }
        uint8_t pageTmp[10];
        reader.readBytes(pageTmp, 10);
        for (int16_t j = 0; j < 10; j++) {
          uint8_t pageVal = pageTmp[j];
          if (pageVal > 3) pageVal = 3;
          pages[i][j] = pageVal;
        }

        int16_t fileVersion = getSd();
        if (fileVersion < 40) {
          for (int16_t j = 0; j < 10; j++) {
            lastStepPoly[i][j] = 15;
            pages[i][j] = pagesLegacy[i];
          }
        }

        #undef getSd
        f.close();
      }
    }
  }

  /////////////////////////
  /////  SYNTH PATTERN ////
  /////////////////////////
  {
    char path[96];

    for (int16_t i = 0; i < 16; i++) {
      makePath(path, sizeof(path),
               "/SONGS/BANK%u/SONG%u/PATS%u.BB",
               folderIdx,
               songIdx,
               static_cast<uint16_t>(i + 1));

      File f = SD.open(path, FILE_READ);
      if (f) {
        SdByteReader reader; reader.begin(f);
        #ifdef getSd
        #undef getSd
        #endif
        #define getSd() reader.get()

        for (int16_t j = 0; j < 3; j++) {
          reader.readBytes(synth[i][j], 64);
        }
        for (int j = 6; j < 10; j++) {
          offsetOct[i][j] = getSd();
          if (offsetOct[i][j] > 3) offsetOct[i][j] = 1;
          else if (offsetOct[i][j] == 3) offsetOct[i][j] = -1;
        }
        reader.readBytes(synth[i][3], 64);

        tremRateMultiplier[i] = getSd(); if (tremRateMultiplier[i] == 0) tremRateMultiplier[i] = 2;
        tremRateMultiplier[i]--; tremRateMultiplier[i] = constrain(tremRateMultiplier[i], 0, 2);
        vibratoRateMultiplier[i] = getSd(); if (vibratoRateMultiplier[i] == 0) vibratoRateMultiplier[i] = 2;
        vibratoRateMultiplier[i]--; vibratoRateMultiplier[i] = constrain(vibratoRateMultiplier[i], 0, 2);
        vibratoPower[i] = getSd(); if (vibratoPower[i] == 0) vibratoPower[i] = 1; vibratoPower[i]--;
        vibratoPower[i] = constrain(vibratoPower[i], 0, 9);

        #undef getSd
        f.close();
      }
    }
  }

  /////////////////////////
  /////  AUTOMATION   /////
  /////////////////////////
  {
    char path[96];
    makePath(path, sizeof(path),
             "/SONGS/BANK%u/SONG%u/AUTO.BB",
             folderIdx,
             songIdx);

    File f = SD.open(path, FILE_READ);
    if (f) {
      SdByteReader reader; reader.begin(f);
      #ifdef getSd
      #undef getSd
      #endif
      #define getSd() reader.get()

      const uint16_t fallbackTempo =
        (tempo >= MIN_BPM && tempo <= MAX_BPM) ? static_cast<uint16_t>(tempo) : 100;
      bool truncated = false;

      memset(sceneValidMask, 0, sizeof(sceneValidMask));
      scenes.clear();

      reader.skip(9000);
      truncated |= reader.exhausted();

      for (int16_t j = 0; j < 16; j++) {
        for (int16_t i = 0; i < 64; i++) {
          stutterSteps[j][i] = getSd();
          bendSteps.set(j, i, getSd() != 0);
          freezeSteps.set(j, i, getSd() != 0);
        }
      }

      uint16_t tempoLow  = getSd();
      uint16_t tempoHigh = getSd();
      uint16_t tempoCandidate = tempoLow | (tempoHigh << 8);
      truncated |= reader.exhausted();
      if (tempoCandidate < MIN_BPM || tempoCandidate > MAX_BPM || truncated) {
        tempoCandidate = fallbackTempo;
      }
      tempo = tempoCandidate;
      uClock.setTempo(tempo);

      for (int16_t i = 0; i < 10; i++) {
        reader.readBytes(prob[i], 10);
        for (int16_t j = 0; j < 10; j++) {
          if (prob[i][j] > 9) prob[i][j] = 0;
        }
      }
      randomDepth[0] = getSd(); if (randomDepth[0] > 9) randomDepth[0] = 0;

      uint8_t temp1;
      for (int16_t k = 0; k < 16; k++) {
        for (int16_t j = 0; j < 12; j++) {
          temp1 = getSd();
          for (int16_t i = 0; i < 8; i++) {
            randomer[k][i + (j * 8)] = bitRead(temp1, i) ? 1 : 0;
          }
        }
      }
      for (int16_t i = 1; i < 16; i++) {
        randomDepth[i] = getSd();
        if (randomDepth[i] > 9) randomDepth[i] = 0;
      }

      for (int16_t k = 0; k < 16; k++) {
        for (int16_t j = 0; j < 32; j++) {
          vibAuto[k][j]   = getSd(); if (vibAuto[k][j]   > 170) vibAuto[k][j]   = 0;
          tremAuto[k][j]  = getSd(); if (tremAuto[k][j]  > 170) tremAuto[k][j]  = 0;
          swingAuto[k][j] = getSd(); if (swingAuto[k][j] > 170) swingAuto[k][j] = 0;
          wiggleAuto[k][j]= getSd(); if (wiggleAuto[k][j]> 170) wiggleAuto[k][j]= 0;
          kitAuto[k][j]   = getSd(); if (kitAuto[k][j]   > 170) kitAuto[k][j]   = 0;
          phonesAuto[k][j]= getSd(); if (phonesAuto[k][j]> 170) phonesAuto[k][j]= 0;
          mixAuto[k][j]   = getSd(); if (mixAuto[k][j]   > 170) mixAuto[k][j]   = 0;
        }
      }

      reader.readBytes(autoData, 3000);

      for (int16_t k = 0; k < 16; k++) {
        for (int16_t j = 0; j < 32; j++) {
          vibAutoWav[k][j]  = getSd(); if (vibAutoWav[k][j]  > 170) vibAutoWav[k][j]  = 0;
          tremAutoWav[k][j] = getSd(); if (tremAutoWav[k][j] > 170) tremAutoWav[k][j] = 0;
        }
      }

      for (int16_t i = 1; i < 16; i++) {
        tremWavPatt[i] = getSd(); if (tremWavPatt[i] > 10) tremWavPatt[i] = 0;
        vibWavPatt[i]  = getSd(); if (vibWavPatt[i]  > 10) vibWavPatt[i]  = 0;
      }

      for (int16_t k = 0; k < 16; k++) {
        for (int16_t j = 0; j < 64; j++) {
          uint8_t routeBuffer[RouteAutoStorage::kLaneCount];
          reader.readBytes(routeBuffer, RouteAutoStorage::kLaneCount);
          routeAuto.importStep(k, j, routeBuffer);
        }
      }

      for (int16_t i = 0; i < 16; i++) {
        reader.readBytes(keyChange[i], 32);
        for (int16_t j = 0; j < 32; j++) {
          if (keyChange[i][j] > 15) keyChange[i][j] = 15;
        }
      }
      for (int16_t i = 0; i < 16; i++) {
        keyRoot[i] = getSd();
        if (keyRoot[i] > 15) keyRoot[i] = 15;
      }

      for (int16_t i = 0; i < 16; i++) {
        reader.readBytes(kitVariation[i], 10);
        for (int16_t j = 0; j < 10; j++) {
          if (kitVariation[i][j] > 3) kitVariation[i][j] = 0;
        }
      }

      reader.readBytes(musicModes, 16);
      for (int16_t i = 0; i < 16; i++) {
        musicModes[i] = constrain(musicModes[i], 0, 9);
      }

      for (int16_t i = 0; i < 16; i++) {
        uint8_t maskLow = getSd();
        uint8_t maskHigh = getSd();
        sceneValidMask[i] = maskLow | (static_cast<uint16_t>(maskHigh) << 8);

        for (int16_t j = 0; j < 10; j++) {
          SceneState scene = scenes.load(i, j);
          uint8_t muteLow = getSd();
          uint8_t muteHigh = getSd();
          (void)muteLow;
          (void)muteHigh;
          uint8_t soloSlot = getSd();
          (void)soloSlot;
          uint8_t mix = getSd();
          if (mix > 9) mix = 9;
          scene.mixVol = mix;
          uint8_t phones = getSd();
          if (phones > 9) phones = 9;
          scene.phonesVol = phones;
          uint8_t routing[7];
          reader.readBytes(routing, sizeof(routing));
          uint8_t flags = getSd();
          bool hasVolumes = (flags & 0x80) != 0;
          bool hasMultipliers = (flags & 0x40) != 0;
          bool hasDecays = (flags & 0x20) != 0;

          uint8_t fallbackKitIndex = 0;
          if (!hasVolumes || !hasMultipliers || !hasDecays) {
            uint8_t baseKit = kit[i];
            if (baseKit > 9) {
              baseKit %= 10;
            }
            uint8_t variation = kitVariation[i][baseKit];
            uint8_t kitIndex = baseKit + static_cast<uint8_t>(variation * 10);
            if (kitIndex >= 40) {
              kitIndex = baseKit;
            }
            fallbackKitIndex = kitIndex;
          }

          if (hasVolumes) {
            for (uint8_t k = 0; k < SCENE_VOLUME_COUNT; k++) {
              uint8_t value = getSd();
              if (value > 15) {
                value = 15;
              }
              scene.volumeFaders[k] = value;
            }
          } else {
            for (uint8_t k = 0; k < SCENE_VOLUME_COUNT; k++) {
              uint8_t raw = opl[fallbackKitIndex][SCENE_VOLUME_INDICES[k]];
              scene.volumeFaders[k] = (raw > 15) ? static_cast<uint8_t>(15) : raw;
            }
          }

          if (hasMultipliers) {
            for (uint8_t k = 0; k < SCENE_MULT_COUNT; k++) {
              uint8_t value = getSd();
              if (value > 15) {
                value = 15;
              }
              scene.multiplierFaders[k] = value;
            }
          } else {
            for (uint8_t k = 0; k < SCENE_MULT_COUNT; k++) {
              uint8_t raw = opl[fallbackKitIndex][SCENE_MULT_INDICES[k]];
              scene.multiplierFaders[k] = (raw > 15) ? static_cast<uint8_t>(15) : raw;
            }
          }

          if (hasDecays) {
            for (uint8_t k = 0; k < SCENE_DECAY_COUNT; k++) {
              uint8_t value = getSd();
              if (value > 15) {
                value = 15;
              }
              scene.decayFaders[k] = value;
            }
          } else {
            for (uint8_t k = 0; k < SCENE_DECAY_COUNT; k++) {
              uint8_t raw = opl[fallbackKitIndex][SCENE_DECAY_INDICES[k]];
              scene.decayFaders[k] = (raw > 15) ? static_cast<uint8_t>(15) : raw;
            }
          }
          scenes.store(i, j, scene);
        }
      }

      #undef getSd
      f.close();
    }
  }

  // Reset bend automation state so the newly loaded data starts from a clean slate.
  resetBendAutomation();

  /////////////////////////
  /////     CHAIN     /////
  /////////////////////////
  {
    char path[96];
    makePath(path, sizeof(path),
             "/SONGS/BANK%u/SONG%u/CHAIN.BB",
             folderIdx,
             songIdx);

    File f = SD.open(path, FILE_READ);
    if (f) {
      SdByteReader reader; reader.begin(f);
      #ifdef getSd
      #undef getSd
      #endif
      #define getSd() reader.get()

      pattChainLength = getSd();
      pattChainStep   = getSd();
      reader.readBytes(pattChain, sizeof(pattChain));

      #undef getSd
      f.close();
    }
  }

  // reload first patt in chain (unchanged)
  currentPattern = 0;
  pattChainStep = 0;
  if (pattChainLength) currentPattern = pattChain[pattChainStep];
  if (!pattChainLength) {
    currentPattern = EEPROMread(25);
    if (currentPattern > 15) currentPattern = 0;
  }
  currentKit = kit[currentPattern];
  updateCurrentKit();

  for (int16_t i = 0; i < 152; i++) {
    oplLast[i] = (opl[currentKit][i] > 0) ? 0 : 1;
  }

  /////////////////////////
  /////    NUDGING    /////
  /////////////////////////
  {
    char path[96];
    for (int16_t i = 0; i < 16; i++) {
      makePath(path, sizeof(path),
               "/SONGS/BANK%u/SONG%u/NUDG%u.BB",
               folderIdx,
               songIdx,
               static_cast<uint16_t>(i + 1));

      memset(nudgeDelay[i], 0, sizeof(nudgeDelay[i]));

      File f = SD.open(path, FILE_READ);
      if (f) {
        SdByteReader reader; reader.begin(f);
        #ifdef getSd
        #undef getSd
        #endif
        #define getSd() reader.get()

        for (int16_t j = 0; j < 5; j++) {
          reader.readBytes(nudgeDelay[i][j], 64);
        }

        #undef getSd
        f.close();
      }
    }
  }

  endSD();
  tremWav = tremWavPatt[currentPattern];
  vibWav  = vibWavPatt[currentPattern];
  key     = keyRoot[currentPattern];
  showPattern();
  showMode();
  mx.setPoint(7, 4, run); // keep your LED quirk fix
}

// ======= DROP-IN REPLACEMENT: loadKit() ==============================
void loadKit(uint8_t bank, uint8_t kit) {
  char base[64];
  const uint16_t folderIdx = static_cast<uint16_t>(currentFolder + 1);
  const uint16_t songIdx   = static_cast<uint16_t>(currentSong + 1);
  const uint16_t bankIdx   = static_cast<uint16_t>(bank + 1);
  const uint16_t kitIdx    = static_cast<uint16_t>(kit + 1);
  const int16_t bankOffset = static_cast<int16_t>(bank) * 10;
  const int16_t kitSlot    = bankOffset + kit;
  uint8_t* const kitBase   = opl[kitSlot];
  // Decide sandbox path once
  makePath(base, sizeof(base),
           "/SONGS/BANK%u/SONG%u/KITS",
           folderIdx,
           songIdx);

  bool sandBoxed = false;
  { File f = SD.open(base, FILE_READ); if (f) { sandBoxed = true; f.close(); } }

  // -------- DRUM --------
  char path[96];
  File f;
  if (sandBoxed) {
    makePath(path, sizeof(path),
             "/SONGS/BANK%u/SONG%u/KITS/BANK%u/DRUM%u.BB",
             folderIdx,
             songIdx,
             bankIdx,
             kitIdx);
    f = SD.open(path, FILE_READ);
    if (!f) {
      makePath(path, sizeof(path),
               "/KITS/BANK%u/DRUM%u.BB",
               bankIdx,
               kitIdx);
      f = SD.open(path, FILE_READ);
    }
  } else {
    makePath(path, sizeof(path),
             "/KITS/BANK%u/DRUM%u.BB",
             bankIdx,
             kitIdx);
    f = SD.open(path, FILE_READ);
  }

  if (f) {
    SdByteReader reader; reader.begin(f);
    reader.readBytes(kitBase, 36);
    reader.readBytes(kitBase + 148, 4);
    f.close();
  }

  for (int16_t i = 0; i < 10; i++) {
    opl[bankOffset + i][0] = 0;  // close gates
  }

  // -------- SYNTH --------
  if (sandBoxed) {
    makePath(path, sizeof(path),
             "/SONGS/BANK%u/SONG%u/KITS/BANK%u/SYNT%u.BB",
             folderIdx,
             songIdx,
             bankIdx,
             kitIdx);
    f = SD.open(path, FILE_READ);
    if (!f) {
      makePath(path, sizeof(path),
               "/KITS/BANK%u/SYNT%u.BB",
               bankIdx,
               kitIdx);
      f = SD.open(path, FILE_READ);
    }
  } else {
    makePath(path, sizeof(path),
             "/KITS/BANK%u/SYNT%u.BB",
             bankIdx,
             kitIdx);
    f = SD.open(path, FILE_READ);
  }

  if (f) {
    SdByteReader reader; reader.begin(f);
    reader.readBytes(kitBase + 36, 112);
    f.close();
  }
}
