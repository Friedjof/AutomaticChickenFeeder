#include <Arduino.h>

#include <ESPAsyncWebServer.h>

#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#ifdef ESP32DEV
#include <WiFi.h>
#else
#include <ESPAsyncTCP.h>
#include <ESP8266WiFi.h>
#endif

#include <SPI.h>

#include <time.h>

#ifndef __CLOCK_H__
#define __CLOCK_H__
#include <Wire.h>
#include <DS3231.h>
#endif

#include <ConfigManager.h>
#include <AlertManager.h>
#include <LoggingManager.h>
#include <ClockService.h>


#define VERSION "1.0.0"

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

ClockService   clockService;
LoggingManager loggingManager(clockService);
ConfigManager  configManager(CONFIG_FILE);
AlertManager   alertManager(configManager, clockService);

void setup() {
  Serial.begin(115200);

  delay(2000);

  loggingManager.log(LOG_LEVEL_INFO, "Start Chicken Feeder");

  // FGTAAF(LOG_LEVEL_INFO, "Start Chicken Feeder");

  // Setup of the alert manager
  alertManager.setup();

  // Setup of the logging manager
  loggingManager.set_log_level(LOG_LEVEL_INFO);

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
    // FGTAAF(LOG_LEVEL_INFO, "Wakeup caused by external signal using RTC_IO");

    // feed the chickens
    feed();

    // go to sleep
    // FGTAAF(LOG_LEVEL_INFO, "sleep mode activated");

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
    // FGTAAF(LOG_LEVEL_INFO, "Going to sleep because of auto sleep");
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
  // FGTAAF(LOG_LEVEL_INFO, "feeding started");

  digitalWrite(RELAY_PIN, HIGH);

  // feed for a certain time
  delay(configManager.get_quantity() * configManager.get_factor());

  digitalWrite(RELAY_PIN, LOW);

  interrupt_flag = false;

  // FGTAAF(LOG_LEVEL_INFO, "feeding finished");

  // Setup the new alert (if necessary)
  alertManager.set_next_alert();
}

// Will be executed when a new request is received
void new_request() {
  // reset auto sleep millis to prevent going to sleep
  auto_sleep_millis = millis();
}

void setup_wifi() {
  // FGTAAF(LOG_LEVEL_INFO, "Setup WiFi");
  // Wait for connection
  while (strcmp(configManager.get_wifi_ssid(), "") == 0 || strcmp(configManager.get_wifi_password(), "") == 0) {
    delay(1000);
  }

  // FGTAAF(LOG_LEVEL_INFO, "Starte WiFi Access Point");
  WiFi.softAP(configManager.get_wifi_ssid(), configManager.get_wifi_password());

  //std::string log_msg = "Hotspot-SSID: ";
  //log_msg += (std::string)configManager.get_wifi_ssid();

  // FGTAAF(LOG_LEVEL_INFO, log_msg);

  //log_msg = "Hotspot-IP-Adresse: ";
  //log_msg += WiFi.softAPIP().toString();
  // FGTAAF(LOG_LEVEL_INFO, log_msg);
}

void setup_aws() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    new_request();
    // FGTAAF(LOG_LEVEL_INFO, "GET /");

    request->send(LittleFS, INDEX_FILE, "text/html");
  });
  
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, CSS_FILE, "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, JS_FILE, "text/javascript");
  });

  server.on("/logging", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, DEFAULT_LOG_FILE, "text/plain");
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    new_request();
    // FGTAAF(LOG_LEVEL_INFO, "GET /get [configuration]");

    String jsonString;
    serializeJson(configManager.get_timers_json(), jsonString);

    request->send(200, "application/json", jsonString);
  });

  // This endpoint is used to set the timers
  AsyncCallbackJsonWebHandler* set_handler = new AsyncCallbackJsonWebHandler("/set", [](AsyncWebServerRequest *request, JsonVariant &json) {
    new_request();
    // FGTAAF(LOG_LEVEL_INFO, "POST /set [configuration]");

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
    // FGTAAF(LOG_LEVEL_INFO, "GET /sleep [start sleep mode]");
    
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
    // FGTAAF(LOG_LEVEL_INFO, "POST /feed [feed manually]");

    // if 'on' is true, start feeding
    if (json.containsKey("on") && json["on"].is<bool>()) {
      if (json["on"].as<bool>()) {
        // FGTAAF(LOG_LEVEL_INFO, "Start feeding");
        digitalWrite(RELAY_PIN, HIGH);
      } else {
        // FGTAAF(LOG_LEVEL_INFO, "Stop feeding");
        digitalWrite(RELAY_PIN, LOW);
      }
    }

    request->send(200);
  });

  server.addHandler(feed_handler);

  // Webserver starten
  // FGTAAF(LOG_LEVEL_INFO, "Starte Webserver");
  server.begin();
}