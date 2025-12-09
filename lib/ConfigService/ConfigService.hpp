#ifndef CONFIG_SERVICE_HPP
#define CONFIG_SERVICE_HPP

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>

#define MAX_SCHEDULES 6

struct Schedule {
    uint8_t id;
    bool enabled;
    char time[6];  // "HH:MM"
    uint8_t weekday_mask;  // Bit 0=Sunday, 1=Monday, ..., 6=Saturday
    uint8_t portion_units; // 1-5 units (12g each)
};

class ConfigService {
public:
    ConfigService();

    bool begin();

    // Schedule management
    bool loadSchedule(uint8_t index, Schedule &schedule);
    bool saveSchedule(uint8_t index, const Schedule &schedule);
    bool loadAllSchedules(Schedule schedules[MAX_SCHEDULES]);
    bool saveAllSchedules(const Schedule schedules[MAX_SCHEDULES]);

    // Config metadata
    uint8_t getPortionUnitGrams();
    void setPortionUnitGrams(uint8_t grams);

    // Reset to defaults
    bool resetToDefaults();

private:
    Preferences preferences;
    uint8_t portionUnitGrams;

    void getScheduleKey(uint8_t index, char *key);
};

#endif // CONFIG_SERVICE_HPP
