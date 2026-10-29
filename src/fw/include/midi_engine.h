#if defined(__MK20DX256__)
#include <usb_midi.h>
#endif

#if defined(USBD_USE_MIDI) && (defined(ARDUINO_ARCH_STM32) || defined(STM32F1) || defined(STM32F4))
#include "stm32/usb/usbd_midi_if.h"
#endif

#include "pattern_visualizer.h"
#include "midi_sysex.h"
#include "web_tool_protocol.h"

struct MidiParserState {
  uint8_t status = 0;
  uint8_t channel = 0;
  uint8_t data[2] = {0, 0};
  uint8_t dataLen = 0;
  uint8_t dataIndex = 0;
  MidiInputSource source = MIDI_INPUT_NONE;
};

MidiParserState midiDinParser;

#if defined(USBD_USE_MIDI) && (defined(ARDUINO_ARCH_STM32) || defined(STM32F1) || defined(STM32F4))
MidiParserState midiUsbParser;
#endif

constexpr size_t MIDI_SYSEX_BUFFER_SIZE = 512;
static uint8_t midiSysexBuffer[MIDI_SYSEX_BUFFER_SIZE];
static size_t midiSysexLength = 0;
static bool midiSysexActive = false;
static bool midiSysexOverflow = false;

inline void resetMidiSysexParser() {
  midiSysexActive = false;
  midiSysexLength = 0;
  midiSysexOverflow = false;
}

constexpr uint8_t MIDI_NOTE_OFF_STATUS = 0x80;
constexpr uint8_t MIDI_NOTE_ON_STATUS = 0x90;
constexpr uint8_t MIDI_CC_STATUS = 0xB0;
constexpr uint8_t MIDI_PC_STATUS = 0xC0;
constexpr uint8_t MIDI_AFTERTOUCH_STATUS = 0xD0;
constexpr uint8_t MIDI_PITCH_STATUS = 0xE0;

constexpr uint8_t NOTE_TOOL_CH_MUTES = 70;
constexpr uint8_t NOTE_TOOL_CH_SOLOS = 71;
constexpr uint8_t NOTE_TOOL_CH_PROGRAM = 72;

void handlePitchBend(uint8_t channel, uint8_t lsb, uint8_t msb);
void setMidiPitchBend(uint8_t voice, int16_t bendValue);

inline uint8_t clampToMidiChannel(uint8_t channel) {
  return channel ? channel : 1;
}

inline uint8_t encodeStatus(uint8_t baseStatus, uint8_t channel) {
  uint8_t zeroBasedCh = clampToMidiChannel(channel) - 1;
  return static_cast<uint8_t>((baseStatus & 0xF0) | (zeroBasedCh & 0x0F));
}

#if defined(USBD_USE_MIDI) && (defined(ARDUINO_ARCH_STM32) || defined(STM32F1) || defined(STM32F4))
inline void stm32UsbMidiSendPacket(uint8_t cin, uint8_t byte0, uint8_t byte1, uint8_t byte2) {
  if (!USBMIDI_ready()) { return; }
  uint8_t packet[4] = { static_cast<uint8_t>(cin & 0x0F), byte0, byte1, byte2 };
  constexpr uint32_t USB_MIDI_SEND_TIMEOUT_MS = 20;
  constexpr uint16_t USB_MIDI_RETRY_DELAY_US = 200;
  const uint32_t startMs = millis();
  // Retry briefly so burst transfers (e.g. settings handshake) don't drop packets.
  while (!USBMIDI_sendPackets(packet, sizeof(packet))) {
    if (!USBMIDI_ready()) { return; }
    if ((millis() - startMs) >= USB_MIDI_SEND_TIMEOUT_MS) { return; }
    delayMicroseconds(USB_MIDI_RETRY_DELAY_US);
    yield();
  }
}

inline void stm32UsbMidiSendMessage3(uint8_t status, uint8_t data1, uint8_t data2) {
  stm32UsbMidiSendPacket(static_cast<uint8_t>((status >> 4) & 0x0F), status, data1, data2);
}

inline void stm32UsbMidiSendMessage2(uint8_t status, uint8_t data1) {
  stm32UsbMidiSendPacket(static_cast<uint8_t>((status >> 4) & 0x0F), status, data1, 0x00);
}

inline void stm32UsbMidiSendRealTime(uint8_t realtime) {
  stm32UsbMidiSendPacket(0x0F, realtime, 0x00, 0x00);
}
#endif

inline bool midiOutputAllowedToDin() {
  if (suppressIncomingMidiForwarding) {
    return activeMidiInputSource != MIDI_INPUT_USB;
  }
  return true;
}

inline bool midiOutputAllowedToUsb() {
  if (suppressIncomingMidiForwarding) {
    return activeMidiInputSource != MIDI_INPUT_DIN;
  }
  return true;
}

