#pragma once

#if defined(ARDUINO_ARCH_STM32)

#include <Arduino.h>
#include <HardwareTimer.h>

class STM32Timer {
 public:
  explicit STM32Timer(TIM_TypeDef* instance)
      : _timer(new HardwareTimer(instance)), _callback(nullptr), _intervalUs(0) {}

  bool attachInterruptInterval(uint32_t microseconds, void (*callback)()) {
    if (!callback) {
      return false;
    }
    _callback = callback;
    _intervalUs = microseconds;
    configureTimer();
    return true;
  }

  void stopTimer() {
    if (_timer) {
      _timer->pause();
      _timer->detachInterrupt();
    }
  }

  void restartTimer() {
    if (_timer && _callback) {
      configureTimer();
    }
  }

 private:
  void configureTimer() {
    _timer->pause();
    _timer->setOverflow(_intervalUs, MICROSEC_FORMAT);
    _timer->attachInterrupt(_callback);
    _timer->resume();
  }

  HardwareTimer* _timer;
  void (*_callback)();
  uint32_t _intervalUs;
};

#endif  // defined(ARDUINO_ARCH_STM32)

