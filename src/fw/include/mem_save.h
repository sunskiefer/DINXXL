

// Ensure SD helpers are available for directory checks.
#include <SD.h>
#include <string.h>
#include "scene_types.h"
#include "packed_storage.h"

extern SceneBank scenes;

static const uint32_t kDrumPatternFileBytes = 161;
static const uint32_t kSynthPatternFileBytes = 263;
static const uint32_t kAutoFileBytes = 36404;
static const uint32_t kChainFileBytes = 258;
static const uint32_t kNudgeFileBytes = 320;
static const uint32_t kDrumKitFileBytes = 40;
static const uint32_t kSynthKitFileBytes = 112;

static bool makeSiblingPathWithExtension(const char* path, const char* extension,
                                         char* out, size_t outSize) {
  const char* slash = strrchr(path, '/');
  const char* nameStart = slash ? slash + 1 : path;
  const char* dot = strrchr(nameStart, '.');
  const size_t prefixLen = static_cast<size_t>(nameStart - path);
  const size_t baseLen = dot ? static_cast<size_t>(dot - nameStart) : strlen(nameStart);
  const size_t extensionLen = strlen(extension);

  if (!baseLen || baseLen > 8 || extensionLen > 3) {
    return false;
  }
  if (prefixLen + baseLen + 1 + extensionLen + 1 > outSize) {
    return false;
  }

  memcpy(out, path, prefixLen);
  memcpy(out + prefixLen, nameStart, baseLen);
  out[prefixLen + baseLen] = '.';
  memcpy(out + prefixLen + baseLen + 1, extension, extensionLen);
  out[prefixLen + baseLen + 1 + extensionLen] = '\0';
  return true;
}

static bool getFileSizeBytes(const char* path, uint32_t* sizeOut) {
  File f = SD.open(path, FILE_READ);
  if (!f) {
    return false;
  }

  *sizeOut = f.size();
  f.close();
  return true;
}

static bool fileHasSize(const char* path, uint32_t expectedBytes) {
  uint32_t actualBytes = 0;
  return getFileSizeBytes(path, &actualBytes) && actualBytes == expectedBytes;
}

static bool filesAreIdentical(const char* firstPath, const char* secondPath) {
  File first = SD.open(firstPath, FILE_READ);
  File second = SD.open(secondPath, FILE_READ);
  if (!first || !second || first.size() != second.size()) {
    if (first) first.close();
    if (second) second.close();
    return false;
  }

  uint8_t secondBuffer[64];
  bool identical = true;
  while (identical) {
    const int firstCount = first.read(buffy, sizeof(secondBuffer));
    const int secondCount = second.read(secondBuffer, sizeof(secondBuffer));
    if (firstCount != secondCount || firstCount < 0) {
      identical = false;
      break;
    }
    if (firstCount == 0) break;
    identical = memcmp(buffy, secondBuffer, static_cast<size_t>(firstCount)) == 0;
  }

  first.close();
  second.close();
  return identical;
}

#if !defined(__MK20DX256__)
static bool copyFileContents(const char* sourcePath, const char* destPath) {
  File source = SD.open(sourcePath, FILE_READ);
  if (!source) {
    return false;
  }

  SD.remove(destPath);
  File dest = SD.open(destPath, FILE_WRITE);
  if (!dest) {
    source.close();
    return false;
  }

  bool ok = true;
  while (ok) {
    const int bytesRead = source.read(buffy, sizeof(buffy));
    if (bytesRead < 0) {
      ok = false;
      break;
    }
    if (bytesRead == 0) {
      break;
    }
    if (dest.write(buffy, static_cast<size_t>(bytesRead)) != static_cast<size_t>(bytesRead)) {
      ok = false;
    }
  }

  dest.flush();
  dest.close();
  source.close();

  if (!ok) {
    SD.remove(destPath);
  }
  return ok;
}
#endif