inline void sendMidiRealtime(uint8_t realtime) {
  if (midiOutputAllowedToDin()) {
    Serial1.write(realtime);
  }

#if defined(__MK20DX256__)
  if (midiOutputAllowedToUsb()) {
    switch (realtime) {
      case 0xF8: usbMIDI.sendRealTime(usbMIDI.Clock); break;
      case 0xFA: usbMIDI.sendRealTime(usbMIDI.Start); break;
      case 0xFB: usbMIDI.sendRealTime(usbMIDI.Continue); break;
      case 0xFC: usbMIDI.sendRealTime(usbMIDI.Stop); break;
    }
  }
#endif

#if defined(USBD_USE_MIDI) && (defined(ARDUINO_ARCH_STM32) || defined(STM32F1) || defined(STM32F4))
  if (midiOutputAllowedToUsb()) {
    stm32UsbMidiSendRealTime(realtime);
  }
#endif
}

inline void sendMidiPacket(uint8_t status, uint8_t data1, uint8_t data2) {
  if (midiOutputAllowedToDin()) {
    Serial1.write(status);
    Serial1.write(data1);
    Serial1.write(data2);
  }

#if defined(__MK20DX256__)
  if (midiOutputAllowedToUsb()) {
    switch (status & 0xF0) {
      case MIDI_NOTE_ON_STATUS:
        usbMIDI.sendNoteOn(data1, data2, (status & 0x0F) + 1);
        break;
      case MIDI_NOTE_OFF_STATUS:
        usbMIDI.sendNoteOff(data1, data2, (status & 0x0F) + 1);
        break;
      case MIDI_CC_STATUS:
        usbMIDI.sendControlChange(data1, data2, (status & 0x0F) + 1);
        break;
      case MIDI_PITCH_STATUS:
        usbMIDI.sendPitchBend(((uint16_t)data2 << 7) | data1, (status & 0x0F) + 1);
        break;
    }
  }
#endif

#if defined(USBD_USE_MIDI) && (defined(ARDUINO_ARCH_STM32) || defined(STM32F1) || defined(STM32F4))
  if (midiOutputAllowedToUsb()) {
    stm32UsbMidiSendMessage3(status, data1, data2);
  }
#endif
}

inline void sendMidiPacket(uint8_t status, uint8_t data1) {
  if (midiOutputAllowedToDin()) {
    Serial1.write(status);
    Serial1.write(data1);
  }

#if defined(__MK20DX256__)
  if (midiOutputAllowedToUsb()) {
    switch (status & 0xF0) {
      case MIDI_PC_STATUS:
        usbMIDI.sendProgramChange(data1, (status & 0x0F) + 1);
        break;
      case MIDI_AFTERTOUCH_STATUS:
        usbMIDI.sendAfterTouch(data1, (status & 0x0F) + 1);
        break;
    }
  }
#endif

#if defined(USBD_USE_MIDI) && (defined(ARDUINO_ARCH_STM32) || defined(STM32F1) || defined(STM32F4))
  if (midiOutputAllowedToUsb()) {
    stm32UsbMidiSendMessage2(status, data1);
  }
#endif
}

inline void resetParser(MidiParserState &state) {
  state.status = 0;
  state.channel = 0;
  state.dataLen = 0;
  state.dataIndex = 0;
  state.source = MIDI_INPUT_NONE;
}

inline void startParserMessage(MidiParserState &state, uint8_t status, uint8_t channel, uint8_t dataLen) {
  state.status = status & 0xF0;
  state.channel = clampToMidiChannel(channel);
  state.dataLen = dataLen;
  state.dataIndex = 0;
}

