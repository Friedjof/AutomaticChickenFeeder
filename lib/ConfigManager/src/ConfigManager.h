#ifndef __CONFIG_CONFIG_MANAGER_H__
#define __CONFIG_CONFIG_MANAGER_H__

#define MAX_TIMERS 4
#define MAX_TIMER_NAME_LENGTH 32
#define MAX_TIMER_TIME_LENGTH 6
#define MAX_WIFI_SSID_LENGTH 32
#define MAX_WIFI_PASSWORD_LENGTH 64
#define MAX_FILENAME_LENGTH 32
#define JSON_BUFFER_SIZE 4096
#define DEFAULT_CONFIG_FILE "/config.json"

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

typedef struct {
    unsigned int hour;
    unsigned int minute;
} timer_time_t;

typedef struct {
    char ssid[MAX_WIFI_SSID_LENGTH];
    char password[MAX_WIFI_PASSWORD_LENGTH];
} local_wifi_config_t;

typedef struct {
    char name[MAX_TIMER_NAME_LENGTH];
    timer_time_t time;
    bool enabled;

    // Weekdays
    bool monday;
    bool tuesday;
    bool wednesday;
    bool thursday;
    bool friday;
    bool saturday;
    bool sunday;
} timer_config_t;

typedef struct {
    timer_config_t* timers;
    size_t num_timers;
} timer_config_list_t;

typedef struct {
    timer_config_list_t timers;
    bool empty;
} optional_timer_config_list_t;


typedef struct {
    int quantity;
} feed_config_t;

typedef struct {
    local_wifi_config_t wifi;
    timer_config_list_t timer_list;
    feed_config_t feed;
} config_t;


class ConfigManager {
    private:
        const char* filename;

    public:
        ConfigManager(const char* filename);
        ConfigManager();
        ~ConfigManager();

        config_t config;

        void load_config();
        void save_config();

        timer_config_list_t sort_timers_by_time(timer_config_list_t timers);

        void set_wifi_config(local_wifi_config_t wifi_config);
        local_wifi_config_t get_wifi_config();
        
        const char* get_wifi_ssid();
        void set_wifi_ssid(const char* ssid);
        const char* get_wifi_password();
        void set_wifi_password(const char* password);

        timer_config_t get_timer(int index);
        size_t get_num_timers();
        timer_config_list_t get_timers();
        StaticJsonDocument<JSON_BUFFER_SIZE> get_timers_json();
        feed_config_t get_feed_config();

        void set_timer(int index, timer_config_t timer_config);
        void set_timers(timer_config_list_t timers, size_t num_timers);
        void set_timers_json(JsonVariant &json);

        String time_to_string(timer_time_t time);
        timer_time_t get_time_from_string(String time);

        void set_timer_enabled(int index, bool enabled);
        void set_timer_time(int index, char* time);
        void set_timer_name(int index, char* name);

        // Debugging
        void print_config();
        void print_timers();
};

#endif