static bool moveFileReplacingNoRollback(const char* sourcePath, const char* destPath) {
#if defined(__MK20DX256__)
  return SD.rename(sourcePath, destPath);
#else
  if (!copyFileContents(sourcePath, destPath)) {
    return false;
  }
  SD.remove(sourcePath);
  return true;
#endif
}

static bool restoreBackupFile(const char* backupPath, const char* finalPath) {
  SD.remove(finalPath);
  return moveFileReplacingNoRollback(backupPath, finalPath);
}

static bool replaceFileWithVerifiedTemp(const char* finalPath, const char* tempPath,
                                        uint32_t expectedBytes) {
  char backupPath[96];
  if (!makeSiblingPathWithExtension(finalPath, "BAK", backupPath, sizeof(backupPath))) {
    SD.remove(tempPath);
    return false;
  }

  const bool hadOriginal = SD.exists(finalPath);
  uint32_t originalBytes = 0;

  if (hadOriginal) {
    if (!getFileSizeBytes(finalPath, &originalBytes)) {
      SD.remove(tempPath);
      return false;
    }

    // A no-change save needs no backup or replacement. This avoids dozens of
    // FAT updates and copy passes when the user saves an unchanged song.
    if (originalBytes == expectedBytes && filesAreIdentical(finalPath, tempPath)) {
      SD.remove(tempPath);
      return true;
    }

    SD.remove(backupPath);

#if defined(__MK20DX256__)
    if (!SD.rename(finalPath, backupPath)) {
      SD.remove(tempPath);
      return false;
    }
#else
    if (!copyFileContents(finalPath, backupPath) || !fileHasSize(backupPath, originalBytes)) {
      SD.remove(tempPath);
      SD.remove(backupPath);
      return false;
    }
    SD.remove(finalPath);
#endif
  }

  if (!moveFileReplacingNoRollback(tempPath, finalPath) || !fileHasSize(finalPath, expectedBytes)) {
    SD.remove(finalPath);
    if (hadOriginal) {
      restoreBackupFile(backupPath, finalPath);
    }
    return false;
  }

  SD.remove(backupPath);
  return true;
}

static bool beginAtomicWrite(const char* finalPath) {
  char tempPath[96];
  if (!makeSiblingPathWithExtension(finalPath, "TMP", tempPath, sizeof(tempPath))) {
    return false;
  }

  SD.remove(tempPath);
  buffCounter = 0;
  sdWriteOk = true;
  sdFile = SD.open(tempPath, FILE_WRITE);
  return sdFile;
}

static bool commitAtomicWrite(const char* finalPath, uint32_t expectedBytes) {
  char tempPath[96];
  if (!makeSiblingPathWithExtension(finalPath, "TMP", tempPath, sizeof(tempPath))) {
    if (sdFile) {
      sdFile.close();
    }
    return false;
  }

  fillPage();
  sdFile.flush();
  sdFile.close();

  if (!sdWriteOk || !fileHasSize(tempPath, expectedBytes)) {
    SD.remove(tempPath);
    return false;
  }

  return replaceFileWithVerifiedTemp(finalPath, tempPath, expectedBytes);
}

// ================== DROP-IN: saveKit() ==================
void saveKit() {
  startSD();

  for (int16_t i = 0; i < 10; i++) {
    saveKitData(0, i);
    saveKitData(1, i);
    saveKitData(2, i);
    saveKitData(3, i);
  }

  endSD();
  flashAll();
  showMode();
}

