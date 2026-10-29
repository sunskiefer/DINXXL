#include "BlastLedMatrix.h"

BlastLedMatrix::BlastLedMatrix(uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint8_t numDevices)
    : dataPin_(dataPin), clkPin_(clkPin), csPin_(csPin), frame_{} {
  (void)numDevices;
}

void BlastLedMatrix::begin() {
  pinMode(dataPin_, OUTPUT);
  pinMode(clkPin_, OUTPUT);
  pinMode(csPin_, OUTPUT);
  digitalWrite(csPin_, HIGH);
  digitalWrite(clkPin_, LOW);

  writeRegister(kRegDisplayTest, 0x00);
  writeRegister(kRegDecodeMode, 0x00);
  writeRegister(kRegScanLimit, 0x07);
  writeRegister(kRegIntensity, 0x08);
  writeRegister(kRegShutdown, 0x01);
  clear();
}

void BlastLedMatrix::clear() {
  for (uint8_t row = 0; row < kRows; ++row) {
    frame_[row] = 0;
  }
  flushAll();
}

bool BlastLedMatrix::control(ControlRequest mode, int value) {
  switch (mode) {
    case SHUTDOWN:
      writeRegister(kRegShutdown, value ? 0x01 : 0x00);
      return true;
    case SCANLIMIT:
      writeRegister(kRegScanLimit, constrain(value, 0, 7));
      return true;
    case INTENSITY:
      writeRegister(kRegIntensity, constrain(value, 0, 15));
      return true;
    case TEST:
      writeRegister(kRegDisplayTest, value ? 0x01 : 0x00);
      return true;
    case DECODE:
      writeRegister(kRegDecodeMode, value ? 0xFF : 0x00);
      return true;
  }
  return false;
}

bool BlastLedMatrix::setPoint(uint8_t row, uint16_t col, bool state) {
  if (row >= kRows || col >= kCols) {
    return false;
  }

  const uint8_t hardwareRow = mapRow(row);
  const uint8_t bitMask = static_cast<uint8_t>(1U << mapCol(static_cast<uint8_t>(col)));
  const bool currentState = (frame_[hardwareRow] & bitMask) != 0;
  if (currentState == state) {
    return true;
  }

  if (state) {
    frame_[hardwareRow] |= bitMask;
  } else {
    frame_[hardwareRow] &= static_cast<uint8_t>(~bitMask);
  }
  flushRow(hardwareRow);
  return true;
}

bool BlastLedMatrix::getPoint(uint8_t row, uint16_t col) const {
  if (row >= kRows || col >= kCols) {
    return false;
  }
  return (frame_[mapRow(row)] & (1U << mapCol(static_cast<uint8_t>(col)))) != 0;
}

uint8_t BlastLedMatrix::mapRow(uint8_t row) const {
  return row;
}

uint8_t BlastLedMatrix::mapCol(uint8_t col) const {
  return 7 - col;
}

void BlastLedMatrix::writeRegister(uint8_t reg, uint8_t value) {
  digitalWrite(csPin_, LOW);
  shiftOut(dataPin_, clkPin_, MSBFIRST, reg);
  shiftOut(dataPin_, clkPin_, MSBFIRST, value);
  digitalWrite(csPin_, HIGH);
}

void BlastLedMatrix::flushRow(uint8_t hardwareRow) {
  writeRegister(kRegDigit0 + hardwareRow, frame_[hardwareRow]);
}

void BlastLedMatrix::flushAll() {
  for (uint8_t row = 0; row < kRows; ++row) {
    flushRow(row);
  }
}
