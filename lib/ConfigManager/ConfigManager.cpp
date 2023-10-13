#include "ConfigManager.h"

// this section is for the implementation
ConfigManager::ConfigManager(const char* filename) {
    this->filename = filename;

    // start LittleFS
    if (!LittleFS.begin()) {
        // GGTAAF(LOG_LEVEL_ERROR, "Could not initialize LittleFS");
        return;
    }
    
    // load config
    this->load_config();
}

ConfigManager::ConfigManager() {
    this->filename = DEFAULT_CONFIG_FILE;

    // start LittleFS
    if (!LittleFS.begin()) {
        // GGTAAF(LOG_LEVEL_ERROR, "Could not initialize LittleFS");
        return;
    }

    // load config
    this->load_config();
}

ConfigManager::~ConfigManager() { }

// load config from LittleFS
void ConfigManager::load_config() {
    //const char* log_msg = "Loading config from ";
    //log_msg += this->filename;
    // GGTAAF(LOG_LEVEL_INFO, log_msg);
    
    // Open file for reading
    File file = LittleFS.open(this->filename, "r");

    if (!file) {
        // GGTAAF(LOG_LEVEL_ERROR, "Failed to open config file");
        return;
    }
    // GGTAAF(LOG_LEVEL_INFO, "File opened");

    // Allocate a buffer to store contents of the file
    StaticJsonDocument<JSON_BUFFER_SIZE> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        // GGTAAF(LOG_LEVEL_ERROR, "Failed to read file, using default configuration");
        return;
    }

    // Copy values from the JsonDocument to the Config
    strlcpy(this->config.wifi.ssid, doc["wifi"]["ssid"] | "", sizeof(this->config.wifi.ssid));
    strlcpy(this->config.wifi.password, doc["wifi"]["password"] | "", sizeof(this->config.wifi.password));
    
    // Copy system config
    this->config.system.auto_sleep = doc["system"]["auto_sleep"] | false;
    this->config.system.auto_sleep_after = doc["system"]["auto_sleep_after"] | 300;

    // Feed
    this->config.feed.quantity = doc["feed"]["quantity"] | 0;
    this->config.feed.factor = doc["feed"]["factor"] | 1.0;

    // Number of timers in config file
    int timers = doc["timers"].size();

    this->config.timer_list.timers = (timer_config_t*) malloc(timers * sizeof(timer_config_t));
    this->config.timer_list.num_timers = timers;

    // Copy values from the JsonDocument to the Config
    for (int i = 0; i < timers; i++) {
        this->config.timer_list.timers[i].time = this->get_time_from_string(doc["timers"][i]["time"]);
        this->config.timer_list.timers[i].enabled = doc["timers"][i]["enabled"] | false;
        strlcpy(this->config.timer_list.timers[i].name, doc["timers"][i]["name"] | "", sizeof(this->config.timer_list.timers[i].name));

        this->config.timer_list.timers[i].monday = doc["timers"][i]["days"]["monday"] | false;
        this->config.timer_list.timers[i].tuesday = doc["timers"][i]["days"]["tuesday"] | false;
        this->config.timer_list.timers[i].wednesday = doc["timers"][i]["days"]["wednesday"] | false;
        this->config.timer_list.timers[i].thursday = doc["timers"][i]["days"]["thursday"] | false;
        this->config.timer_list.timers[i].friday = doc["timers"][i]["days"]["friday"] | false;
        this->config.timer_list.timers[i].saturday = doc["timers"][i]["days"]["saturday"] | false;
        this->config.timer_list.timers[i].sunday = doc["timers"][i]["days"]["sunday"] | false;
    }

    // Print file to Serial
    //serializeJson(doc, Serial);
    //Serial.println("\n");

    // Close the file (Curiously, File's destructor doesn't close the file)
    file.close();
}