// ================== DROP-IN: save() ==================
void save() {
  startSkip = 2;
  startSD();

  char path[96];
  bool saveOk = true;

  // -------- DRUM PATTERN --------
  for (int16_t i = 0; i < 16; i++) {

    buffCounter = 0;

    makePath(path, sizeof(path),
             "/SONGS/BANK%u/SONG%u/PATD%u.BB",
             (uint16_t)(currentFolder + 1),
             (uint16_t)(currentSong + 1),
             (uint16_t)(i + 1));

    if (beginAtomicWrite(path)) {
      for (int16_t j = 0; j < 2; j++) {
        bufferBytes(pattern[i][j], 64);
      }

      buffer(pages[i][0]);
      buffer(lastStepPoly[i][0]);
      buffer(swingSetting[i]);
      buffer(vibratoRate[i]);
      buffer(tremRate[i]);
      buffer(kit[i]);

      for (int j = 0; j < 6; j++) buffer(offsetOct[i][j]);

      bufferBytes(lastStepPoly[i], 10);
      for (int16_t j = 0; j < 10; j++) buffer(static_cast<uint8_t>(pages[i][j]));

      buffer(40); // file version

      saveOk = commitAtomicWrite(path, kDrumPatternFileBytes) && saveOk;
    } else {
      saveOk = false;
    }
  }

  // -------- SYNTH PATTERN --------
  for (int16_t i = 0; i < 16; i++) {

    buffCounter = 0;

    makePath(path, sizeof(path),
             "/SONGS/BANK%u/SONG%u/PATS%u.BB",
             (uint16_t)(currentFolder + 1),
             (uint16_t)(currentSong + 1),
             (uint16_t)(i + 1));

    if (beginAtomicWrite(path)) {
      for (int16_t j = 0; j < 3; j++) {
        bufferBytes(synth[i][j], 64);
      }

      for (int j = 6; j < 10; j++) {
        if (offsetOct[i][j] == -1) buffer(3);
        else buffer(offsetOct[i][j]);
      }

      bufferBytes(synth[i][3], 64);

      buffer((uint8_t)(tremRateMultiplier[i] + 1));
      buffer((uint8_t)(vibratoRateMultiplier[i] + 1));
      buffer((uint8_t)(vibratoPower[i] + 1));

      saveOk = commitAtomicWrite(path, kSynthPatternFileBytes) && saveOk;
    } else {
      saveOk = false;
    }
  }

  // -------- AUTOMATION --------
  {
    buffCounter = 0;
    makePath(path, sizeof(path),
             "/SONGS/BANK%u/SONG%u/AUTO.BB",
             (uint16_t)(currentFolder + 1),
             (uint16_t)(currentSong + 1));

    if ((saveAuto) || (saveToNewSlot)) {
      if (beginAtomicWrite(path)) {
        bufferBytes(autoData, 3000);
        bufferFill(0, 6000);

        for (int16_t j = 0; j < 16; j++) {
          for (int16_t i = 0; i < 64; i++) {
            buffer(stutterSteps[j][i]);
            buffer(bendSteps[j][i]);
            buffer(freezeSteps[j][i]);
          }
        }

        buffer(lowByte(tempo));
        buffer(highByte(tempo));

        for (int16_t i = 0; i < 10; i++) {
          bufferBytes(prob[i], 10);
        }

        buffer(randomDepth[0]);

        uint8_t temp2 = 0;
        for (int16_t k = 0; k < 16; k++) {
          for (int16_t j = 0; j < 12; j++) {
            temp2 = 0;
            for (int16_t i = 0; i < 8; i++) {
              bitWrite(temp2, i, randomer[k][i + (j * 8)] ? 1 : 0);
            }
            buffer(temp2);
          }
        }

        for (int16_t i = 1; i < 16; i++) buffer(randomDepth[i]);

        for (int16_t k = 0; k < 16; k++) {
          for (int16_t j = 0; j < 32; j++) {
            buffer(vibAuto[k][j]);
            buffer(tremAuto[k][j]);
            buffer(swingAuto[k][j]);
            buffer(wiggleAuto[k][j]);
            buffer(kitAuto[k][j]);
            buffer(phonesAuto[k][j]);
            buffer(mixAuto[k][j]);
          }
        }

        bufferBytes(autoData, 3000);

        for (int16_t k = 0; k < 16; k++) {
          for (int16_t j = 0; j < 32; j++) {
            buffer(vibAutoWav[k][j]);
            buffer(tremAutoWav[k][j]);
          }
        }

        for (int16_t i = 1; i < 16; i++) {
          buffer(tremWavPatt[i]);
          buffer(vibWavPatt[i]);
        }

        for (int16_t k = 0; k < 16; k++) {
          for (int16_t j = 0; j < 64; j++) {
            uint8_t routeBuffer[RouteAutoStorage::kLaneCount];
            routeAuto.exportStep(k, j, routeBuffer);
            bufferBytes(routeBuffer, RouteAutoStorage::kLaneCount);
          }
        }

        for (int16_t i = 0; i < 16; i++) {
          bufferBytes(keyChange[i], 32);
        }
        bufferBytes(keyRoot, 16);

        for (int16_t i = 0; i < 16; i++) {
          bufferBytes(kitVariation[i], 10);
        }

        bufferBytes(musicModes, 16);

        for (int16_t i = 0; i < 16; i++) {
          buffer(lowByte(sceneValidMask[i]));
          buffer(highByte(sceneValidMask[i]));
          for (int16_t j = 0; j < 10; j++) {
            SceneState scene = scenes.load(i, j);
            const uint8_t mix = constrain(scene.mixVol, 0, static_cast<uint8_t>(9));
            const uint8_t phones = constrain(scene.phonesVol, 0, static_cast<uint8_t>(9));
            buffer(static_cast<uint8_t>(0));  // muteMask low placeholder
            buffer(static_cast<uint8_t>(0));  // muteMask high placeholder
            buffer(static_cast<uint8_t>(0));  // solo slot placeholder
            buffer(mix);
            buffer(phones);
            uint8_t routingStub[7] = {};
            bufferBytes(routingStub, sizeof(routingStub));
            buffer(static_cast<uint8_t>(0xE0));  // mark volumes + multipliers + decays present
            for (uint8_t k = 0; k < SCENE_VOLUME_COUNT; ++k) {
              uint8_t value = scene.volumeFaders[k];
              if (value > 15) {
                value = 15;
                scene.volumeFaders[k] = value;
              }
              buffer(value);
            }
            for (uint8_t k = 0; k < SCENE_MULT_COUNT; ++k) {
              uint8_t value = scene.multiplierFaders[k];
              if (value > 15) {
                value = 15;
                scene.multiplierFaders[k] = value;
              }
              buffer(value);
            }
            for (uint8_t k = 0; k < SCENE_DECAY_COUNT; ++k) {
              uint8_t value = scene.decayFaders[k];
              if (value > 15) {
                value = 15;
                scene.decayFaders[k] = value;
              }
              buffer(value);
            }
            scenes.store(i, j, scene);
          }
        }

        saveOk = commitAtomicWrite(path, kAutoFileBytes) && saveOk;
      } else {
        saveOk = false;
      }
    }
  }

  // -------- CHAIN --------
  {
    buffCounter = 0;
    makePath(path, sizeof(path),
             "/SONGS/BANK%u/SONG%u/CHAIN.BB",
             (uint16_t)(currentFolder + 1),
             (uint16_t)(currentSong + 1));

    if (beginAtomicWrite(path)) {
      buffer(pattChainLength);
      buffer(pattChainStep);
      bufferBytes(pattChain, sizeof(pattChain));
      saveOk = commitAtomicWrite(path, kChainFileBytes) && saveOk;
    } else {
      saveOk = false;
    }
  }

  // -------- NUDGING --------
  for (int16_t i = 0; i < 16; i++) {
    buffCounter = 0;

    makePath(path, sizeof(path),
             "/SONGS/BANK%u/SONG%u/NUDG%u.BB",
             (uint16_t)(currentFolder + 1),
             (uint16_t)(currentSong + 1),
             (uint16_t)(i + 1));

    if (beginAtomicWrite(path)) {
      for (int16_t j = 0; j < 5; j++) {
        bufferBytes(nudgeDelay[i][j], 64);
      }

      saveOk = commitAtomicWrite(path, kNudgeFileBytes) && saveOk;
    } else {
      saveOk = false;
    }
  }

  endSD();

  if (saveOk) {
    saveAuto = 0;
  }

  if (saveKitAlso) saveKit();
  else             flashAll();

  showMode();
}