inline void processMidiByte(MidiParserState &state, uint8_t byte, MidiInputSource source) {
  state.source = source;

  if (midiSysexActive) {
    if (byte == 0xF7) {
      if (!midiSysexOverflow && midiSysexLength < MIDI_SYSEX_BUFFER_SIZE) {
        midiSysexBuffer[midiSysexLength++] = byte;
      } else {
        midiSysexOverflow = true;
      }
      if (!midiSysexOverflow) {
        handlePatternVisualizerSysEx(midiSysexBuffer, midiSysexLength);
      }
      resetMidiSysexParser();
      return;
    }

    if (byte < 0x80) {
      if (!midiSysexOverflow && midiSysexLength < MIDI_SYSEX_BUFFER_SIZE) {
        midiSysexBuffer[midiSysexLength++] = byte;
      } else {
        midiSysexOverflow = true;
      }
      return;
    }

    if (byte >= 0xF8) {
      switch (byte) {
        case 0xF8:
          midiClock();
          break;
        case 0xFA:
        case 0xFB:
          midiStart();
          break;
        case 0xFC:
          midiStop();
          break;
        default:
          break;
      }
      return;
    }

    // Any other status byte cancels the current SysEx stream.
    resetMidiSysexParser();
  }

  if (byte == 0xF0) {
    midiSysexActive = true;
    midiSysexLength = 0;
    midiSysexOverflow = false;
    midiSysexBuffer[midiSysexLength++] = byte;
    return;
  }

  if (byte & 0x80) {
    switch (byte) {
      case 0xF8:
        midiClock();
        return;
      case 0xFA:
      case 0xFB:
        midiStart();
        return;
      case 0xFC:
        midiStop();
        return;
      case 0x80 ... 0x8F:
        startParserMessage(state, MIDI_NOTE_OFF_STATUS, (byte & 0x0F) + 1, 2);
        return;
      case 0x90 ... 0x9F:
        startParserMessage(state, MIDI_NOTE_ON_STATUS, (byte & 0x0F) + 1, 2);
        return;
      case 0xA0 ... 0xAF:
        startParserMessage(state, 0xA0, (byte & 0x0F) + 1, 2);
        return;
      case 0xB0 ... 0xBF:
        startParserMessage(state, MIDI_CC_STATUS, (byte & 0x0F) + 1, 2);
        return;
      case 0xC0 ... 0xCF:
        startParserMessage(state, MIDI_PC_STATUS, (byte & 0x0F) + 1, 1);
        return;
      case 0xD0 ... 0xDF:
        startParserMessage(state, MIDI_AFTERTOUCH_STATUS, (byte & 0x0F) + 1, 1);
        return;
      case 0xE0 ... 0xEF:
        startParserMessage(state, MIDI_PITCH_STATUS, (byte & 0x0F) + 1, 2);
        return;
      default:
        resetParser(state);
        return;
    }
  }

  if (!state.dataLen) { return; }

  if (state.dataIndex < 2) {
    state.data[state.dataIndex++] = byte;
  }

  if (state.dataIndex < state.dataLen) { return; }

  MidiInputSource previousSource = activeMidiInputSource;
  bool previousSuppress = suppressIncomingMidiForwarding;
  activeMidiInputSource = state.source;
  suppressIncomingMidiForwarding = (state.source != MIDI_INPUT_NONE);

  switch (state.status) {
    case MIDI_NOTE_ON_STATUS:
      if (state.data[1]) handleNoteOn(state.channel, state.data[0], state.data[1]);
      else handleNoteOff(state.channel, state.data[0], state.data[1]);
      break;
    case MIDI_NOTE_OFF_STATUS:
      handleNoteOff(state.channel, state.data[0], state.data[1]);
      break;
    case MIDI_CC_STATUS:
      HandleControlChange(state.channel, state.data[0], state.data[1]);
      break;
    case MIDI_PC_STATUS:
      handlePC(state.channel, state.data[0]);
      break;
    case MIDI_AFTERTOUCH_STATUS:
      // Aftertouch currently not used; keep stub so parser stays in sync.
      break;
    case MIDI_PITCH_STATUS:
      handlePitchBend(state.channel, state.data[0], state.data[1]);
      break;
  }

  suppressIncomingMidiForwarding = previousSuppress;
  activeMidiInputSource = previousSource;
  state.dataIndex = 0;
}

void sendControlChange(uint8_t number, uint8_t value, uint8_t channel) {
  if (!channel) { return; }
  uint8_t status = encodeStatus(MIDI_CC_STATUS, channel);
  sendMidiPacket(status, number, value);
}

void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
  if (!channel) { return; }
  uint8_t status = encodeStatus(MIDI_NOTE_OFF_STATUS, channel);
  sendMidiPacket(status, note, velocity);
}

void handlePitchBend(uint8_t channel, uint8_t lsb, uint8_t msb) {
  const uint16_t raw = (static_cast<uint16_t>(msb & 0x7F) << 7) | (lsb & 0x7F);
  const int16_t bend = static_cast<int16_t>(raw) - 8192;
  if (channel == ch1) {
    setMidiPitchBend(0, bend);
  } else if (channel == ch2) {
    setMidiPitchBend(1, bend);
  } else if (channel == ch3) {
    setMidiPitchBend(2, bend);
  } else if (channel == ch4) {
    setMidiPitchBend(3, bend);
  }
}

void handlePC(uint8_t channel, uint8_t value) {
  if (channel == chProgramChange) {
    switch (value) {
      case 0 ... 15: notPc=false;changePattern(value); break;
      case 16 ... 25:
        changeKit(value - 16);
        showPattern();
        break;
    }
  }
}

void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
  if (!channel) { return; }
  uint8_t status = encodeStatus(MIDI_NOTE_ON_STATUS, channel);
  sendMidiPacket(status, note, velocity);
}