// save config to LittleFS
void ConfigManager::save_config() {
    // open file for writing
    File file = LittleFS.open(this->filename, "w");

    if (!file) {
        // GGTAAF(LOG_LEVEL_ERROR, "Failed to create file");
        return;
    }

    // allocate a buffer to store contents of the file
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);

    // Set the values in the document
    doc["wifi"]["ssid"] = this->config.wifi.ssid;
    doc["wifi"]["password"] = this->config.wifi.password;

    // Set system config
    doc["system"]["auto_sleep"] = this->config.system.auto_sleep;
    doc["system"]["auto_sleep_after"] = this->config.system.auto_sleep_after;

    // feed
    doc["feed"]["quantity"] = this->config.feed.quantity;
    doc["feed"]["factor"] = this->config.feed.factor;

    // timers
    for (size_t i = 0; i < this->config.timer_list.num_timers && i <= MAX_TIMERS; i++) {
        JsonObject timer = doc["timers"].createNestedObject();

        timer["time"] = this->time_to_string(this->config.timer_list.timers[i].time);
        timer["enabled"] = this->config.timer_list.timers[i].enabled;
        timer["name"] = this->config.timer_list.timers[i].name;

        JsonObject days = timer.createNestedObject("days");
        days["monday"] = this->config.timer_list.timers[i].monday;
        days["tuesday"] = this->config.timer_list.timers[i].tuesday;
        days["wednesday"] = this->config.timer_list.timers[i].wednesday;
        days["thursday"] = this->config.timer_list.timers[i].thursday;
        days["friday"] = this->config.timer_list.timers[i].friday;
        days["saturday"] = this->config.timer_list.timers[i].saturday;
        days["sunday"] = this->config.timer_list.timers[i].sunday;
    }

    // serialize JSON to file
    if (serializeJson(doc, file) == 0) {
        // GGTAAF(LOG_LEVEL_ERROR, "Failed to write to file");
    }

    // print file to Serial
    //serializeJson(doc, Serial);
    //Serial.println("\n");

    // Close the file
    file.close();
}

timer_time_t ConfigManager::get_time_from_string(String time) {
    timer_time_t timer_time = {0, 0};

    if (time.length() != 5) {
        // GGTAAF(LOG_LEVEL_ERROR, "Time string has wrong length");
        return timer_time;
    }

    int hour = time.substring(0, 2).toInt();
    int minute = time.substring(3, 5).toInt();

    if (hour < 0 || hour > 23) {
        // GGTAAF(LOG_LEVEL_ERROR, "Hour out of range");
        return timer_time;
    }

    if (minute < 0 || minute > 59) {
        // GGTAAF(LOG_LEVEL_ERROR, "Minute out of range");
        return timer_time;
    }

    timer_time.hour = hour;
    timer_time.minute = minute;

    return timer_time;
}

// wifi getter and setter
void ConfigManager::set_wifi_ssid(const char* ssid) {
    strlcpy(this->config.wifi.ssid, ssid, sizeof(this->config.wifi.ssid));
}

const char* ConfigManager::get_wifi_ssid() {
    return this->config.wifi.ssid;
}

void ConfigManager::set_wifi_password(const char* password) {
    strlcpy(this->config.wifi.password, password, sizeof(this->config.wifi.password));
}

const char* ConfigManager::get_wifi_password() {
    return this->config.wifi.password;
}

// timer getter and setter
timer_config_t ConfigManager::get_timer(int id) {
    if (id < 0 || id >= (int)this->config.timer_list.num_timers) {
        // GGTAAF(LOG_LEVEL_ERROR, "timer ID out of range");
        return this->config.timer_list.timers[id];
    }

    return this->config.timer_list.timers[id];
}

size_t ConfigManager::get_num_timers() {
    return this->config.timer_list.num_timers;
}

timer_config_list_t ConfigManager::get_timers() {
    timer_config_list_t timers;
    timers.timers = this->config.timer_list.timers;
    timers.num_timers = this->config.timer_list.num_timers;

    return timers;
}

