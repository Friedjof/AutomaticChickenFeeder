#include <Arduino.h>

#include <ESPAsyncWebServer.h>

#include <AsyncJson.h>

#ifdef ESP32DEV
#include <WiFi.h>
#else
#include <ESPAsyncTCP.h>
#include <ESP8266WiFi.h>
#endif

// Own libraries from lib/
#include <ConfigManager.h>
#include <AlertManager.h>
#include <LoggingManager.h>
#include <ClockService.h>


#define VERSION "0.1.2"

#define CONFIG_FILE "/config.json"

#define FEED_FACTOR 1 // factor to match the mass of the food

#define INDEX_FILE "/index.html"
#define CSS_FILE "/style.css"
#define JS_FILE "/script.js"

#define RELAY_PIN 2  // D2 (Onboard LED)
#define CLINT 4      // RTC interrupt pin for alarm 1

// Prototypes
void IRAM_ATTR interrupt_handler();
void setup_wifi();  // Setup WiFi
void setup_aws();   // Setup AsyncWebServer

void feed();        // Feed the chickens
void new_request(); // Will be executed when a new request is received

// Global variables
bool interrupt_flag = false;
unsigned long feed_millis = millis();
unsigned long auto_sleep_millis = millis();

// deep sleep
#ifdef ESP32DEV
esp_sleep_wakeup_cause_t wakeup_reason;
#else
String wakeup_reason;
#endif

AsyncWebServer server(80);

ClockService clockService;

LoggingManager loggingManager(clockService);
ConfigManager configManager(DEFAULT_CONFIG_FILE, loggingManager);
AlertManager alertManager(configManager, loggingManager, clockService);

void setup() {
  Serial.begin(115200);

  // check if LittleFS is mounted
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }

  // Start ClockService
  clockService.begin();

  // Setup of the LoggingManager
  loggingManager.set_log_level(LOG_LEVEL_INFO);
  loggingManager.set_file_log_level(LOG_LEVEL_INFO_FILE);

  // Start LoggingManager
  loggingManager.begin();
  // Start ConfigManager
  configManager.begin();
  // Start AlertManager
  alertManager.begin();

  // Log the start of the program
  loggingManager.log(LOG_LEVEL_INFO_FILE, "Start Chicken Feeder");

  // print next alert
  optional_ds3231_timer_t next_alert = alertManager.get_next_alert();
  if (!next_alert.empty) {
    alertManager.print_timer(next_alert.timer);
  }

  // Setup PIN interrupt
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(CLINT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLINT), interrupt_handler, FALLING);

  #ifdef ESP32DEV
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0);

  wakeup_reason = esp_sleep_get_wakeup_cause();
  bool timer_wakeup = wakeup_reason == ESP_SLEEP_WAKEUP_EXT0;
  #else
  ESP.deepSleep(0);

  wakeup_reason = ESP.getResetReason();
  bool timer_wakeup = wakeup_reason == "Deep-Sleep Wake";
  #endif

  if (timer_wakeup) {
    loggingManager.log(LOG_LEVEL_INFO_FILE, "Wakeup caused by external signal using RTC_IO");

    // feed the chickens
    feed();

    // go to sleep
    loggingManager.log(LOG_LEVEL_INFO_FILE, "sleep mode activated");

    #ifdef ESP32DEV
    esp_deep_sleep_start();
    #else
    ESP.deepSleep(0);
    #endif
  } else {
    // Setup WiFi
    setup_wifi();

    // Setup AsyncWebServer
    setup_aws();
  }
}

void loop() {
  // handle interrupt
  if (interrupt_flag) {
    feed();
  }

  // auto sleep depending on AUTO_SLEEP
  if (configManager.get_system_config().auto_sleep && (millis() - auto_sleep_millis > (long unsigned int)(1000 * configManager.get_system_config().auto_sleep_after))) {
    loggingManager.log(LOG_LEVEL_INFO_FILE, "Going to sleep because of auto sleep");
    #ifdef ESP32DEV
    esp_deep_sleep_start();
    #else
    ESP.deepSleep(0);
    #endif
  }
}

// Interrupt handler
void IRAM_ATTR interrupt_handler() {
  interrupt_flag = true;
}

// Feed the chickens
void feed() {
  loggingManager.log(LOG_LEVEL_INFO_FILE, "feeding started");

  digitalWrite(RELAY_PIN, HIGH);

  // feed for a certain time
  delay(configManager.get_quantity() * configManager.get_factor());

  digitalWrite(RELAY_PIN, LOW);

  interrupt_flag = false;

  loggingManager.log(LOG_LEVEL_INFO, "feeding finished");

  // Setup the new alert (if necessary)
  alertManager.set_next_alert();
}

// Will be executed when a new request is received
void new_request() {
  // reset auto sleep millis to prevent going to sleep
  auto_sleep_millis = millis();
}

