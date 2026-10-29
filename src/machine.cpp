#include "machine.h"
#include "hw.h"
#include "Arduino.h"

void bb_setup();
void bb_loop();
void prepareFaders();

namespace machine {
static const uint32_t LOOP_US = 100;   // the firmware's main loop runs about every 100 us on the R2
static uint32_t g_carry = 0;

void power_on(uint32_t seed) {
  hw::reset(seed);
  g_carry = 0;
  bb_setup();
}
void run(uint32_t us) {
  us += g_carry;
  while (us >= LOOP_US) {
    hw::advance(LOOP_US);
    bb_loop();
    us -= LOOP_US;
  }
  g_carry = us;
}
void sync_faders() { prepareFaders(); }
}
