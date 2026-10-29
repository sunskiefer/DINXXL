#if defined(__MK20DX256__)  // Code specific to Teensy 3.1/3.2

#include "vgmStream.h"

const char folderName1[] = "/VGM/1";
const char folderName2[] = "/VGM/2";
const char folderName3[] = "/VGM/3";
const char folderName4[] = "/VGM/4";
const char folderName5[] = "/VGM/5";
const char folderName6[] = "/VGM/6";
const char folderName7[] = "/VGM/7";
const char folderName8[] = "/VGM/8";
const char folderName9[] = "/VGM/9";
const char folderName10[] = "/VGM/10";
const int maxFiles = 16;

const uint32_t offset = 0x40;

byte* currentBuffer;

String fileNames[maxFiles];  // Array to store file names

bool isVgmFileName(const String& fileName) {
  if (!fileName.length() || fileName.startsWith(".") ||
      fileName.startsWith("_") || fileName.startsWith("~") ||
      fileName.startsWith("$") || fileName.startsWith("#")) {
    return false;
  }
  String lower = fileName;
  lower.toLowerCase();
  if (lower == "thumbs.db" || lower == "desktop.ini" ||
      lower == "indexervolumeguid" || lower == "wpsettings.dat" ||
      lower.endsWith(".tmp") || lower.endsWith(".temp") ||
      lower.endsWith(".bak") || lower.endsWith(".swp") ||
      lower.endsWith(".swo") || lower.endsWith("~")) {
    return false;
  }
  return lower.endsWith(".vgm") || lower.endsWith(".vgz");
}

namespace {
uint32_t g_waitSampleRemainderUs = 0;

inline void resetWaitTimer() {
  g_waitSampleRemainderUs = 0;
}

inline void waitForSamples(uint32_t samples) {
  if (!samples) {
    return;
  }

  constexpr uint32_t kSampleRate = 44100;
  uint64_t scaled = static_cast<uint64_t>(samples) * 1000000ULL + g_waitSampleRemainderUs;
  uint32_t waitMicros = static_cast<uint32_t>(scaled / kSampleRate);
  g_waitSampleRemainderUs = static_cast<uint32_t>(scaled % kSampleRate);

  if (!waitMicros) {
    return;
  }

  const uint32_t start = micros();
  while (static_cast<uint32_t>(micros() - start) < waitMicros) {
    yield();
  }
}
}  // namespace

int16_t countFiles() {
  int16_t count = 0;
  File dir;
  switch (vgmFolder) {
    case 0: dir = SD.open(folderName1); break;
    case 1: dir = SD.open(folderName2); break;
    case 2: dir = SD.open(folderName3); break;
    case 3: dir = SD.open(folderName4); break;
    case 4: dir = SD.open(folderName5); break;
    case 5: dir = SD.open(folderName6); break;
    case 6: dir = SD.open(folderName7); break;
    case 7: dir = SD.open(folderName8); break;
    case 8: dir = SD.open(folderName9); break;
    case 9: dir = SD.open(folderName10); break;
  }

  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      // No more files
      break;
    }
    if (!entry.isDirectory()) {
      char entryName[96];
      entry.getName(entryName, sizeof(entryName));
      String fileName = entryName;
      if (isVgmFileName(fileName)) {
        fileNames[count] = fileName;
        count++;
        if (count >= maxFiles) break;
      }
    }
    entry.close();
  }
  dir.close();

  // Sort file names alphabetically
  for (int i = 0; i < count - 1; i++) {
    for (int j = i + 1; j < count; j++) {
      if (fileNames[i].compareTo(fileNames[j]) > 0) {
        String temp = fileNames[i];
        fileNames[i] = fileNames[j];
        fileNames[j] = temp;
      }
    }
  }
  Serial.print(count);
  Serial.println(" vgm files found");
  return count;
}



void vgm() {

  checkCard();
  startSD();
  fileCount = countFiles();
  endSD();
  showVgmFiles();
}

void showVgmFiles() {
  //now we show how many are avialable
  clearBottom();
  clearTop();
  setLed(vgmFolder + 16, 1);
  for (int i = 0; i < fileCount; i++) {
    setLed(i, 1);
    delay(10);
  }
}

void showPlaying(byte number) {
  //now we show how many are avialable
  clearBottom();
  clearTop();
  setLed(vgmFolder + 16, 1);
  for (int i = 1; i <= fileCount; i++) {
    setLed(number, 1);
  }
}


uint32_t combineLittleEndianBytes(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
  uint32_t result = (uint32_t)b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
  return result;
}

