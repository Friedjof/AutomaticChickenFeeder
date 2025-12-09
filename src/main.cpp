#include <Arduino.h>

#include "FeedingService.hpp"
#include "ButtonService.hpp"
#include "ClockService.hpp"
#include "ConfigService.hpp"
#include "WebService.hpp"
#include "SchedulingService.hpp"

#define SERVO2_PIN 2
#define SERVO1_PIN 3
#define TRANSISTOR_PIN 5
#define BUTTON_PIN 4

void simpleClickHandler(Button2 &btn);
void doubleClickHandler(Button2 &btn);

// Services
ButtonService buttonService;
FeedingService feedingService;
ClockService clockService;
ConfigService configService;
SchedulingService schedulingService(configService, clockService, feedingService);
WebService webService(configService, clockService, feedingService, schedulingService);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n[INFO] ========================================");
  Serial.println("[INFO] Automatic Chicken Feeder v2.0");
  Serial.println("[INFO] ========================================\n");

  // Initialize config service
  if (!configService.begin()) {
    Serial.println("[ERROR] Failed to initialize ConfigService!");
  }

  // Initialize clock service
  if (!clockService.begin()) {
    Serial.println("[WARN] DS3231 RTC not available - time sync required");
  }

  // Initialize button service
  buttonService.begin();
  buttonService.setSimpleClickHandler(simpleClickHandler);
  buttonService.setDoubleClickHandler(doubleClickHandler);

  // Initialize feeding service
  feedingService.setup();

  // Initialize scheduling service
  schedulingService.begin();

  // Start web server
  if (!webService.begin()) {
    Serial.println("[ERROR] Failed to start WebService!");
  }

  Serial.println("\n[INFO] Setup complete.");
  Serial.println("[INFO] Press button once to feed manually");
  Serial.println("[INFO] Press button twice to start AP mode");
}

void loop() {
  buttonService.loop();
  feedingService.update();
  webService.update();
  schedulingService.update();
}

void simpleClickHandler(Button2 &btn) {
  Serial.println("[BUTTON] Single click - Manual feed");
  feedingService.feed(1);
}

void doubleClickHandler(Button2 &btn) {
  Serial.println("[BUTTON] Double click - Starting AP mode");
  webService.startAP("ChickenFeeder", "");
}
