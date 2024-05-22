#include <Arduino.h>

#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#if defined(ESP32DEV) || defined(ESP32S3)
#include <WiFi.h>
#elif defined(ESP8266)
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

#define CONFIG_FILE "/config.json"

#define INDEX_FILE "/index.html"
#define CSS_FILE "/style.css"
#define JS_FILE "/script.js"

#if defined(ESP32DEV) || defined(ESP8266)
#define RELAY_PIN 2    // D2 (Onboard LED)
#define CLINT 4        // RTC interrupt pin for alarm 1
#define WAKEUP_PIN GPIO_NUM_4
#elif defined(ESP32S3)
#define RELAY_PIN D8   // D2 (Onboard LED)
#define CLINT D9       // RTC interrupt pin for alarm 1
#define WAKEUP_PIN GPIO_NUM_8
#endif

// Prototypes
void IRAM_ATTR interrupt_handler();
void setup_wifi();     // Setup WiFi
void setup_aws();      // Setup AsyncWebServer

void startFeeding();  // Feed the chickens
void stopFeeding();   // Stop feeding the chickens

void new_request();    // Will be executed when a new request is received
void goToSleep();      // Go to sleep
void setSleepTime();     // Reset sleep
long remaining_auto_sleep_time();

// Global variables
bool interrupt_flag = false;
bool feeding[] = {false, false};
bool awakened = false;

unsigned long feed_millis = millis();
unsigned long auto_sleep_millis = millis();

// deep sleep
#if defined(ESP32DEV) || defined(ESP32S3)
esp_sleep_wakeup_cause_t wakeup_reason;
#elif defined(ESP8266)
String wakeup_reason;
#endif

AsyncWebServer server(80);

