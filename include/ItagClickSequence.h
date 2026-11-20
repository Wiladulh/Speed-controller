#pragma once

#include <stdint.h>

class ItagClickSequence {
public:
    void feedButtonEdge(bool pressed, uint32_t nowMs);
    uint8_t service(uint32_t nowMs);
    void reset();

private:
    static constexpr uint32_t CLICK_GAP_MS = 500;
    static constexpr uint32_t MAX_TAP_MS = 800;
    bool buttonDown_ = false;
    uint32_t pressStartedAt_ = 0;
    uint32_t lastReleaseAt_ = 0;
    uint8_t clickCount_ = 0;
};
