#include "ItagClickSequence.h"

void ItagClickSequence::feedButtonEdge(bool pressed, uint32_t nowMs) {
    if (pressed) {
        if (!buttonDown_) {
            buttonDown_ = true;
            pressStartedAt_ = nowMs;
        }
        return;
    }

    if (!buttonDown_) return;
    buttonDown_ = false;
    const uint32_t pressDuration = nowMs - pressStartedAt_;
    if (pressDuration > MAX_TAP_MS) {
        clickCount_ = 0;
        lastReleaseAt_ = 0;
        return;
    }

    if (clickCount_ == 0 || static_cast<uint32_t>(nowMs - lastReleaseAt_) > CLICK_GAP_MS) {
        clickCount_ = 1;
    } else if (clickCount_ < 5) {
        ++clickCount_;
    }
    lastReleaseAt_ = nowMs;
}

uint8_t ItagClickSequence::service(uint32_t nowMs) {
    if (buttonDown_ || clickCount_ == 0 ||
        static_cast<uint32_t>(nowMs - lastReleaseAt_) <= CLICK_GAP_MS) {
        return 0;
    }
    const uint8_t result = clickCount_;
    reset();
    return result;
}

void ItagClickSequence::reset() {
    buttonDown_ = false;
    pressStartedAt_ = 0;
    lastReleaseAt_ = 0;
    clickCount_ = 0;
}