void midiRead() {
  while (Serial1.available()) {
    processMidiByte(midiDinParser, Serial1.read(), MIDI_INPUT_DIN);
  }

#if defined(USBD_USE_MIDI) && (defined(ARDUINO_ARCH_STM32) || defined(STM32F1) || defined(STM32F4))
  if (USBMIDI_ready()) {
    uint8_t packet[4];
    while (USBMIDI_readPacket(packet) == 4) {
      uint8_t cin = packet[0] & 0x0F;
      switch (cin) {
        case 0x4:
        case 0x7:
          processMidiByte(midiUsbParser, packet[1], MIDI_INPUT_USB);
          processMidiByte(midiUsbParser, packet[2], MIDI_INPUT_USB);
          processMidiByte(midiUsbParser, packet[3], MIDI_INPUT_USB);
          break;
        case 0x5:
          processMidiByte(midiUsbParser, packet[1], MIDI_INPUT_USB);
          break;
        case 0x6:
          processMidiByte(midiUsbParser, packet[1], MIDI_INPUT_USB);
          processMidiByte(midiUsbParser, packet[2], MIDI_INPUT_USB);
          break;
        case 0x8:
        case 0x9:
        case 0xA:
        case 0xB:
        case 0xE:
          processMidiByte(midiUsbParser, packet[1], MIDI_INPUT_USB);
          processMidiByte(midiUsbParser, packet[2], MIDI_INPUT_USB);
          processMidiByte(midiUsbParser, packet[3], MIDI_INPUT_USB);
          break;
        case 0xC:
        case 0xD:
          processMidiByte(midiUsbParser, packet[1], MIDI_INPUT_USB);
          processMidiByte(midiUsbParser, packet[2], MIDI_INPUT_USB);
          break;
        case 0xF:
          processMidiByte(midiUsbParser, packet[1], MIDI_INPUT_USB);
          break;
        default:
          break;
      }
    }
  }
#endif

#if defined(__MK20DX256__)
  while (usbMIDI.read()) {
    uint8_t type = usbMIDI.getType();
    uint8_t channel = clampToMidiChannel(usbMIDI.getChannel());
    uint8_t data1 = usbMIDI.getData1();
    uint8_t data2 = usbMIDI.getData2();

    MidiInputSource previousSource = activeMidiInputSource;
    bool previousSuppress = suppressIncomingMidiForwarding;
    activeMidiInputSource = MIDI_INPUT_USB;
    suppressIncomingMidiForwarding = true;

    switch (type) {
      case usbMIDI.NoteOn:
        if (data2) handleNoteOn(channel, data1, data2);
        else handleNoteOff(channel, data1, data2);
        break;
      case usbMIDI.NoteOff:
        handleNoteOff(channel, data1, data2);
        break;
      case usbMIDI.ControlChange:
        HandleControlChange(channel, data1, data2);
        break;
      case usbMIDI.ProgramChange:
        handlePC(channel, data1);
        break;
      case usbMIDI.SystemExclusive:
        handlePatternVisualizerSysEx(usbMIDI.getSysExArray(), usbMIDI.getSysExArrayLength());
        break;
      case usbMIDI.Clock:
        midiClock();
        break;
      case usbMIDI.Start:
      case usbMIDI.Continue:
        midiStart();
        break;
      case usbMIDI.Stop:
        midiStop();
        break;
      default:
        break;
    }

    suppressIncomingMidiForwarding = previousSuppress;
    activeMidiInputSource = previousSource;
  }
#endif
}