StaticJsonDocument<JSON_BUFFER_SIZE> ConfigManager::get_timers_json() {
    this->load_config();

    StaticJsonDocument<JSON_BUFFER_SIZE> doc;

    JsonArray timers = doc.createNestedArray("timers");

    for (size_t i = 0; i < this->config.timer_list.num_timers && i <= MAX_TIMERS; i++) {
        JsonObject timer = timers.createNestedObject();

        timer["time"] = this->time_to_string(this->config.timer_list.timers[i].time);
        timer["enabled"] = this->config.timer_list.timers[i].enabled;
        timer["name"] = this->config.timer_list.timers[i].name;

        JsonObject days = timer.createNestedObject("days");
        days["monday"] = this->config.timer_list.timers[i].monday;
        days["tuesday"] = this->config.timer_list.timers[i].tuesday;
        days["wednesday"] = this->config.timer_list.timers[i].wednesday;
        days["thursday"] = this->config.timer_list.timers[i].thursday;
        days["friday"] = this->config.timer_list.timers[i].friday;
        days["saturday"] = this->config.timer_list.timers[i].saturday;
        days["sunday"] = this->config.timer_list.timers[i].sunday;
    }

    // feed
    doc["feed"]["quantity"] = this->config.feed.quantity;

    return doc;
}

void ConfigManager::set_timers_json(JsonVariant &json) {

    // feed
    this->config.feed.quantity = json["feed"]["quantity"] | 0;

    // timers
    JsonArray timers = json["timers"];

    for (size_t i = 0; i < timers.size() && i <= MAX_TIMERS; i++) {
        JsonObject timer = timers[i];

        this->config.timer_list.timers[i].time = this->get_time_from_string(timer["time"]);
        this->config.timer_list.timers[i].enabled = timer["enabled"] | false;
        strlcpy(this->config.timer_list.timers[i].name, timer["name"] | "", sizeof(this->config.timer_list.timers[i].name));

        this->config.timer_list.timers[i].monday = timer["days"]["monday"] | false;
        this->config.timer_list.timers[i].tuesday = timer["days"]["tuesday"] | false;
        this->config.timer_list.timers[i].wednesday = timer["days"]["wednesday"] | false;
        this->config.timer_list.timers[i].thursday = timer["days"]["thursday"] | false;
        this->config.timer_list.timers[i].friday = timer["days"]["friday"] | false;
        this->config.timer_list.timers[i].saturday = timer["days"]["saturday"] | false;
        this->config.timer_list.timers[i].sunday = timer["days"]["sunday"] | false;
    }

    // set number of timers
    if (timers.size() > MAX_TIMERS) {
        this->config.timer_list.num_timers = MAX_TIMERS;
    } else {
        this->config.timer_list.num_timers = timers.size();
    }

    // print timers to Serial
    //this->print_timers();

    this->save_config();
}

int ConfigManager::get_quantity() {
    return this->config.feed.quantity;
}

float ConfigManager::get_factor() {
    return this->config.feed.factor;
}

void ConfigManager::set_factor(float factor) {
    this->config.feed.factor = factor;
}

String ConfigManager::time_to_string(timer_time_t time) {
    String time_string = "";

    if (time.hour < 10) {
        time_string += "0";
    }
    time_string += time.hour;
    time_string += ":";

    if (time.minute < 10) {
        time_string += "0";
    }
    time_string += time.minute;

    return time_string;
}

timer_config_list_t ConfigManager::sort_timers_by_time(timer_config_list_t timers) {
    timer_config_list_t sorted_timers;
    sorted_timers.timers = (timer_config_t*) malloc(sizeof(timer_config_t) * timers.num_timers);
    sorted_timers.num_timers = 0;

    for (size_t i = 0; i < timers.num_timers; i++) {
        timer_config_t timer = timers.timers[i];

        if (sorted_timers.num_timers == 0) {
            sorted_timers.timers[sorted_timers.num_timers] = timer;
            sorted_timers.num_timers++;
        } else {
            bool inserted = false;

            // as long as the timer has not been inserted, compare it with the already inserted timers
            for (size_t j = 0; j < sorted_timers.num_timers; j++) {
                timer_config_t sorted_timer = sorted_timers.timers[j];

                if (timer.time.hour < sorted_timer.time.hour) {
                    // move all timers one position back
                    for (size_t k = sorted_timers.num_timers; k > j; k--) {
                        sorted_timers.timers[k] = sorted_timers.timers[k - 1];
                    }

                    // insert timer
                    sorted_timers.timers[j] = timer;
                    sorted_timers.num_timers++;

                    inserted = true;
                    break;
                } else if (timer.time.hour == sorted_timer.time.hour && timer.time.minute < sorted_timer.time.minute) {
                    // move all timers one position back
                    for (size_t k = sorted_timers.num_timers; k > j; k--) {
                        sorted_timers.timers[k] = sorted_timers.timers[k - 1];
                    }

                    // insert timer
                    sorted_timers.timers[j] = timer;
                    sorted_timers.num_timers++;

                    inserted = true;
                    break;
                }
            }

            // if the timer was not inserted, insert it at the end because it has the largest time
            if (!inserted) {
                sorted_timers.timers[sorted_timers.num_timers] = timer;
                sorted_timers.num_timers++;
            }
        }
    }

    return sorted_timers;
}