void setup_wifi() {
  loggingManager.log(LOG_LEVEL_INFO, "Setup WiFi");
  // Wait for connection
  while (strcmp(configManager.get_wifi_ssid(), "") == 0 || strcmp(configManager.get_wifi_password(), "") == 0) {
    delay(1000);
  }

  loggingManager.log(LOG_LEVEL_INFO, "Starte WiFi Access Point");
  WiFi.softAP(configManager.get_wifi_ssid(), configManager.get_wifi_password());

  loggingManager.start_seq(LOG_LEVEL_INFO, "Hotspot-SSID: '");
  loggingManager.append_seq(configManager.get_wifi_ssid());
  loggingManager.end_seq("'");

  loggingManager.start_seq(LOG_LEVEL_INFO, "Hotspot-IP-Adresse: '");
  loggingManager.append_seq(WiFi.softAPIP().toString().c_str());
  loggingManager.end_seq("'");
}

void setup_aws() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    new_request();
    loggingManager.log(LOG_LEVEL_INFO, "GET /");

    request->send(LittleFS, INDEX_FILE, "text/html");
  });
  
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, CSS_FILE, "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, JS_FILE, "text/javascript");
  });

  server.on("/logging", HTTP_GET, [](AsyncWebServerRequest *request) {
    new_request();
    
    request->send(200, "text/plain", loggingManager.get_logs());
  });

  server.on("/log_lines", HTTP_GET, [](AsyncWebServerRequest *request) {
    new_request();

    int line_counter = loggingManager.count_log_lines() + 1;
    int file_line_counter = loggingManager.get_file_line_counter() + 1;

    loggingManager.start_seq(LOG_LEVEL_INFO_FILE, "GET /log_lines [");
    loggingManager.append_seq(line_counter);
    loggingManager.append_seq(", ");
    loggingManager.append_seq(file_line_counter);
    loggingManager.end_seq("]");

    String jsonString = "{";
    jsonString += "\"log_lines\":";
    jsonString += line_counter;
    jsonString += ",";
    jsonString += "\"counted_lines\":";
    jsonString += file_line_counter;
    jsonString += ",";
    jsonString += "\"log_level\":";
    jsonString += loggingManager.get_log_level();
    jsonString += ",";
    jsonString += "\"file_log_level\":";
    jsonString += loggingManager.get_file_log_level();
    jsonString += "}";
    
    request->send(200, "text/json", jsonString);
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    new_request();
    loggingManager.log(LOG_LEVEL_INFO, "GET /get [configuration]");

    String jsonString;
    serializeJson(configManager.get_timers_json(), jsonString);

    request->send(200, "application/json", jsonString);
  });

  // This endpoint is used to set the timers
  AsyncCallbackJsonWebHandler* set_handler = new AsyncCallbackJsonWebHandler("/set", [](AsyncWebServerRequest *request, JsonVariant &json) {
    new_request();
    loggingManager.log(LOG_LEVEL_INFO, "POST /set [configuration]");

    // Convert the data to a JSON object
    configManager.set_timers_json(json);

    // Setup the new alert (if necessary)
    alertManager.set_next_alert();

    request->send(200);
  });

  server.addHandler(set_handler);

  // activate sleep mode
  server.on("/sleep", HTTP_GET, [](AsyncWebServerRequest *request) {
    new_request();
    loggingManager.log(LOG_LEVEL_INFO_FILE, "GET /sleep [start sleep mode]");
    
    // go to sleep
    #ifdef ESP32DEV
    esp_deep_sleep_start();
    #else
    ESP.deepSleep(0);
    #endif
    request->send(200);
  });

  // get current time
  server.on("/time", HTTP_GET, [](AsyncWebServerRequest *request) {
    new_request();

    // get current time
    ds3231_datetime_t now = alertManager.now();

    // create json object
    StaticJsonDocument<JSON_OBJECT_SIZE(6)> json;
    json["year"] = now.year;
    json["month"] = now.month;
    json["day"] = now.day;
    json["hour"] = now.hour;
    json["minute"] = now.minute;
    json["second"] = now.second;

    String jsonString;
    serializeJson(json, jsonString);

    request->send(200, "application/json", jsonString); 
  });

  // feed manually
  AsyncCallbackJsonWebHandler* feed_handler = new AsyncCallbackJsonWebHandler("/feed", [](AsyncWebServerRequest *request, JsonVariant &json) {
    new_request();
    loggingManager.log(LOG_LEVEL_INFO_FILE, "POST /feed [feed manually]");

    // if 'on' is true, start feeding
    if (json.containsKey("on") && json["on"].is<bool>()) {
      if (json["on"].as<bool>()) {
        loggingManager.log(LOG_LEVEL_INFO, "Start feeding");
        digitalWrite(RELAY_PIN, HIGH);
      } else {
        loggingManager.log(LOG_LEVEL_INFO, "Stop feeding");
        digitalWrite(RELAY_PIN, LOW);
      }
    }

    request->send(200);
  });

  server.addHandler(feed_handler);

  // Webserver starten
  loggingManager.log(LOG_LEVEL_INFO, "Starte Webserver");
  server.begin();
}