void HandleControlChange(uint8_t channel, uint8_t number, uint8_t data) {
  uint8_t offset = 0;
  if (channel == ch2) offset = 28;       //virtual third row
  else if (channel == ch3) offset = 56;  //virtual 4th row
  else if (channel == ch4) offset = 84;  //virtual 5th row

  data = data >> 3;

  if (channel == ch10) {
    switch (number) {
      case 20: opl[currentKit][1] = data; break;  //vol
      case 21: opl[currentKit][2] = data; break;  //wav
      case 22: opl[currentKit][3] = data; break;  //feedback
      case 23: opl[currentKit][4] = data; break;  //mult
      case 24: opl[currentKit][5] = data; break;  //tune
      case 25: opl[currentKit][6] = data; break;  //fall
      case 26: opl[currentKit][7] = data; break;  //att
      case 27:
        opl[currentKit][8] = data;
        break;  //decay
        /////////// .  /////////// .  /////////// .  /////////// .  ///////////
      case 28: opl[currentKit][9] = data; break;   //vol
      case 29: opl[currentKit][10] = data; break;  //wav
      case 30: opl[currentKit][11] = data; break;  //mult
      case 31: opl[currentKit][12] = data; break;  //fall
      case 32: opl[currentKit][13] = data; break;  //att
      case 33:
        opl[currentKit][14] = data;
        break;  //rel
        /////////// .  /////////// .  /////////// .  /////////// .  /////////// .
      case 34: opl[currentKit][15] = data; break;  //vol
      case 35: opl[currentKit][16] = data; break;  //wav
      case 36:
        opl[currentKit][19] = data;
        opl[currentKit][151] = DRUM_SHARED_MULT_SNARE;
        break;  //mult
      case 37: opl[currentKit][17] = data; break;  //tune
      case 38: opl[currentKit][18] = data; break;  //fall
      case 39: opl[currentKit][20] = data; break;  //att
      case 40:
        opl[currentKit][21] = data;
        break;  //rel
        /////////// .  /////////// .  /////////// .  /////////// .  /////////// .

      case 41: opl[currentKit][22] = data; break;  //selectedVOICE==5 vol
      case 42: opl[currentKit][23] = data; break;  //voice 2 vol
      case 43: opl[currentKit][24] = data; break;  //vol
      case 44: opl[currentKit][25] = data; break;  //wav selectedVoice==2
      case 45: opl[currentKit][26] = data; break;  //wav
      case 46: opl[currentKit][29] = data; break;  //mult
      case 47: opl[currentKit][27] = data; break;  //tune
      case 48:
        opl[currentKit][28] = data;
        break;  //fall

      //att
      case 49: opl[currentKit][30] = data; break;  //voice 5
      case 50: opl[currentKit][31] = data; break;  //voice 2
      case 51:
        opl[currentKit][32] = data;
        break;  //voice 0

      //rel
      case 52: opl[currentKit][33] = data; break;  //voice5
      case 53: opl[currentKit][34] = data; break;  //voice 2
      case 54: opl[currentKit][35] = data; break;  //rel
    }
  }

  if ((channel == ch1) || (channel == ch2) || (channel == ch3) || (channel == ch4)) {
    switch (number) {

      /////////// .  /////////// .  /////////// .
      /////////// .   SECOND ROW .             .
      /////////// .  /////////// .  /////////// .
      case 20: opl[currentKit][offset + 36] = data; break;  //vol
      case 21: opl[currentKit][offset + 37] = data; break;  //wav
      case 22: opl[currentKit][offset + 38] = data; break;  //feed
      case 23: opl[currentKit][offset + 39] = data; break;  //algo
      case 24: opl[currentKit][offset + 40] = data; break;  ////mult
      case 25: opl[currentKit][offset + 41] = data; break;  //att
      case 26: opl[currentKit][offset + 42] = data; break;  //rel

      case 27: opl[currentKit][offset + 43] = data; break;  //vol
      case 28: opl[currentKit][offset + 44] = data; break;  //wav
      case 29: opl[currentKit][offset + 45] = data; break;  //tune
      case 30: opl[currentKit][offset + 46] = data; break;  //trem
      case 31: opl[currentKit][offset + 47] = data; break;  ////vibra
      case 32: opl[currentKit][offset + 48] = data; break;  //att
      case 33: opl[currentKit][offset + 49] = data; break;  //rel

      case 34: opl[currentKit][offset + 50] = data; break;  //vol
      case 35: opl[currentKit][offset + 51] = data; break;  //wav
      case 36: opl[currentKit][offset + 52] = data; break;  //fdb
      case 37: opl[currentKit][offset + 53] = data; break;  //tune
      case 38: opl[currentKit][offset + 54] = data; break;  ////mult
      case 39: opl[currentKit][offset + 55] = data; break;  //att
      case 40: opl[currentKit][offset + 56] = data; break;  //rel

      case 41: opl[currentKit][offset + 57] = data; break;  //vol
      case 42: opl[currentKit][offset + 58] = data; break;  //wav
      case 43: opl[currentKit][offset + 59] = data; break;  //mult
      case 44: opl[currentKit][offset + 60] = data; break;  //trem
      case 45: opl[currentKit][offset + 61] = data; break;  ////vibra
      case 46: opl[currentKit][offset + 62] = data; break;  //att
      case 47: opl[currentKit][offset + 63] = data; break;  //rel
    }
  }
}




void midiClock() {
  if (!analogSlave) {
    if (midiSlave) {



      midiClockTicks++;
      if (midiClockTicks == 6) { midiClockAdvance(); }
    }
  }
}

void midiStart() {
  if (!analogSlave) {
    sendSync();
    stepTickCounter = 0;
    midiClockTicks = -1;
    tremTickCounter = 0;
    vibTickCounter = 0;
    startSeq();
    midiSlave = true;
    tempoAlt = 1;
    fire();
  }
}

void midiStop() {
  if (!analogSlave) {
    midiSlave = false;
    seqReset();
    stopSeq();
  }
}



bool handleSoloMute(uint8_t channel, uint8_t note) {
  constexpr uint8_t C7 = 96-12;   // MIDI C6
  constexpr uint8_t C8 = 108-12;  // MIDI C7

  bool handled = false;

  if (channel == chSoloMidi) {
    if (note >= C7 && note < C7 + 10) {
      uint8_t idx = note - C7;  // 0..9
      int target = idx + 1;     // 1..10

      if (solo == target) solo = 0;
      else solo = target;

      handled = true;
    }
  }

  if (channel == chMuteMidi) {
    if (note >= C8 && note < C8 + 10) {
      uint8_t idx = note - C8;  // 0..9
      mute[idx] = !mute[idx];   // toggle mute[i]
      handled = true;
    }
  }

  return handled;
}

