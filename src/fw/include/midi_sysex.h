#pragma once

#include <stddef.h>
#include <stdint.h>

// Sends the provided SysEx payload to all connected USB MIDI endpoints.
// Returns true if at least one transport accepted the message.
bool sendBlastBeatsSysex(const uint8_t* data, size_t length);