inline bool isOpl2CompatibleVgmWrite(uint8_t command) {
  return command == 0x5A ||  // YM3812 / OPL2
         command == 0x5B ||  // YM3526 / OPL
         command == 0x5C ||  // Y8950, FM-compatible writes only
         command == 0xAA ||  // second YM3812 instance
         command == 0xAB ||  // second YM3526 instance
         command == 0xAC;    // second Y8950 instance
}

inline bool isOpl3VgmWrite(uint8_t command) {
  return command == 0x5E || command == 0x5F;
}

inline uint8_t opl2CompatibilityData(uint8_t address, uint8_t data) {
  // Blast Beats keeps the YMF262 in OPL3 mode. In that mode the high nibble of
  // C0-C8 enables the output buses. OPL2 VGM files usually write only the OPL2
  // feedback/connection bits here, so force all outputs on or the channel mutes.
  if (address >= 0xC0 && address <= 0xC8) {
    return data | 0xF0;
  }
  return data;
}

inline void sendOpl2CompatibleWrite(uint8_t command, uint8_t address, uint8_t data) {
  const bool secondChip = command >= 0xAA;
  data = opl2CompatibilityData(address, data);
  if (secondChip) {
    opl2.writeHigh(address, data);
  } else {
    opl2.write(address, data);
  }
}

inline void sendOpl3Write(uint8_t command, uint8_t address, uint8_t data) {
  if (command == 0x5F) {
    opl2.writeHigh(address, data);
  } else {
    opl2.write(address, data);
  }
}

