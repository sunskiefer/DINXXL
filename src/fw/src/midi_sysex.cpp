#include <Arduino.h>

#include "midi_sysex.h"

#if defined(__MK20DX256__)
#include <usb_midi.h>
#endif

#if defined(USBD_USE_MIDI) && (defined(ARDUINO_ARCH_STM32) || defined(STM32F1) || defined(STM32F4))
#include "stm32/usb/usbd_midi_if.h"
#endif

bool sendBlastBeatsSysex(const uint8_t* data, size_t length) {
  if (!data || length == 0) {
    return false;
  }

  bool sent = false;

#if defined(__MK20DX256__)
  usbMIDI.sendSysEx(length, data, true);
  usbMIDI.send_now();
  sent = true;
#endif

#if defined(USBD_USE_MIDI) && (defined(ARDUINO_ARCH_STM32) || defined(STM32F1) || defined(STM32F4))
  if (USBMIDI_ready()) {
    size_t index = 0;
    while (index < length) {
      const size_t remaining = length - index;
      uint8_t packet[4] = {0, 0, 0, 0};

      if (remaining >= 3) {
        packet[1] = data[index++];
        packet[2] = data[index++];
        packet[3] = data[index++];
        packet[0] = (index < length) ? 0x04 : 0x07;  // Continue or end with 3 bytes.
      } else if (remaining == 2) {
        packet[1] = data[index++];
        packet[2] = data[index++];
        packet[0] = 0x06;  // SysEx ends with 2 bytes.
      } else {
        packet[1] = data[index++];
        packet[0] = 0x05;  // SysEx ends with 1 byte.
      }

      const uint32_t startMs = millis();
      while (!USBMIDI_sendPackets(packet, sizeof(packet))) {
        if (!USBMIDI_ready()) {
          return sent;
        }
        if ((millis() - startMs) > 20) {
          return sent;
        }
        delayMicroseconds(200);
        yield();
      }
    }
    sent = true;
  }
#endif

  return sent;
}

