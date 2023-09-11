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

#define INDEX_FILE "/index.html"
#define CSS_FILE "/style.css"
#define JS_FILE "/script.js"

void load_config();
void save_config();
void print_config();

AsyncWebServer server(80);

ConfigManager configManager(CONFIG_FILE);
AlertManager alertManager(configManager);

void setup()
{
  // IP-Adresse des ESP32 im Hotspot-Modus ausgeben
  Serial.begin(115200);
  while (!Serial) { delay(100); }

  delay(3000);

  Serial.println("Starte Hühner-Futterautomat");

  // Setup of the alert manager
  alertManager.setup();

  // print next alert
  optional_ds3231_timer_t next_alert = alertManager.get_next_alert();
  if (!next_alert.empty) {
    alertManager.print_timer(next_alert.timer);
  } else {
    Serial.println("Kein Timer gefunden");
  }

  // Solange die SSID und das Passwort leer sind, warte
  Serial.println("Warte auf SSID und Passwort");
  while (configManager.get_wifi_ssid() == "" || configManager.get_wifi_password() == "") {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Done");

  // Verbindung zum eigenen Hotspot herstellen
  Serial.println("Erstelle eigenen Hotspot");
  WiFi.softAP(configManager.get_wifi_ssid(), configManager.get_wifi_password());
  
  Serial.println();
  Serial.print("Hotspot-SSID: ");
  Serial.println(configManager.get_wifi_ssid());
  Serial.print("Hotspot-IP-Adresse: ");
  Serial.println(WiFi.softAPIP());
  Serial.println();

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

  // Endpunkt zum Setzen der Timer-Daten
  AsyncCallbackJsonWebHandler* set_handler = new AsyncCallbackJsonWebHandler("/set", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // Convert the data to a JSON object
    Serial.println("Set new configuration");
    configManager.set_timers_json(json);

    // Setup the new alert (if necessary)
    alertManager.set_next_alert();
    alertManager.setup_interrupt();

    request->send(200);
  });

  server.addHandler(set_handler);

  // Schlafmodus aktivieren
  server.on("/sleep", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Schlafmodus aktiviert");
    request->send(200);
  });

  // Manuelles Auslösen des Fütterns
  AsyncCallbackJsonWebHandler* feed_handler = new AsyncCallbackJsonWebHandler("/feed", [](AsyncWebServerRequest *request, JsonVariant &json) {
    // Wenn 'on' in den Daten enthalten ist, füttern
    if (json.containsKey("on") && json["on"].is<bool>()) {
      if (json["on"].as<bool>()) {
        Serial.println("Fütterung gestartet");
        digitalWrite(RELAY_PIN, HIGH);
      } else {
        Serial.println("Fütterung beendet");
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

void loop() { }