void sendMidiDrums() {

  //expire held midi out notes
  for (int16_t i = 0; i < 11; i++) {
    if (midiTimeout[i] == 1) {
      midiTimeout[i] = 0;
      switch (i) {
        case 0: sendNoteOff(drumNotes[0], 127, ch10); break;
        case 1: sendNoteOff(drumNotes[1], 127, ch10); break;
        case 2: sendNoteOff(drumNotes[2], 127, ch10); break;
        case 3: sendNoteOff(drumNotes[3], 127, ch10); break;
        case 4: sendNoteOff(drumNotes[4], 127, ch10); break;
        case 5: sendNoteOff(drumNotes[5], 127, ch10); break;


        case 6: sendNoteOff(midiLastNoteSynth[0], 127, ch1); break;   //synth 1
        case 7: sendNoteOff(midiLastNoteSynth[1], 127, ch2); break;   //synth 2
        case 8: sendNoteOff(midiLastNoteSynth[2], 127, ch3); break;   //synth 3 voice1
        case 9: sendNoteOff(midiLastNoteSynth[3], 127, ch4); break;   //synth 4
        case 10: sendNoteOff(midiLastNoteSynth[4], 127, ch3); break;  //synth 3 voice2
      }
    }
  }

  //trigger synth MIDI
  if (midiHotSynth[0]) {
    midiTimeout[6] = stepTicks;
    sendNoteOn(key + midiHotSynth[0], 127, ch1);
    midiLastNoteSynth[0] = midiHotSynth[0];
    midiHotSynth[0] = 0;

  }  //synth 1

  if (midiHotSynth[1]) {
    midiTimeout[7] = stepTicks;
    sendNoteOn(key + midiHotSynth[1], 127, ch2);
    midiLastNoteSynth[1] = midiHotSynth[1];
    midiHotSynth[1] = 0;
  }  //synth 2

  if (midiHotSynth[2]) {
    midiTimeout[8] = stepTicks;
    sendNoteOn(key + midiHotSynth[2], 127, ch3);
    midiLastNoteSynth[2] = midiHotSynth[2];
    midiHotSynth[2] = 0;
  }  //synth 3 voice1

  if (midiHotSynth[3]) {
    midiTimeout[9] = stepTicks;
    sendNoteOn(key + midiHotSynth[3], 127, ch4);
    midiLastNoteSynth[3] = midiHotSynth[3];
    midiHotSynth[3] = 0;
  }  //synth 4

  if (midiHotSynth[4]) {
    midiTimeout[10] = stepTicks;
    sendNoteOn(key + midiHotSynth[4], 127, ch3);
    midiLastNoteSynth[4] = midiHotSynth[4];
    midiHotSynth[4] = 0;
  }  //synth 3 voice2




  //KICK
  if (midiHot[0]) {
    midiHot[0] = false;
    if (visu) { flipLed(16); }
    midiTimeout[0] = stepTicks;
    sendNoteOn(drumNotes[0], 127, ch10);
  }

  //SNARE
  if (midiHot[1]) {
    midiHot[1] = false;
    if (visu) { flipLed(17); }
    midiTimeout[1] = stepTicks;
    sendNoteOn(drumNotes[1], 127, ch10);
  }

  //TOM
  if (midiHot[2]) {
    midiHot[2] = false;
    if (visu) { flipLed(18); }
    midiTimeout[2] = stepTicks;
    sendNoteOn(drumNotes[2], 127, ch10);
  }


  //HH CL
  if (midiHot[3]) {
    midiHot[3] = false;
    if (visu) { flipLed(19); }
    midiTimeout[3] = stepTicks;
    sendNoteOn(drumNotes[3], 127, ch10);
  }

  //HH OPEN
  if (midiHot[4]) {
    midiHot[4] = false;
    if (visu) { flipLed(20); }
    midiTimeout[4] = stepTicks;
    sendNoteOn(drumNotes[4], 127, ch10);
  }

  //CRASH
  if (midiHot[5]) {
    midiHot[5] = false;
    if (visu) { flipLed(21); }
    midiTimeout[5] = stepTicks;
    sendNoteOn(drumNotes[5], 127, ch10);
  }
}



void handleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {

  const bool soloMuteUpdated = handleSoloMute(channel, note);
  if (soloMuteUpdated) {
    showMode();
  }

  if (!ignoreNotes) {
    if (channel == ch10) {

      //( bass,  snare,  tom,  cymbal,  hihat)

      if (note == drumNotes[0]) {
        opl2.setDrums(1, 0, 0, 0, 0);
        if (rec) bitWrite(pattern[currentPattern][0][targetStep], 0, 1);
        pitchNow[0] = pitchRoot[0];
        pitchNow[1] = pitchRoot[1];
      }  //kick

      else if (note == drumNotes[1]) {
        opl2.setDrums(0, 1, 0, 0, 0);
        if (rec) bitWrite(pattern[currentPattern][0][targetStep], 1, 1);
        pitchNow[2] = pitchRoot[2];
      }  //snare

      else if (note == drumNotes[2]) {
        opl2.setDrums(0, 0, 1, 0, 0);
        if (rec) bitWrite(pattern[currentPattern][0][targetStep], 2, 1);
        pitchNow[3] = pitchRoot[3];
      }  //tom

      else if (note == drumNotes[3]) {
        opl2.setRelease(7, OPERATOR1, hatDecay);  //close hat
        opl2.setDrums(0, 0, 0, 0, 1);
        if (rec) bitWrite(pattern[currentPattern][0][targetStep], 3, 1);
        pitchNow[4] = pitchRoot[4];
      }  //closed hat

      else if (note == drumNotes[4]) {
        opl2.setRelease(7, OPERATOR1, hatDecayOpen);  //open hat
        opl2.setDrums(0, 0, 0, 0, 1);
        if (rec) bitWrite(pattern[currentPattern][0][targetStep], 4, 1);
        pitchNow[4] = pitchRoot[4];
      }  //open hat

      else if (note == drumNotes[5]) {
        opl2.setDrums(0, 0, 0, 1, 0);
        if (rec) bitWrite(pattern[currentPattern][0][targetStep], 5, 1);
        pitchNow[4] = pitchRoot[4];
      }  //cymbal

    }

    //////////

    else if (channel == ch1) {
      triggerSynthWithMidi(0, note, 1);
    }

    else if (channel == ch2) {
      triggerSynthWithMidi(1, note, 1);
    }

    else if (channel == ch3) {
      triggerSynthWithMidi(2, note, 1);
    }

    else if (channel == ch4) {
      triggerSynthWithMidi(3, note, 1);
    }
  }
}

uint8_t lastNote, lastVelocity;

void applyToolModeSetting(uint8_t note, uint8_t velocity) {
  const bool boolT = (velocity > 0);

  switch (note) {
    case 30:
      EEPROMupdate(30, velocity);
      drumNotes[0] = velocity;
      break;
    case 31:
      EEPROMupdate(31, velocity);
      drumNotes[1] = velocity;
      break;
    case 32:
      EEPROMupdate(32, velocity);
      drumNotes[2] = velocity;
      break;
    case 33:
      EEPROMupdate(33, velocity);
      drumNotes[3] = velocity;
      break;
    case 34:
      EEPROMupdate(34, velocity);
      drumNotes[4] = velocity;
      break;
    case 35:
      EEPROMupdate(35, velocity);
      drumNotes[5] = velocity;
      break;
    case 9:
      EEPROMupdate(9, velocity + 1);
      ch10 = velocity + 1;
      break;
    case 10:
      EEPROMupdate(10, velocity + 1);
      ch1 = velocity + 1;
      break;
    case 11:
      EEPROMupdate(11, velocity + 1);
      ch2 = velocity + 1;
      break;
    case 12:
      EEPROMupdate(12, velocity + 1);
      ch3 = velocity + 1;
      break;
    case 13:
      EEPROMupdate(13, velocity + 1);
      ch4 = velocity + 1;
      break;
    case NOTE_TOOL_CH_MUTES:
      EEPROMupdate(EEPROM_ADDR_CH_MUTES, velocity + 1);
      chMuteMidi = velocity + 1;
      break;
    case NOTE_TOOL_CH_SOLOS:
      EEPROMupdate(EEPROM_ADDR_CH_SOLOS, velocity + 1);
      chSoloMidi = velocity + 1;
      break;
    case NOTE_TOOL_CH_PROGRAM:
      EEPROMupdate(EEPROM_ADDR_CH_PROGRAM, velocity + 1);
      chProgramChange = velocity + 1;
      break;
    case 3:
      EEPROMupdate(note, velocity << 4);
      routing[0] = velocity << 4;
      updateRouting();
      break;
    case 4:
      EEPROMupdate(note, velocity << 4);
      routing[1] = velocity << 4;
      updateRouting();
      break;
    case 5:
      EEPROMupdate(note, velocity << 4);
      routing[2] = velocity << 4;
      updateRouting();
      break;
    case 6:
      EEPROMupdate(note, velocity << 4);
      routing[3] = velocity << 4;
      updateRouting();
      break;
    case 7:
      EEPROMupdate(note, velocity << 4);
      routing[4] = velocity << 4;
      updateRouting();
      break;
    case 18:
      EEPROMupdate(note, velocity << 4);
      routing[5] = velocity << 4;
      updateRouting();
      break;
    case 19:
      EEPROMupdate(note, velocity << 4);
      routing[6] = velocity << 4;
      updateRouting();
      break;
    case 8:
      safe = boolT;
      EEPROMupdate(note, boolT);
      break;
    case 15:
      EEPROMupdate(note, boolT);
      if (boolT) {
        pitchFallRate = fallRateFast;
      } else {
        pitchFallRate = fallRateSlow;
      }
      break;
    case 29:
      quantise = boolT;
      EEPROMupdate(note, boolT ? 0 : 1);
      break;
    case 23:
      metro = boolT;
      EEPROMupdate(note, boolT);
      break;
    case 24:
      saveKitAlso = (velocity > 0) ? 1 : 0;
      EEPROMupdate(14, saveKitAlso);
      break;
    case 25:
      wiggleOnEnabledStep = (velocity > 0) ? 1 : 0;
      EEPROMupdate(36, wiggleOnEnabledStep);
      break;
    case 26:
      ignoreNotes = (velocity > 0) ? 1 : 0;
      EEPROMupdate(37, ignoreNotes);
      break;
    default:
      break;
  }
}

