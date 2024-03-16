#include <Arduino.h>
#include <Wire.h>
#include <FS.h>

#include <TaskScheduler.h>

#define MOTO_PIN D8

Scheduler runner;

void feedingTask();

Task tastFeeding(1000, TASK_FOREVER, &feedingTask);

bool feeding[] = {false, false};

void setup() {
  Serial.begin(115200);

  pinMode(MOTO_PIN, OUTPUT);

  runner.init();
  runner.addTask(tastFeeding);
  tastFeeding.enable();
}

void loop() {
  runner.execute();

  if (feeding[0] && !feeding[1]) {
    Serial.println("feeding");

    digitalWrite(MOTO_PIN, HIGH);
    feeding[1] = true;
  } else if (feeding[1] && !feeding[0]) {
    Serial.println("stop feeding");

    digitalWrite(MOTO_PIN, LOW);
    feeding[1] = false;
  }
}

void feedingTask() {
  feeding[0] = !feeding[0];
}
