#pragma once

#include <Arduino.h>
#include <SD.h>

// These globals/functions are provided elsewhere in the project.
extern uint8_t routingPreset[10][7];
extern uint8_t EEPROMArray[2048];
extern bool EEPROMChanged;
extern uint32_t EEPROMLastChangeMs;
extern volatile bool run;
extern uint8_t sdSessionDepth;
extern File sdFile;
extern bool sdCardReady;

void startSD();
void endSD();
void buffer(uint8_t data);
void fillPage();
void bufferBytes(const uint8_t* data, size_t len);
void bufferFill(uint8_t value, size_t count);
uint8_t getSd();
void checkCard();

inline uint8_t EEPROMread(int16_t address) {
#if defined(__MK20DX256__)  // Teensy 3.1/3.2
  return EEPROM.read(address);
#elif defined(STM32F1) || defined(STM32F4)
  return EEPROMArray[address];
#endif
}

inline void EEPROMwrite(int16_t address, uint8_t data) {
#if defined(__MK20DX256__)
  EEPROM.update(address, data);
#elif defined(STM32F1) || defined(STM32F4)
  if (EEPROMArray[address] == data) return;
  EEPROMArray[address] = data;
  EEPROMChanged = true;
  EEPROMLastChangeMs = millis();
#endif
}

inline void EEPROMupdate(int16_t address, uint8_t data) {
#if defined(__MK20DX256__)
  EEPROM.update(address, data);
#elif defined(STM32F1) || defined(STM32F4)
  if (EEPROMArray[address] == data) return;
  EEPROMArray[address] = data;
  EEPROMChanged = true;
  EEPROMLastChangeMs = millis();
#endif
}

inline void routePresetStore() {
  EEPROMupdate(39, 123);
  const int startAddress = 40;  // EEPROM start address for storing routingPreset
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 7; j++) {
      const int address = startAddress + (i * 7) + j;
      EEPROMupdate(address, routingPreset[i][j]);
    }
  }
}

inline void routePresetLoad() {
  const int startAddress = 40;  // EEPROM start address for loading routingPreset
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 7; j++) {
      const int address = startAddress + (i * 7) + j;
      routingPreset[i][j] = EEPROMread(address);
    }
  }
}

inline void EEPROMSave() {
#if defined(STM32F1) || defined(STM32F4)
  startSD();
  static const char finalPath[] = "/EEPROM.BB";

  // A settings file is recoverable; a journal file that survives a reset is
  // not. Write only the boot-compatible final filename, in SD-sector chunks.
  SD.remove(finalPath);
  File settings = SD.open(finalPath, FILE_WRITE);
  bool ok = static_cast<bool>(settings);
  for (size_t offset = 0; ok && offset < sizeof(EEPROMArray); offset += 512) {
    ok = settings.write(EEPROMArray + offset, 512) == 512;
  }
  if (settings) {
    settings.flush();
    settings.close();
  }

  File verify = SD.open(finalPath, FILE_READ);
  ok = ok && verify && verify.size() == sizeof(EEPROMArray);
  if (verify) verify.close();
  if (!ok) SD.remove(finalPath);  // next boot safely regenerates defaults

  // Remove journals left by earlier 7.0 test builds.
  SD.remove("/SYSTEM/EEPROM.TMP");
  SD.remove("/SYSTEM/EEPROM.BAK");
  SD.remove("/EEPROM.TMP");
  SD.remove("/EEPROM.BAK");
  SD.rmdir("/SYSTEM");  // only succeeds when the directory is empty
  endSD();
  if (ok) {
    EEPROMChanged = false;
    EEPROMLastChangeMs = 0;
  }
#endif
}

inline void EEPROMLoad() {
#if defined(STM32F1) || defined(STM32F4)
  checkCard();
  startSD();

  // Prefer the final file, but recover a complete journal produced by an early
  // 7.0 test build. Empty/partial files are always ignored.
  static const char* const candidates[] = {
    "/EEPROM.BB", "/SYSTEM/EEPROM.BAK", "/SYSTEM/EEPROM.TMP", "/EEPROM.BAK"
  };
  bool loadedRecovery = false;
  for (size_t i = 0; i < (sizeof(candidates) / sizeof(candidates[0])); ++i) {
    sdFile = SD.open(candidates[i], FILE_READ);
    loadedRecovery = i != 0;
    if (sdFile && sdFile.size() == sizeof(EEPROMArray)) break;
    if (sdFile) sdFile.close();
    loadedRecovery = false;
  }
  if (sdFile && sdFile.size() == sizeof(EEPROMArray)) {
    const size_t bytesRead = sdFile.read(EEPROMArray, sizeof(EEPROMArray));
    if (bytesRead != sizeof(EEPROMArray)) {
      memset(EEPROMArray, 0, sizeof(EEPROMArray));
    }
  }
  if (sdFile) {
    sdFile.close();
  }
  SD.remove("/SYSTEM/EEPROM.TMP");
  SD.remove("/SYSTEM/EEPROM.BAK");
  SD.rmdir("/SYSTEM");
  SD.remove("/EEPROM.TMP");
  SD.remove("/EEPROM.BAK");

  endSD();
  // Promote recovered settings later, once normal startup is fully complete.
  EEPROMChanged = loadedRecovery;
  EEPROMLastChangeMs = loadedRecovery ? millis() : 0;
#endif
}

inline void serviceEEPROMAutosave() {
#if defined(STM32F1) || defined(STM32F4)
  constexpr uint32_t kEEPROMAutosaveDelayMs = 1000;

  if (!EEPROMChanged) {
    return;
  }
  if (run || sdSessionDepth != 0) {
    return;
  }
  if ((millis() - EEPROMLastChangeMs) < kEEPROMAutosaveDelayMs) {
    return;
  }

  EEPROMSave();
#endif
}
