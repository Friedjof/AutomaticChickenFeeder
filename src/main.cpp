/*
Der Hühner-Futterautomat
Externe Module:
- RTC (Real Time Clock)
  SDA: GPIO 21
  SCL: GPIO 22
*/

#include <Arduino.h>

#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <SPI.h>

#include <time.h>
#include <Wire.h>
#include <DS3231.h>

#include <ConfigManager.h>
#include <AlertManager.h>


#define CONFIG_FILE "/config.json"

#define FEED_FACTOR 1 // factor to match the mass of the food

#define INDEX_FILE "/index.html"
#define CSS_FILE "/style.css"
#define JS_FILE "/script.js"

#define RELAY_PIN 2 // D2 (Onboard LED)
#define CLINT 4     // RTC interrupt pin for alarm 1

// Prototypes
void IRAM_ATTR interrupt_handler();
void setup_wifi();  // Setup WiFi
void setup_aws();   // Setup AsyncWebServer

void feed();        // Feed the chickens

// Global variables
bool interrupt_flag = false;
unsigned long feed_millis = millis();

// deep sleep
esp_sleep_wakeup_cause_t wakeup_reason;

AsyncWebServer server(80);

ConfigManager configManager(CONFIG_FILE);
AlertManager alertManager(configManager);

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("Starte Hühner-Futterautomat");

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
  attachInterrupt(digitalPinToInterrupt(CLINT), interrupt_handler, FALLING);

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0);

  wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Wakeup caused by external signal using RTC_IO");

    // feed the chickens
    feed();

    // go to sleep
    Serial.println("Schlafmodus aktiviert");
    esp_deep_sleep_start();
  } else {
    // Setup WiFi
    setup_wifi();

    // Setup AsyncWebServer
    setup_aws();
  }
}

void loop() {
  if (interrupt_flag) {
    feed();
  }
}

void IRAM_ATTR interrupt_handler() {
  interrupt_flag = true;
}

void feed() {
  Serial.println("Fütterung gestartet");
  digitalWrite(RELAY_PIN, HIGH);
  delay(configManager.get_feed_config().quantity * FEED_FACTOR);
  digitalWrite(RELAY_PIN, LOW);
  Serial.println("Fütterung beendet");
  interrupt_flag = false;

  // Setup the new alert (if necessary)
  alertManager.set_next_alert();
}

void setup_wifi() {
  Serial.print("Waiting for SSID and password");
  while (configManager.get_wifi_ssid() == "" || configManager.get_wifi_password() == "") {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Done");

  Serial.println("Starte WiFi Access Point");
  WiFi.softAP(configManager.get_wifi_ssid(), configManager.get_wifi_password());
  
  Serial.println();
  Serial.print("Hotspot-SSID: ");
  Serial.println(configManager.get_wifi_ssid());
  Serial.print("Hotspot-IP-Adresse: ");
  Serial.println(WiFi.softAPIP());
  Serial.println();
}

void setup_aws() {
    // Index-Datei auf `/` ausliefern
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, INDEX_FILE, "text/html");
  });

  // CSS-Datei auf `/style.css` ausliefern
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, CSS_FILE, "text/css");
  });

  // JS-Datei auf `/script.js` ausliefern
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, JS_FILE, "text/javascript");
  });

  // Endpunkt zum Abrufen der Timer-Daten
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    // JSON-Daten in einen String umwandeln
    String jsonString;
    serializeJson(configManager.get_timers_json(), jsonString);

    Serial.println("Get configuration");

    request->send(200, "application/json", jsonString);
  });

  // This endpoint is used to set the timers
  AsyncCallbackJsonWebHandler* set_handler = new AsyncCallbackJsonWebHandler("/set", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // Convert the data to a JSON object
    Serial.println("Set new configuration");
    configManager.set_timers_json(json);

    // Setup the new alert (if necessary)
    alertManager.set_next_alert();

    request->send(200);
  });

  server.addHandler(set_handler);

  // activate sleep mode
  server.on("/sleep", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Schlafmodus aktiviert");
    
    // go to sleep
    esp_deep_sleep_start();
    request->send(200);
  });

  // feed manually
  AsyncCallbackJsonWebHandler* feed_handler = new AsyncCallbackJsonWebHandler("/feed", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // if 'on' is true, start feeding
    if (json.containsKey("on") && json["on"].is<bool>()) {
      if (json["on"].as<bool>()) {
        Serial.println("Start feeding");
        digitalWrite(RELAY_PIN, HIGH);
      } else {
        Serial.println("Stop feeding");
        digitalWrite(RELAY_PIN, LOW);
      }
    }

    request->send(200);
  });

  server.addHandler(feed_handler);

  // Webserver starten
  Serial.println("Starte Webserver");
  server.begin();
}