ConfigManager configManager(CONFIG_FILE);
AlertManager alertManager(configManager);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("[INFO] Starte Hühner-Futterautomat");

  // Setup of the alert manager
  alertManager.setup();

  // print next alert
  optional_ds3231_timer_t next_alert = alertManager.get_next_alert();
  if (!next_alert.empty) {
    alertManager.print_timer(next_alert.timer);
  }

  // Setup PIN interrupt
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(CLINT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLINT), startFeeding, FALLING);

  #if defined(ESP32DEV) || defined(ESP32S3)
  esp_sleep_enable_ext0_wakeup(WAKEUP_PIN, 0);

  wakeup_reason = esp_sleep_get_wakeup_cause();
  bool timer_wakeup = wakeup_reason == ESP_SLEEP_WAKEUP_EXT0;
  #elif defined(ESP8266)
  ESP.deepSleep(0);

  wakeup_reason = ESP.getResetReason();
  bool timer_wakeup = wakeup_reason == "Deep-Sleep Wake";
  #endif

  if (timer_wakeup) {
    Serial.println("[INFO] Wakeup caused by external signal using RTC_IO");
    awakened = true;

    // feed the chickens
    startFeeding();
  } else {
    // Setup WiFi
    Serial.print("[INFO] Waiting for SSID and password…");
    while (strcmp(configManager.get_wifi_ssid(), "") == 0 || strcmp(configManager.get_wifi_password(), "") == 0) {
      Serial.print(".");
      delay(1000);
    }
    Serial.println("[INFO] Done");

    Serial.println("[INFO] Starte WiFi Access Point");
    WiFi.softAP(configManager.get_wifi_ssid(), configManager.get_wifi_password());

    Serial.println();
    Serial.print("[INFO] Hotspot-SSID: ");
    Serial.println(configManager.get_wifi_ssid());
    Serial.print("Hotspot-IP-Adresse: ");
    Serial.println(WiFi.softAPIP());
    Serial.println();

    // Setup AsyncWebServer
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    new_request();

    request->send(LittleFS, INDEX_FILE, "text/html");
    });
    
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, CSS_FILE, "text/css");
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, JS_FILE, "text/javascript");
    });

    server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
      new_request();

      String jsonString;
      
      JsonDocument json;
      configManager.get_timers_json(json);

      serializeJson(json, jsonString);

      json.clear();

      Serial.println("[INFO] Get configuration");

      request->send(200, "application/json", jsonString);
    });

    // This endpoint is used to set the timers
    AsyncCallbackJsonWebHandler* set_handler = new AsyncCallbackJsonWebHandler("/set", [](AsyncWebServerRequest *request, JsonVariant &json) {
      // Convert the data to a JSON object
      Serial.println("[INFO] Set new configuration");

      configManager.set_timers_json(json);

      // Setup the new alert (if necessary)
      alertManager.set_next_alert();

      request->send(200);
    });

    server.addHandler(set_handler);

    // activate sleep mode
    server.on("/sleep", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("[INFO] Schlafmodus aktiviert");
      
      goToSleep();

      request->send(200);
    });

    // get current time
    server.on("/time", HTTP_GET, [](AsyncWebServerRequest *request) {
      new_request();
      
      // get current time
      ds3231_datetime_t now = alertManager.now();

      // create json object
      JsonDocument json;
      json["year"] = now.year;
      json["month"] = now.month;
      json["day"] = now.day;
      json["hour"] = now.hour;
      json["minute"] = now.minute;
      json["second"] = now.second;

      String jsonString;
      serializeJson(json, jsonString);

      json.clear();

      request->send(200, "application/json", jsonString); 
    });

    server.on("/rtc", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, (const char*)data);

      if (error) {
        Serial.print("[ERROR] deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }

      int year = doc["y"];
      int month = doc["m"];
      int day = doc["d"];
      int hour = doc["h"];
      int minute = doc["min"];
      int second = doc["s"];

      doc.clear();

      Serial.printf("[INFO] new datime synced: %d-%d-%d %d:%d:%d\n", year, month, day, hour, minute, second);

      alertManager.set_new_datetime(year, month, day, hour, minute, second);

      request->send(200);
    });

    // get autosleep remaining time
    server.on("/autosleep", HTTP_GET, [](AsyncWebServerRequest *request) {
      // get remaining time
      long remaining = remaining_auto_sleep_time();

      // create json object
      JsonDocument json;
      json["remaining"] = remaining;

      String jsonString;
      serializeJson(json, jsonString);

      json.clear();

      request->send(200, "application/json", jsonString); 
    });

    // feed manually
    AsyncCallbackJsonWebHandler* feed_handler = new AsyncCallbackJsonWebHandler("/feed", [](AsyncWebServerRequest *request, JsonVariant &json) {
      startFeeding();

      request->send(200);
    });

    server.addHandler(feed_handler);

    // Webserver starten
    Serial.println("[INFO] Starte Webserver");
    server.begin();

    setSleepTime();
  }
}

void loop() {
  if (feeding[0] && !feeding[1]) {
    Serial.println("[INFO] feeding");

    digitalWrite(RELAY_PIN, HIGH);
    feeding[1] = true;
  } else if (feeding[1] && !feeding[0]) {
    Serial.println("[INFO] stop feeding");

    digitalWrite(RELAY_PIN, LOW);

    feeding[1] = false;

    if (awakened) {
      goToSleep();
    }
  }

  if (feed_millis < millis() && feeding[1]) {
    stopFeeding();
  }

  if (auto_sleep_millis < millis() && !feeding[1]) {
    goToSleep();
  }
}

void goToSleep() {
  // Set next alert
  alertManager.set_next_alert();

  // go to sleep
  Serial.println("[INFO] Going to sleep because of auto sleep");
  #if defined(ESP32DEV) || defined(ESP32S3)
  esp_deep_sleep_start();
  #elif defined(ESP8266)
  ESP.deepSleep(0);
  #endif
}

void setSleepTime() {
  auto_sleep_millis = millis() + configManager.get_auto_sleep_after();
}

// Interrupt handler
void IRAM_ATTR startFeeding() {
  interrupt_flag = true;
  feeding[0] = true;

  // set the time when the feeding should stop
  feed_millis = millis() + configManager.get_feeding_time();
}

// Feed the chickens
void stopFeeding() {
  interrupt_flag = false;
  feeding[0] = false;
}

// remaining auto sleep time
long remaining_auto_sleep_time() {
  return auto_sleep_millis / 1000 - millis() / 1000;
}

// Will be executed when a new request is received
void new_request() {
  // reset auto sleep millis to prevent going to sleep
  setSleepTime();
}
