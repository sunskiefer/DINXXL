#pragma once

#include <Arduino.h>

class BlastLedMatrix {
 public:
  enum ControlRequest {
    SHUTDOWN = 0,
    SCANLIMIT = 1,
    INTENSITY = 2,
    TEST = 3,
    DECODE = 4,
  };

  enum ControlValue {
    OFF = 0,
    ON = 1,
  };

  BlastLedMatrix(uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint8_t numDevices = 1);

  void begin();
  void clear();
  bool control(ControlRequest mode, int value);
  bool setPoint(uint8_t row, uint16_t col, bool state);
  bool getPoint(uint8_t row, uint16_t col) const;

 private:
  static constexpr uint8_t kRows = 8;
  static constexpr uint8_t kCols = 8;

  enum Register {
    kRegDigit0 = 0x01,
    kRegDecodeMode = 0x09,
    kRegIntensity = 0x0A,
    kRegScanLimit = 0x0B,
    kRegShutdown = 0x0C,
    kRegDisplayTest = 0x0F,
  };

  uint8_t mapRow(uint8_t row) const;
  uint8_t mapCol(uint8_t col) const;
  void writeRegister(uint8_t reg, uint8_t value);
  void flushRow(uint8_t hardwareRow);
  void flushAll();

  uint8_t dataPin_;
  uint8_t clkPin_;
  uint8_t csPin_;
  uint8_t frame_[kRows];
};
