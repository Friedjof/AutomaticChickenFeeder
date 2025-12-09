#include <Arduino.h>

#include "FeedingService.hpp"
#include "ButtonService.hpp"

#define SERVO2_PIN 2
#define SERVO1_PIN 3
#define TRANSISTOR_PIN 5

#define BUTTON_PIN 4

void simpleClickHandler(Button2 &btn);
//void doubleClickHandler(Button2 &btn);
//void longClickHandler(Button2 &btn);

ButtonService buttonService;
FeedingService feedingService;

void setup() {
  Serial.begin(115200);

  // Initialize button service (configures pin with INPUT_PULLUP)
  buttonService.begin();

  buttonService.setSimpleClickHandler(simpleClickHandler);
  //buttonService.setDoubleClickHandler(doubleClickHandler);
  //buttonService.setLongClickHandler(longClickHandler);

  feedingService.setup();

  Serial.println("[INFO] Setup complete.");
}

void loop() {
  buttonService.loop();
  feedingService.update();
}

void simpleClickHandler(Button2 &btn) {
  Serial.println("Single Click detected!");
  feedingService.feed();
}
