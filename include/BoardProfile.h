#pragma once

#include <Arduino.h>
#include <vector>

class BoardProfile {
public:
    // Conservative GPIO allowlists for the selected PlatformIO board profile.
    // ESP32-C3 Super Mini: GPIO3-6 are reserved for relay-driver outputs;
    // GPIO0,1,2,7,10 are available for button inputs. GPIO2 is a strapping
    // pin: keep its boot level HIGH with external bias and avoid a button
    // wiring that can hold it LOW during reset.
    static std::vector<int> outputCapablePins() {
#if defined(BOARD_PROFILE_ESP32_C3)
        return {3, 4, 5, 6};
#elif defined(BOARD_PROFILE_ESP32_S3)
        return {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 38, 39, 40, 41, 42, 47, 48};
#else
        return {4, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33};
#endif
    }

    static std::vector<int> inputCapablePins() {
#if defined(BOARD_PROFILE_ESP32_C3)
        return {0, 1, 2, 7, 10};
#elif defined(BOARD_PROFILE_ESP32_S3)
        return {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 38, 39, 40, 41, 42, 47, 48};
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
        const auto pins = inputCapablePins();
        for (int candidate : pins) {
            if (candidate == pin) return true;
        }
        return false;
    }
};
