#include <unity.h>

#include "ItagClickSequence.h"

void test_single_click_maps_to_one() {
    ItagClickSequence detector;
    detector.feedButtonEdge(true, 100);
    detector.feedButtonEdge(false, 180);
    TEST_ASSERT_EQUAL_UINT8(1, detector.service(681));
}

void test_double_click_maps_to_two() {
    ItagClickSequence detector;
    detector.feedButtonEdge(true, 100);
    detector.feedButtonEdge(false, 150);
    detector.feedButtonEdge(true, 250);
    detector.feedButtonEdge(false, 300);
    TEST_ASSERT_EQUAL_UINT8(2, detector.service(801));
}

void test_five_clicks_maps_to_off_code() {
    ItagClickSequence detector;
    uint32_t now = 100;
    for (uint8_t i = 0; i < 5; ++i) {
        detector.feedButtonEdge(true, now);
        detector.feedButtonEdge(false, now + 40);
        now += 140;
    }
    TEST_ASSERT_EQUAL_UINT8(5, detector.service(now + 501));
}

void test_long_press_is_ignored() {
    ItagClickSequence detector;
    detector.feedButtonEdge(true, 100);
    detector.feedButtonEdge(false, 1000);
    TEST_ASSERT_EQUAL_UINT8(0, detector.service(1600));
}

void test_click_window_resets_after_gap() {
    ItagClickSequence detector;
    detector.feedButtonEdge(true, 100);
    detector.feedButtonEdge(false, 150);
    detector.feedButtonEdge(true, 800);
    detector.feedButtonEdge(false, 850);
    TEST_ASSERT_EQUAL_UINT8(1, detector.service(1351));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_single_click_maps_to_one);
    RUN_TEST(test_double_click_maps_to_two);
    RUN_TEST(test_five_clicks_maps_to_off_code);
    RUN_TEST(test_long_press_is_ignored);
    RUN_TEST(test_click_window_resets_after_gap);
    return UNITY_END();
}
