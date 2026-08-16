#include <Arduino.h>
#include <unity.h>
#include "VibrationService.hpp"
// Unused directly, but its inclusion here makes `pio test`'s LDF correctly
// resolve RTClib's SPI dependency for this test target (matches the working
// chain in test_config_service; without it, Adafruit BusIO fails to build
// with a missing SPI.h in isolated `pio test` runs, even though it's fine
// through the same chain in a normal `pio run`).
#include "ClockService.hpp"

VibrationService vibrationService;

void setUp(void) {
    // Make sure every test starts from an off/idle state
    vibrationService.endFeedShake(0);
}

void tearDown(void) {
}

void test_initial_state_inactive(void) {
    TEST_ASSERT_FALSE(vibrationService.isActive());
}

void test_start_feed_shake_sets_active(void) {
    vibrationService.startFeedShake();
    TEST_ASSERT_TRUE(vibrationService.isActive());
}

void test_end_feed_shake_with_zero_tail_turns_off_immediately(void) {
    vibrationService.startFeedShake();
    vibrationService.endFeedShake(0);
    TEST_ASSERT_FALSE(vibrationService.isActive());
}

void test_end_feed_shake_with_tail_stays_active_until_elapsed(void) {
    vibrationService.startFeedShake();
    vibrationService.endFeedShake(100);

    // Tail still running
    vibrationService.update();
    TEST_ASSERT_TRUE(vibrationService.isActive());

    delay(120);
    vibrationService.update();
    TEST_ASSERT_FALSE(vibrationService.isActive());
}

void test_trigger_pulse_turns_on_and_auto_off_after_duration(void) {
    vibrationService.triggerPulse(80);
    TEST_ASSERT_TRUE(vibrationService.isActive());

    delay(100);
    vibrationService.update();
    TEST_ASSERT_FALSE(vibrationService.isActive());
}

void test_trigger_pulse_is_noop_while_feed_shake_forced(void) {
    vibrationService.startFeedShake();
    vibrationService.triggerPulse(10); // should be ignored - feed shake stays forced on

    delay(30);
    vibrationService.update();
    TEST_ASSERT_TRUE(vibrationService.isActive());

    vibrationService.endFeedShake(0);
    TEST_ASSERT_FALSE(vibrationService.isActive());
}

void setup() {
    // Wait for serial monitor to connect before running tests
    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_initial_state_inactive);
    RUN_TEST(test_start_feed_shake_sets_active);
    RUN_TEST(test_end_feed_shake_with_zero_tail_turns_off_immediately);
    RUN_TEST(test_end_feed_shake_with_tail_stays_active_until_elapsed);
    RUN_TEST(test_trigger_pulse_turns_on_and_auto_off_after_duration);
    RUN_TEST(test_trigger_pulse_is_noop_while_feed_shake_forced);

    UNITY_END();
}

void loop() {
    delay(100);
}
