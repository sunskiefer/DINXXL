#pragma once
#include <Arduino.h>
class STM32Timer {
 public:
  explicit STM32Timer(TIM_TypeDef* i) : t_(i) {}
  bool attachInterruptInterval(uint32_t us, void (*cb)()) { t_.setOverflow(us, MICROSEC_FORMAT); t_.attachInterrupt(cb); t_.resume(); return true; }
  void stopTimer() { t_.pause(); }
  void restartTimer() { t_.resume(); }
 private:
  HardwareTimer t_;
};
