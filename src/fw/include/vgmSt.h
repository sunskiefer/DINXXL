#if defined(STM32F1) || defined(STM32F4)  // Code specific to STM32

#include "vgmStream.h"

const int16_t maxFiles = 16;
const uint32_t offset = 0x40;
const int MAX_FILE_COUNT = 16; // Maximum number of files to store
  String filenames[MAX_FILE_COUNT];
byte* currentBuffer;

String fileNames[maxFiles]; // Array to store file names

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
int16_t count=0;
  String fileName = String();

    fileName = "/VGM/";
    fileName += vgmFolder+1;

  File folder = SD.open(fileName);
  // File count and array
  // Print the list of files
  while (true) {
    File entry = folder.openNextFile();
    if (!entry) {
      // No more files in the folder
      break;
    }
    if (!entry.isDirectory()) {
      char entryName[96];
      entry.getName(entryName, sizeof(entryName));
      String fileName = entryName;
      if (isVgmFileName(fileName)) {
        // Append the full path to the file name and save it to the array
        filenames[count] = fileName;
        count++;
        // Check if the maximum file count has been reached
        if (count >= MAX_FILE_COUNT) {
   
          break;
        }
      }
    }
    entry.close();
  }
  folder.close();
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
  clearBottom(); clearTop();
  setLed(vgmFolder + 16, 1);
  for (int16_t i = 0; i < fileCount; i++) {
    setLed(i, 1);
    delay(10);
  }
}

void showPlaying(uint8_t number) {
  //now we show how many are avialable
  clearBottom(); clearTop();
  setLed(vgmFolder + 16, 1);
  for (int16_t i = 1; i <= fileCount; i++) {
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

inline void playFile(int fileNumber);

void playFile(int16_t fileNumber) {

  checkCard();
  showPlaying(fileNumber);
  mx.setPoint(7, 4, 1);//run LED on
  startSD();


  String fileName = String();

    fileName = "/VGM/";
    fileName += vgmFolder+1;
    fileName += "/";
    fileName += filenames[fileNumber];

  VGMStream stream;
  if (stream.open(fileName)) {
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
      int16_t vgmDebounce = 0;
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

            } else if (value == 0x67) {
              int ctrl = stream.read();
              int typeByte = stream.read();
              uint8_t lenBytes[4];
              if (ctrl < 0 || typeByte < 0 ||
                  stream.readBytes(lenBytes, sizeof(lenBytes)) != sizeof(lenBytes)) {
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
              // Command contains only parameters for a PCM transfer that we ignore for now.
              (void)chipType;

            } else if (value >= 0x70 && value <= 0x7F) {
              waitForSamples(static_cast<uint32_t>((value & 0x0F) + 1));

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
  } else {
    Serial.println("failed to open vgm file");
  }

  //muting all voices

  for (int16_t i = 0; i < 24; i++) {
    opl2.setVolume(i, OPERATOR1, 63 );
    opl2.setVolume(i, OPERATOR2, 63 );
  }
  endSD();
  showVgmFiles();
  mx.setPoint(7, 4, 0);

}

inline void playFile(int fileNumber) {
  playFile(static_cast<int16_t>(fileNumber));
}

void opl3Send(uint8_t address, uint8_t data) {
  // Implementation for sending address and data to OPL chip (port 0)
  // Replace with your own code
  opl2.write(address, data);


}

void opl3SendHigh(uint8_t address, uint8_t data) {
  // Implementation for sending address and data to OPL3
  // Replace with your own code
  opl2.writeHigh(address, data);

}

void pause(uint8_t data1, uint8_t data2) {
  uint32_t samples = static_cast<uint32_t>(data1) | (static_cast<uint32_t>(data2) << 8);
  waitForSamples(samples);



}

#endif
