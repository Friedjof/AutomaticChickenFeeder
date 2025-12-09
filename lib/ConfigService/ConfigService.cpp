#include "ConfigService.hpp"

ConfigService::ConfigService() : portionUnitGrams(12) {}

bool ConfigService::begin() {
    preferences.begin("feeder", false);

    // Load portion unit grams
    portionUnitGrams = preferences.getUChar("portionGrams", 12);

    Serial.println("[CONFIG] ConfigService initialized");
    Serial.printf("[CONFIG] Portion unit: %d grams\n", portionUnitGrams);

    return true;
}

void ConfigService::getScheduleKey(uint8_t index, char *key) {
    snprintf(key, 16, "sched_%d", index);
}

bool ConfigService::loadSchedule(uint8_t index, Schedule &schedule) {
    if (index >= MAX_SCHEDULES) {
        Serial.printf("[CONFIG] Invalid schedule index: %d\n", index);
        return false;
    }

    char key[16];
    getScheduleKey(index, key);

    // Check if key exists first (avoids NVS error log)
    if (!preferences.isKey(key)) {
        // Return default schedule
        schedule.id = index + 1;
        schedule.enabled = false;
        strncpy(schedule.time, "00:00", 6);
        schedule.weekday_mask = 0;
        schedule.portion_units = 1;
        return true;
    }

    // Load schedule as JSON string
    String jsonStr = preferences.getString(key, "");

    if (jsonStr.isEmpty()) {
        // Return default schedule
        schedule.id = index + 1;
        schedule.enabled = false;
        strncpy(schedule.time, "00:00", 6);
        schedule.weekday_mask = 0;
        schedule.portion_units = 1;
        return true;
    }

    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonStr);

    if (error) {
        Serial.printf("[CONFIG] Failed to parse schedule %d: %s\n", index, error.c_str());
        return false;
    }

    schedule.id = doc["id"] | (index + 1);
    schedule.enabled = doc["enabled"] | false;
    strncpy(schedule.time, doc["time"] | "00:00", 6);
    schedule.weekday_mask = doc["weekday_mask"] | 0;
    schedule.portion_units = doc["portion_units"] | 1;

    return true;
}

bool ConfigService::saveSchedule(uint8_t index, const Schedule &schedule) {
    if (index >= MAX_SCHEDULES) {
        Serial.printf("[CONFIG] Invalid schedule index: %d\n", index);
        return false;
    }

    char key[16];
    getScheduleKey(index, key);

    // Create JSON document
    JsonDocument doc;
    doc["id"] = schedule.id;
    doc["enabled"] = schedule.enabled;
    doc["time"] = schedule.time;
    doc["weekday_mask"] = schedule.weekday_mask;
    doc["portion_units"] = schedule.portion_units;

    // Serialize to string
    String jsonStr;
    serializeJson(doc, jsonStr);

    // Save to preferences
    preferences.putString(key, jsonStr);

    Serial.printf("[CONFIG] Saved schedule %d: %s\n", index, jsonStr.c_str());

    return true;
}

bool ConfigService::loadAllSchedules(Schedule schedules[MAX_SCHEDULES]) {
    for (uint8_t i = 0; i < MAX_SCHEDULES; i++) {
        if (!loadSchedule(i, schedules[i])) {
            return false;
        }
    }
    return true;
}

bool ConfigService::saveAllSchedules(const Schedule schedules[MAX_SCHEDULES]) {
    for (uint8_t i = 0; i < MAX_SCHEDULES; i++) {
        if (!saveSchedule(i, schedules[i])) {
            return false;
        }
    }
    return true;
}

uint8_t ConfigService::getPortionUnitGrams() {
    return portionUnitGrams;
}

void ConfigService::setPortionUnitGrams(uint8_t grams) {
    portionUnitGrams = grams;
    preferences.putUChar("portionGrams", grams);
    Serial.printf("[CONFIG] Portion unit updated to %d grams\n", grams);
}

bool ConfigService::resetToDefaults() {
    Serial.println("[CONFIG] Resetting to defaults...");

    // Default schedules
    Schedule defaults[MAX_SCHEDULES] = {
        {1, false, "06:30", 62, 1},  // Mon-Fri (bit 1-5 = 0b0111110 = 62)
        {2, false, "12:00", 62, 1},
        {3, false, "18:00", 62, 1},
        {4, false, "22:00", 62, 1},
        {5, false, "00:00", 0, 1},
        {6, false, "00:00", 0, 1}
    };

    saveAllSchedules(defaults);
    setPortionUnitGrams(12);

    Serial.println("[CONFIG] Reset complete");
    return true;
}
