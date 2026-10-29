#pragma once

#include <Arduino.h>
#include <SdFat.h>

#if defined(HAS_INCLUDE_FS_H)
#if SDFAT_FILE_TYPE == 1
typedef File32 File;
#elif SDFAT_FILE_TYPE == 2
typedef ExFile File;
#elif SDFAT_FILE_TYPE == 3
typedef FsFile File;
#endif
#endif

class BlastBeatsSDClass {
 public:
  bool begin(uint8_t csPin) {
    return sd_.begin(csPin);
  }

  bool begin(uint32_t maxSck, uint8_t csPin) {
    return sd_.begin(csPin, maxSck);
  }

  void end() {
    sd_.end();
  }

  File open(const char* path, oflag_t mode = FILE_READ) {
    return sd_.open(path, mode);
  }

  File open(const String& path, oflag_t mode = FILE_READ) {
    return open(path.c_str(), mode);
  }

  bool exists(const char* path) {
    return sd_.exists(path);
  }

  bool exists(const String& path) {
    return exists(path.c_str());
  }

  bool remove(const char* path) {
    return sd_.remove(path);
  }

  bool remove(const String& path) {
    return remove(path.c_str());
  }

  bool rename(const char* oldPath, const char* newPath) {
    return sd_.rename(oldPath, newPath);
  }

  bool mkdir(const char* path) {
    return sd_.mkdir(path);
  }

  bool rmdir(const char* path) {
    return sd_.rmdir(path);
  }

 private:
  SdFat sd_;
};

extern BlastBeatsSDClass SD;
