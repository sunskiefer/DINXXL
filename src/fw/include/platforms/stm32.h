#pragma once

#if defined(ARDUINO_ARCH_STM32)

#include <Arduino.h>
#include <HardwareTimer.h>

#ifndef ATOMIC
#define ATOMIC(X)      \
  do {                 \
    noInterrupts();    \
    X;                 \
    interrupts();      \
  } while (0)
#endif

// Forward declaration for the ISR supplied by uClock.
extern void uclockISR();

// Dedicated hardware timer instance for uClock timing duties.
static HardwareTimer uclockTimer(TIM5);

inline void initTimer(uint32_t us_interval) {
  uclockTimer.pause();
  uclockTimer.setOverflow(us_interval, MICROSEC_FORMAT);
  uclockTimer.attachInterrupt(uclockISR);
  uclockTimer.resume();
}

inline void setTimer(uint32_t us_interval) {
  uclockTimer.pause();
  uclockTimer.setOverflow(us_interval, MICROSEC_FORMAT);
  uclockTimer.refresh();
  uclockTimer.resume();
}

#endif  // defined(ARDUINO_ARCH_STM32)