void settingsSpit() {
  constexpr size_t kMaxSettingPairs = 40;
  uint8_t payload[kMaxSettingPairs * 2]{};
  size_t pairCount = 0;

  auto appendSetting = [&](uint8_t note, uint8_t value) {
    if (pairCount >= kMaxSettingPairs) {
      return;
    }
    payload[pairCount * 2] = note & 0x7F;
    payload[pairCount * 2 + 1] = value & 0x7F;
    ++pairCount;
  };

  if (versionTop < 9) {
    appendSetting(0, static_cast<uint8_t>(versionTop + 1));
  } else {
    appendSetting(0, 0);
  }
  if (versionBottom < 9) {
    appendSetting(1, static_cast<uint8_t>(versionBottom + 1));
  } else {
    appendSetting(1, 0);
  }

  appendSetting(30, drumNotes[0]);
  appendSetting(31, drumNotes[1]);
  appendSetting(32, drumNotes[2]);
  appendSetting(33, drumNotes[3]);
  appendSetting(34, drumNotes[4]);
  appendSetting(35, drumNotes[5]);

  appendSetting(9, static_cast<uint8_t>(ch10 ? ch10 - 1 : 0));
  appendSetting(10, static_cast<uint8_t>(ch1 ? ch1 - 1 : 0));
  appendSetting(11, static_cast<uint8_t>(ch2 ? ch2 - 1 : 0));
  appendSetting(12, static_cast<uint8_t>(ch3 ? ch3 - 1 : 0));
  appendSetting(13, static_cast<uint8_t>(ch4 ? ch4 - 1 : 0));
  appendSetting(NOTE_TOOL_CH_MUTES, static_cast<uint8_t>((chMuteMidi > 0) ? chMuteMidi - 1 : 0));
  appendSetting(NOTE_TOOL_CH_SOLOS, static_cast<uint8_t>((chSoloMidi > 0) ? chSoloMidi - 1 : 0));
  appendSetting(NOTE_TOOL_CH_PROGRAM, static_cast<uint8_t>((chProgramChange > 0) ? chProgramChange - 1 : 0));

  appendSetting(3, static_cast<uint8_t>(routing[0] >> 4));
  appendSetting(4, static_cast<uint8_t>(routing[1] >> 4));
  appendSetting(5, static_cast<uint8_t>(routing[2] >> 4));
  appendSetting(6, static_cast<uint8_t>(routing[3] >> 4));
  appendSetting(7, static_cast<uint8_t>(routing[4] >> 4));
  appendSetting(18, static_cast<uint8_t>(routing[5] >> 4));
  appendSetting(19, static_cast<uint8_t>(routing[6] >> 4));

  appendSetting(8, static_cast<uint8_t>(safe ? 1 : 0));
  appendSetting(15, EEPROMread(15));
  appendSetting(29, static_cast<uint8_t>(quantise ? 1 : 0));
  appendSetting(23, static_cast<uint8_t>(metro ? 1 : 0));
  appendSetting(24, static_cast<uint8_t>(saveKitAlso ? 1 : 0));
  appendSetting(25, static_cast<uint8_t>(wiggleOnEnabledStep ? 1 : 0));
  appendSetting(26, static_cast<uint8_t>(ignoreNotes ? 1 : 0));

  uint8_t message[6 + (kMaxSettingPairs * 2)];
  size_t index = 0;
  message[index++] = 0xF0;
  message[index++] = WebToolProtocol::kManufacturerId;
  message[index++] = WebToolProtocol::kProductId0;
  message[index++] = WebToolProtocol::kProductId1;
  message[index++] = WebToolProtocol::kMsgSettingsDump;
  message[index++] = static_cast<uint8_t>(pairCount & 0x7F);
  for (size_t i = 0; i < pairCount * 2; ++i) {
    message[index++] = payload[i];
  }
  message[index++] = 0xF7;

  sendBlastBeatsSysex(message, index);
}

void handleNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  const bool handshakeRequest =
      (velocity == 82) && (lastVelocity == 82) && (note == 19) && (lastNote == 19);

  if (toolMode) {
    if (handshakeRequest) {
      settingsSpit();
      wipe();
    } else {
      applyToolModeSetting(note, velocity);
    }
  } else {
    if (handshakeRequest) {
      toolMode = true;
      settingsSpit();
      wipe();
    } else {
      if (channel == ch1) {
        triggerSynthWithMidi(0, note, 0);
      } else if (channel == ch2) {
        triggerSynthWithMidi(1, note, 0);
      } else if (channel == ch3) {
        triggerSynthWithMidi(2, note, 0);
      } else if (channel == ch4) {
        triggerSynthWithMidi(3, note, 0);
      }
    }
  }

  lastNote = note;
  lastVelocity = velocity;
}