void playFile(int fileNumber) {
  checkCard();
  showPlaying(fileNumber);
  mx.setPoint(7, 4, 1);  //run LED on
  startSD();


Serial.print("playing track");
Serial.println(fileNumber);

  String filePath;
  switch (vgmFolder) {
    case 0: filePath = String(folderName1) + "/" + String(fileNames[fileNumber]); break;
    case 1: filePath = String(folderName2) + "/" + String(fileNames[fileNumber]); break;
    case 2: filePath = String(folderName3) + "/" + String(fileNames[fileNumber]); break;
    case 3: filePath = String(folderName4) + "/" + String(fileNames[fileNumber]); break;
    case 4: filePath = String(folderName5) + "/" + String(fileNames[fileNumber]); break;
    case 5: filePath = String(folderName6) + "/" + String(fileNames[fileNumber]); break;
    case 6: filePath = String(folderName7) + "/" + String(fileNames[fileNumber]); break;
    case 7: filePath = String(folderName8) + "/" + String(fileNames[fileNumber]); break;
    case 8: filePath = String(folderName9) + "/" + String(fileNames[fileNumber]); break;
    case 9: filePath = String(folderName10) + "/" + String(fileNames[fileNumber]); break;
  }

  VGMStream stream;
  if (!stream.open(filePath)) {
    Serial.println("failed to open vgm file");
  } else {
    bool headerOk = true;
    uint8_t header[64];
    if (stream.readBytes(header, sizeof(header)) != sizeof(header)) {
      Serial.println("invalid vgm header");
      headerOk = false;
    }

    uint32_t loopPoint = 0;
    uint32_t dataOffset = 0;
    if (headerOk) {
      loopPoint = combineLittleEndianBytes(header[0x1C], header[0x1D], header[0x1E], header[0x1F]);
      if (loopPoint) {
        loopPoint += 0x1C;
      }

      dataOffset = combineLittleEndianBytes(header[0x34], header[0x35], header[0x36], header[0x37]);
      if (dataOffset == 0) {
        dataOffset = 0x40;
      } else {
        dataOffset += 0x34;
      }

      if (!stream.seek(dataOffset)) {
        Serial.println("unable to reach vgm data");
        headerOk = false;
      }
    }

    if (headerOk) {
      auto skipBytes = [&](uint32_t count) {
        uint8_t discard[64];
        while (count > 0) {
          size_t chunk = count > sizeof(discard) ? sizeof(discard) : count;
          if (stream.readBytes(discard, chunk) != chunk) {
            return false;
          }
          count -= chunk;
        }
        return true;
      };

      vgmState = 1;
      int vgmDebounce = 0;
      resetWaitTimer();

      while (vgmState) {
        vgmDebounce++;
        if (vgmDebounce > 40) {
          vgmDebounce = 0;
          if (!digitalRead(RUN_PIN)) {
            vgmState = 0;
            break;
          }
        }

        delayMicroseconds(21);

        int rawValue = stream.read();
        if (rawValue < 0) {
          if (loopPoint) {
            if (!stream.seek(loopPoint)) {
              vgmState = 0;
              break;
            }
            continue;
          }
          vgmState = 0;
          break;
        }

        uint8_t value = static_cast<uint8_t>(rawValue);

        if (value == 0x66) {
          vgmState = 0;
          break;

        } else if (isOpl2CompatibleVgmWrite(value) || isOpl3VgmWrite(value)) {
          int rawAddress = stream.read();
          if (rawAddress < 0) {
            if (loopPoint && stream.seek(loopPoint)) {
              continue;
            }
            vgmState = 0;
            break;
          }
          int rawData = stream.read();
          if (rawData < 0) {
            if (loopPoint && stream.seek(loopPoint)) {
              continue;
            }
            vgmState = 0;
            break;
          }
          uint8_t address = static_cast<uint8_t>(rawAddress);
          uint8_t data = static_cast<uint8_t>(rawData);
          if (isOpl2CompatibleVgmWrite(value)) {
            sendOpl2CompatibleWrite(value, address, data);
          } else {
            sendOpl3Write(value, address, data);
          }

        } else if (value == 0x61) {
          int d1 = stream.read();
          int d2 = stream.read();
          if (d1 < 0 || d2 < 0) {
            if (loopPoint && stream.seek(loopPoint)) {
              continue;
            }
            vgmState = 0;
            break;
          }
          pause(static_cast<uint8_t>(d1), static_cast<uint8_t>(d2));

        } else if (value >= 0x70 && value <= 0x7F) {
          waitForSamples(static_cast<uint32_t>((value & 0x0F) + 1));

        } else if (value == 0x67) {
          int ctrl = stream.read();
          int typeByte = stream.read();
          uint8_t lenBytes[4];
          if (ctrl < 0 || typeByte < 0 || stream.readBytes(lenBytes, sizeof(lenBytes)) != sizeof(lenBytes)) {
            vgmState = 0;
            break;
          }
          if (ctrl != 0x66) {
            Serial.println("Unexpected data block marker");
            vgmState = 0;
            break;
          }
          uint32_t dataLength = static_cast<uint32_t>(lenBytes[0]) |
                                (static_cast<uint32_t>(lenBytes[1]) << 8) |
                                (static_cast<uint32_t>(lenBytes[2]) << 16) |
                                (static_cast<uint32_t>(lenBytes[3]) << 24);
          if (!skipBytes(dataLength)) {
            vgmState = 0;
            break;
          }

        } else if (value == 0x68 || value == 0x69) {
          int ctrl = stream.read();
          int chipType = stream.read();
          uint8_t readOffset[3];
          uint8_t writeOffset[3];
          uint8_t sizeBytes[3];
          if (ctrl < 0 || chipType < 0 ||
              stream.readBytes(readOffset, sizeof(readOffset)) != sizeof(readOffset) ||
              stream.readBytes(writeOffset, sizeof(writeOffset)) != sizeof(writeOffset) ||
              stream.readBytes(sizeBytes, sizeof(sizeBytes)) != sizeof(sizeBytes)) {
            vgmState = 0;
            break;
          }
          if (ctrl != 0x66) {
            Serial.println("Unexpected PCM block marker");
            vgmState = 0;
            break;
          }
          // These PCM RAM write commands don't embed payload data. We currently
          // ignore them but need to consume their parameter bytes to stay in sync.
          (void)chipType;

        } else if (value == 0x62) {
          waitForSamples(735);

        } else if (value == 0x63) {
          waitForSamples(882);
        } else {
          Serial.print("Unhandled VGM cmd: 0x");
          Serial.println(value, HEX);
          vgmState = 0;
          break;
        }
      }

      vgmState = 0;
    }

    stream.close();
  }

  //muting all voices

  for (int i = 0; i < 24; i++) {
    opl2.setVolume(i, OPERATOR1, 63);
    opl2.setVolume(i, OPERATOR2, 63);
  }
  endSD();
  showVgmFiles();
  mx.setPoint(7, 4, 0);
}

void opl3Send(byte address, byte data) {
  // Implementation for sending address and data to OPL chip (port 0)
  // Replace with your own code
  opl2.write(address, data);
}

void opl3SendHigh(byte address, byte data) {
  // Implementation for sending address and data to OPL3
  // Replace with your own code
  opl2.writeHigh(address, data);
}

void pause(byte data1, byte data2) {
  uint32_t samples = static_cast<uint32_t>(data1) | (static_cast<uint32_t>(data2) << 8);
  waitForSamples(samples);
}


#endif
