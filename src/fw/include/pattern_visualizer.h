#pragma once

#include <stdint.h>
#include <stddef.h>

// Marks the visualizer payload as needing a refresh on the next pump.
void markPatternVisualizerDirty();

// Immediately pushes the current pattern snapshot to connected listeners.
void sendPatternVisualizerSnapshot();

// Call once per loop iteration to service pending visualizer updates.
void patternVisualizerTick();

// Attempts to handle an incoming SysEx message. Returns true if consumed.
bool handlePatternVisualizerSysEx(const uint8_t* data, size_t length);

// Sends the per-voice step positions (voice count inferred by receiver).
void patternVisualizerSendStepPositions(const uint8_t* steps, size_t count);
