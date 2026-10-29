#pragma once

#include <stdint.h>

namespace WebToolProtocol {

constexpr uint8_t kManufacturerId = 0x7D;  // Non-commercial ID.
constexpr uint8_t kProductId0 = 0x42;      // 'B'
constexpr uint8_t kProductId1 = 0x42;      // 'B'

constexpr uint8_t kMsgPatternSnapshot = 0x01;
constexpr uint8_t kMsgPatternRequest = 0x02;
constexpr uint8_t kMsgStepPositions = 0x03;
constexpr uint8_t kMsgSetDrumStep = 0x04;
constexpr uint8_t kMsgSetSynthStep = 0x05;

constexpr uint8_t kMsgSettingsRequest = 0x10;
constexpr uint8_t kMsgSettingsDump = 0x11;
constexpr uint8_t kMsgSettingsUpdate = 0x12;

}  // namespace WebToolProtocol