// ================== DROP-IN: saveKitData() ==================
static bool ensureDirectoryExists(const char* path) {
  if (SD.exists(path)) {
    return true;
  }
  return SD.mkdir(path);
}

static bool ensureKitDirectories(uint8_t bank, bool sandBoxed) {
  if (sandBoxed) {
    char kitsDir[96];
    snprintf(kitsDir, sizeof(kitsDir),
             "/SONGS/BANK%u/SONG%u/KITS",
             static_cast<uint16_t>(currentFolder + 1),
             static_cast<uint16_t>(currentSong + 1));
    if (!ensureDirectoryExists(kitsDir)) {
      return false;
    }

	    char bankDir[96];
	    snprintf(bankDir, sizeof(bankDir),
	             "/SONGS/BANK%u/SONG%u/KITS/BANK%u",
	             static_cast<uint16_t>(currentFolder + 1),
	             static_cast<uint16_t>(currentSong + 1),
	             static_cast<uint16_t>(bank + 1));
    if (!ensureDirectoryExists(bankDir)) {
      return false;
    }
  } else {
    if (!ensureDirectoryExists("/KITS")) {
      return false;
    }

    char bankDir[32];
    snprintf(bankDir, sizeof(bankDir),
             "/KITS/BANK%u",
             static_cast<uint16_t>(bank + 1));
    if (!ensureDirectoryExists(bankDir)) {
      return false;
    }
  }
  return true;
}

