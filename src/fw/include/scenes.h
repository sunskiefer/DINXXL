#pragma once

#include <stdint.h>
#include "scene_types.h"

void enterSceneMode();
void exitSceneMode();
void renderSceneModeLeds();
void handleSceneMorph();
bool captureScene(uint8_t pattern, uint8_t slot);
bool scheduleSceneMorph(uint8_t pattern, uint8_t slot);
void sceneHandleTopButton(uint8_t slot, bool value);
void maintainSceneModeStatusLeds();
void sceneHandleBottomButton(uint8_t slot, bool value);
