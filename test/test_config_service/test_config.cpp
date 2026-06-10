#include <Arduino.h>
#include <unity.h>
#include "ConfigService.hpp"

ConfigService configService;

void setUp(void) {
    // wird VOR jedem Test aufgerufen
    // Sicherstellen, dass Preferences sauber sind für den Test
    Preferences prefs;
    prefs.begin("feeder", false);
    prefs.clear();
    prefs.end();
    
    configService.begin();
}

void tearDown(void) {
    // wird NACH jedem Test aufgerufen
}

void test_default_portion_grams(void) {
    // Da wir setUp() gecleart haben, erwarten wir den Default
    TEST_ASSERT_EQUAL(12, configService.getPortionUnitGrams());
}

void test_set_and_get_portion_grams(void) {
    configService.setPortionUnitGrams(15);
    TEST_ASSERT_EQUAL(15, configService.getPortionUnitGrams());
}

void test_schedule_save_load(void) {
    Schedule s_out;
    s_out.id = 0;
    s_out.enabled = true;
    strcpy(s_out.time, "14:30");
    s_out.weekday_mask = 0b00111110; // Mo-Fr
    s_out.portion_units = 3;

    TEST_ASSERT_TRUE(configService.saveSchedule(0, s_out));

    Schedule s_in;
    TEST_ASSERT_TRUE(configService.loadSchedule(0, s_in));
    
    TEST_ASSERT_EQUAL(s_out.id, s_in.id);
    TEST_ASSERT_TRUE(s_in.enabled);
    TEST_ASSERT_EQUAL_STRING(s_out.time, s_in.time);
    TEST_ASSERT_EQUAL(s_out.weekday_mask, s_in.weekday_mask);
    TEST_ASSERT_EQUAL(s_out.portion_units, s_in.portion_units);
}

void setup() {
    // Wait for serial monitor to connect before running tests
    delay(2000); 

    UNITY_BEGIN();
    
    RUN_TEST(test_default_portion_grams);
    RUN_TEST(test_set_and_get_portion_grams);
    RUN_TEST(test_schedule_save_load);

    UNITY_END();
}

void loop() {
    delay(100);
}
