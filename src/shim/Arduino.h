// Host stand-in for the Arduino/STM32 core the BlastBeats firmware was written for.
// The firmware sources are compiled unchanged; every pin, timer, serial port and
// the SD card is answered here from the plugin's virtual machine (see hw.h).
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <map>
#include <memory>
#include <vector>
#include <deque>
#include <functional>
#include <utility>
#include <sstream>
#include <string>
#include <ctype.h>

typedef uint8_t byte;
typedef bool boolean;
typedef uint16_t word;
#define PROGMEM
#define F(x) (x)
#define pgm_read_byte(a) (*(const uint8_t*)(a))
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define MSBFIRST 1
#define LSBFIRST 0
#define DEC 10
#define HEX 16
#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
#define bitRead(v, b) (((v) >> (b)) & 1)
#define bitSet(v, b) ((v) |= (1UL << (b)))
#define bitClear(v, b) ((v) &= ~(1UL << (b)))
#define bitWrite(v, b, x) ((x) ? bitSet(v, b) : bitClear(v, b))
#define lowByte(w) ((uint8_t)((w) & 0xff))
#define highByte(w) ((uint8_t)((w) >> 8))
#define _BV(b) (1UL << (b))
#define constrain(a, lo, hi) ((a) < (lo) ? (lo) : ((a) > (hi) ? (hi) : (a)))
#define sq(x) ((x) * (x))
#define radians(d) ((d) * 0.017453292519943295)
#define degrees(r) ((r) * 57.29577951308232)
#define NOP() ((void)0)
#define __NOP() ((void)0)
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

// STM32 pin names as plain numbers; only the identity of a pin matters here.
enum : uint8_t {
  PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
  PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
  PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, PC14, PC15,
  NUM_PINS_
};
#define A0 PA0
#define A1 PA1

// ---- virtual machine hooks (implemented in hw.cpp) ----
extern "C++" {
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
int analogRead(uint8_t pin);
void analogReadResolution(int bits);
void analogReadRes(int bits);
uint32_t millis();
uint32_t micros();
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);
void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t order, uint8_t val);
long random(long maxv);
long random(long minv, long maxv);
void randomSeed(unsigned long s);
void noInterrupts();
void interrupts();
}
inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
  if (in_max == in_min) return out_min;
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
inline void yield() {}

class String : public std::string {
 public:
  String() {}
  String(const char* s) : std::string(s ? s : "") {}
  String(const std::string& s) : std::string(s) {}
  String(int v) : std::string(std::to_string(v)) {}
  String& operator=(const char* s) { assign(s ? s : ""); return *this; }
  String& operator+=(unsigned v) { append(std::to_string(v)); return *this; }
  String& operator+=(const String& s) { append(s); return *this; }
  String(unsigned v) : std::string(std::to_string(v)) {}
  const char* c_str() const { return std::string::c_str(); }
  unsigned length() const { return (unsigned)size(); }
  String& operator+=(const char* s) { append(s); return *this; }
  String& operator+=(char c) { push_back(c); return *this; }
  String& operator+=(int v) { append(std::to_string(v)); return *this; }
  String operator+(const char* s) const { String r(*this); r.append(s); return r; }
  String operator+(const String& s) const { String r(*this); r.append(s); return r; }
  String operator+(int v) const { String r(*this); r.append(std::to_string(v)); return r; }
  bool startsWith(const char* s) const { return rfind(s, 0) == 0; }
  bool endsWith(const char* s) const { size_t n = strlen(s); return size() >= n && compare(size() - n, n, s) == 0; }
  void toLowerCase() { for (auto& c : *this) c = (char)tolower((unsigned char)c); }
};
struct SPIShim { void begin() {} void end() {} void setMOSI(uint8_t) {} void setMISO(uint8_t) {} void setSCLK(uint8_t) {} };
extern SPIShim SPI;
// I2C: the firmware talks to three TPA6130A2 amplifiers (out 1-4, phones, mix) chosen by the mux lines.
struct TwoWireShim {
  void setSDA(uint8_t) {}
  void setSCL(uint8_t) {}
  void begin() {}
  void setClock(uint32_t) {}
  void beginTransmission(uint8_t a);
  size_t write(uint8_t b);
  uint8_t endTransmission();
};
extern TwoWireShim Wire;

// Byte FIFO standing in for a UART. rx is fed by the plugin (host MIDI), tx is drained by it.
class HardwareSerial {
 public:
  HardwareSerial() {}
  HardwareSerial(uint8_t, uint8_t) {}
  void begin(unsigned long) {}
  void end() {}
  operator bool() const { return true; }
  int available();
  int read();
  int peek();
  size_t write(uint8_t b);
  size_t write(const uint8_t* b, size_t n) { for (size_t i = 0; i < n; i++) write(b[i]); return n; }
  size_t print(const char*) { return 0; }
  size_t print(long) { return 0; }
  size_t print(long, int) { return 0; }
  size_t println() { return 0; }
  size_t println(const char*) { return 0; }
  size_t println(long) { return 0; }
  size_t println(long, int) { return 0; }
  void flush() {}
  int availableForWrite() { return 64; }
};
extern HardwareSerial Serial;

// Timers: fired from the virtual clock in hw.cpp.
#define MICROSEC_FORMAT 1
struct TIM_TypeDef { int id; };
extern TIM_TypeDef TIM4_, TIM5_;
#define TIM4 (&TIM4_)
#define TIM5 (&TIM5_)
class HardwareTimer {
 public:
  explicit HardwareTimer(TIM_TypeDef* t) : id_(t ? t->id : 0), period_(0), cb_(nullptr), running_(false), next_(0) {}
  void pause();
  void resume();
  void refresh();
  void setOverflow(uint32_t v, int fmt);
  void attachInterrupt(void (*cb)());
  void detachInterrupt() { cb_ = nullptr; }
  uint32_t id_, period_;
  void (*cb_)();
  bool running_;
  uint64_t next_;
  void reg_();
};
#include "binary.h"