feed_config_t ConfigManager::get_feed_config() {
    return this->config.feed;
}

system_t ConfigManager::get_system_config() {
    return this->config.system;
}

// debugging functions
void ConfigManager::print_config() {
    //const char* log_msg = "Config:";
    //log_msg += "\nWifi:";
    //log_msg += "\n  SSID: ";
    //log_msg += this->config.wifi.ssid;
    //log_msg += "\n  Password: ";
    //log_msg += this->config.wifi.password;

    //log_msg += "\nTimers:";
    for (size_t i = 0; i < this->config.timer_list.num_timers; i++) {
        //log_msg += "\n  Timer ";
        //log_msg += i;
        //log_msg += ":";
        //log_msg += "\n    Time: ";
        //log_msg += this->time_to_string(this->config.timer_list.timers[i].time);
        //log_msg += "\n    Enabled: ";
        //log_msg += this->config.timer_list.timers[i].enabled;
        //log_msg += "\n    Name: ";
        //log_msg += this->config.timer_list.timers[i].name;
        //log_msg += "\n    Monday: ";
        //log_msg += this->config.timer_list.timers[i].monday;
        //log_msg += "\n    Tuesday: ";
        //log_msg += this->config.timer_list.timers[i].tuesday;
        //og_msg += "\n    Wednesday: ";
        //log_msg += this->config.timer_list.timers[i].wednesday;
        //log_msg += "\n    Thursday: ";
        //log_msg += this->config.timer_list.timers[i].thursday;
        //log_msg += "\n    Friday: ";
        //log_msg += this->config.timer_list.timers[i].friday;
        //log_msg += "\n    Saturday: ";
        //log_msg += this->config.timer_list.timers[i].saturday;
        //log_msg += "\n    Sunday: ";
        //log_msg += this->config.timer_list.timers[i].sunday;
    }

    //log_msg += "\nFeed:";
    //log_msg += "\n  Quantity: ";
    //log_msg += this->config.feed.quantity;
    
    // GGTAAF(LOG_LEVEL_INFO, log_msg);
}

void ConfigManager::print_timers() {
    //const char* log_msg = "[";

    for (size_t i = 0; i < this->config.timer_list.num_timers; i++) {
        //log_msg += "{";
        //log_msg += "\"time\": \"";
        //log_msg += this->time_to_string(this->config.timer_list.timers[i].time);
        //log_msg += "\", \"enabled\": ";
        //log_msg += this->config.timer_list.timers[i].enabled;
        //log_msg += ", \"name\": \"";
        //log_msg += this->config.timer_list.timers[i].name;
        //log_msg += "\", \"days\": {";
        //log_msg += "\"monday\": ";
        //log_msg += this->config.timer_list.timers[i].monday;
        //log_msg += ", \"tuesday\": ";
        //log_msg += this->config.timer_list.timers[i].tuesday;
        //log_msg += ", \"wednesday\": ";
        //log_msg += this->config.timer_list.timers[i].wednesday;
        //log_msg += ", \"thursday\": ";
        //log_msg += this->config.timer_list.timers[i].thursday;
        //log_msg += ", \"friday\": ";
        //log_msg += this->config.timer_list.timers[i].friday;
        //log_msg += ", \"saturday\": ";
        //log_msg += this->config.timer_list.timers[i].saturday;
        //log_msg += ", \"sunday\": ";
        //log_msg += this->config.timer_list.timers[i].sunday;
        //log_msg += "}}";

        if (i < this->config.timer_list.num_timers - 1) {
            //log_msg += ", ";
        }
    }

    //log_msg += "]";

    // GGTAAF(LOG_LEVEL_INFO, log_msg);
}