#include "VibrationService.hpp"

VibrationService::VibrationService() {
  pinMode(VIBRATION_PIN, OUTPUT);
  digitalWrite(VIBRATION_PIN, LOW);

  Serial.println("[INFO] VibrationService initialized.");
}

void VibrationService::setPin(bool on) {
  pinOn = on;
  digitalWrite(VIBRATION_PIN, on ? HIGH : LOW);
}

void VibrationService::update() {
  if (pinOn && !feedShakeForced && millis() >= offAtMillis) {
    setPin(false);
  }
}

void VibrationService::startFeedShake() {
  feedShakeForced = true;
  setPin(true);
  Serial.println("[VIBRATION] Feed shake started");
}

void VibrationService::endFeedShake(uint32_t tailMs) {
  feedShakeForced = false;

  if (tailMs == 0) {
    setPin(false);
    Serial.println("[VIBRATION] Feed shake stopped");
    return;
  }

  offAtMillis = millis() + tailMs;
  Serial.printf("[VIBRATION] Feed shake tail: %lu ms\n", (unsigned long)tailMs);
}

void VibrationService::triggerPulse(uint32_t durationMs) {
  if (feedShakeForced) {
    // Feed shake already covers vibration - avoid conflicting timers
    return;
  }

  setPin(true);
  offAtMillis = millis() + durationMs;
  Serial.printf("[VIBRATION] Pulse triggered: %lu ms\n", (unsigned long)durationMs);
}
