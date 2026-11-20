#pragma once

#include <Arduino.h>
#include <vector>

class BoardProfile {
public:
    // Conservative output lists for the named PlatformIO board environments.
    // These are starting profiles; verify the actual module/board schematic.
    static std::vector<int> outputCapablePins() {
#if defined(BOARD_PROFILE_ESP32_S3)
        return {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 38, 39, 40, 41, 42, 47, 48};
#elif defined(BOARD_PROFILE_ESP32_C3)
        return {3, 4, 5, 6, 7, 10};
#else
        return {4, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
#endif
    }

    static bool isAllowedOutput(int pin) {
        const auto pins = outputCapablePins();
        for (int candidate : pins) {
            if (candidate == pin) return true;
        }
        return false;
    }

    static bool isAllowedInput(int pin) {
        return isAllowedOutput(pin);
    }
};