void saveKitData(uint8_t bank, uint8_t kit) {
  buffCounter = 0;

  // Determine sandbox once by attempting to open folder
  char base[64];
  makePath(base, sizeof(base),
           "/SONGS/BANK%u/SONG%u/KITS",
           (uint16_t)(currentFolder + 1),
           (uint16_t)(currentSong + 1));

  bool sandBoxed = false;
  { File f = SD.open(base, FILE_READ); if (f) { sandBoxed = true; f.close(); } }

  if (sandBoxed) {
    if (!ensureKitDirectories(bank, true)) {
      sandBoxed = false;
      if (!ensureKitDirectories(bank, false)) {
        return;
      }
    }
  } else {
    if (!ensureKitDirectories(bank, false)) {
      return;
    }
  }

  // ----- DRUM -----
  char path[96];
  if (sandBoxed) {
    snprintf(path, sizeof(path),
             "/SONGS/BANK%u/SONG%u/KITS/BANK%u/DRUM%u.BB",
             (uint16_t)(currentFolder + 1),
             (uint16_t)(currentSong + 1),
             (uint16_t)(bank + 1),
             (uint16_t)(kit + 1));
  } else {
    snprintf(path, sizeof(path),
             "/KITS/BANK%u/DRUM%u.BB",
             (uint16_t)(bank + 1),
             (uint16_t)(kit + 1));
  }

  if (beginAtomicWrite(path)) {
    uint8_t* base = opl[kit + (bank * 10)];
    bufferBytes(base, 36);
    bufferBytes(base + 148, 4);
    commitAtomicWrite(path, kDrumKitFileBytes);
  }

  // ----- SYNTH -----
  buffCounter = 0;

  if (sandBoxed) {
    snprintf(path, sizeof(path),
             "/SONGS/BANK%u/SONG%u/KITS/BANK%u/SYNT%u.BB",
             (uint16_t)(currentFolder + 1),
             (uint16_t)(currentSong + 1),
             (uint16_t)(bank + 1),
             (uint16_t)(kit + 1));
  } else {
    snprintf(path, sizeof(path),
             "/KITS/BANK%u/SYNT%u.BB",
             (uint16_t)(bank + 1),
             (uint16_t)(kit + 1));
  }

  if (beginAtomicWrite(path)) {
    uint8_t* base = opl[kit + (bank * 10)];
    bufferBytes(base + 36, 112);
    commitAtomicWrite(path, kSynthKitFileBytes);
  }
}
