#include <FeedingService.hpp>

FeedingService::FeedingService() {
  pinMode(TRANSISTOR_PIN, OUTPUT);
  digitalWrite(TRANSISTOR_PIN, LOW);

  Serial.println("[INFO] FeedingService initialized.");
}

void FeedingService::setup() {
  // Initialize position to closed but don't move yet
  // Movement will happen on first feed() call
  position = SERVO_MIN_ANGLE;
  Serial.println("[INFO] FeedingService ready (servos at closed position).");
}

void FeedingService::feed(uint8_t count) {
  if (state != IDLE) {
    Serial.println("[WARN] Feed already in progress, ignoring request.");
    return;
  }

  // Validate count
  if (count < 1) count = 1;
  if (count > 5) count = 5;

  feedCount = count;
  feedsCompleted = 0;

  Serial.printf("[INFO] Starting feed sequence: %d portions\n", feedCount);
  isFeedSequence = true;
  startMovement(SERVO_MAX_ANGLE, true);
}

bool FeedingService::isFeeding() {
  return (state != IDLE) || isFeedSequence;
}

void FeedingService::startMovement(uint8_t target, bool feedSeq) {
  if (state != IDLE) {
    Serial.println("[WARN] Movement already in progress.");
    return;
  }

  targetPosition = target;
  isFeedSequence = feedSeq;
  state = POWER_ON;
  stateStartTime = millis();
}

void FeedingService::open() {
  startMovement(SERVO_MAX_ANGLE);
}

void FeedingService::close() {
  startMovement(SERVO_MIN_ANGLE);
}

void FeedingService::update() {
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - stateStartTime;

  switch (state) {
    case IDLE:
      // Nothing to do
      break;

    case POWER_ON:
      // Turn on transistor and wait for servos to initialize
      digitalWrite(TRANSISTOR_PIN, HIGH);
      state = ATTACH_SERVOS;
      stateStartTime = currentTime;
      Serial.println("[DEBUG] Power ON");
      break;

    case ATTACH_SERVOS:
      if (elapsed >= POWER_ON_DELAY) {
        // Attach servos and immediately write target position to avoid default position jump
        servo1.attach(SERVO1_PIN, 500, 2400);
        servo2.attach(SERVO2_PIN, 500, 2400);

        // Immediately write target position to prevent jump to default
        servo1.write(SERVO_MAX_ANGLE - targetPosition);
        servo2.write(targetPosition);

        position = targetPosition;

        state = SERVO_READY;
        stateStartTime = currentTime;
        Serial.print("[DEBUG] Servos attached and moving to position: ");
        Serial.println(targetPosition);
      }
      break;

    case SERVO_READY:
      if (elapsed >= SERVO_ATTACH_DELAY) {
        // Extra stabilization time - servos should now be at target
        state = MOVING;
        stateStartTime = currentTime;
        Serial.println("[DEBUG] Servos stabilizing");
      }
      break;

    case MOVING:
      if (elapsed >= SERVO_MOVE_TIME) {
        // Movement complete, detach servos
        state = DETACH_SERVOS;
        stateStartTime = currentTime;
        Serial.println("[DEBUG] Movement complete");
      }
      break;

    case DETACH_SERVOS:
      servo1.detach();
      servo2.detach();
      state = POWER_OFF;
      stateStartTime = currentTime;
      Serial.println("[DEBUG] Servos detached");
      break;

    case POWER_OFF:
      digitalWrite(TRANSISTOR_PIN, LOW);

      // Check if this was part of a feed sequence and we just finished opening
      if (isFeedSequence && targetPosition == SERVO_MAX_ANGLE) {
        // We just finished opening, now wait before closing
        state = FEED_WAITING;
        stateStartTime = currentTime;
        Serial.println("[DEBUG] Power OFF, waiting before close");
      } else if (isFeedSequence && targetPosition == SERVO_MIN_ANGLE) {
        // We just finished closing, check if we need more feedings
        feedsCompleted++;
        Serial.printf("[DEBUG] Completed feeding %d/%d\n", feedsCompleted, feedCount);

        if (feedsCompleted < feedCount) {
          // Start next feeding cycle
          Serial.println("[DEBUG] Starting next portion");
          state = FEED_WAITING;
          stateStartTime = currentTime;
        } else {
          // All feedings complete
          state = IDLE;
          isFeedSequence = false;
          Serial.println("[DEBUG] All portions complete");
        }
      } else {
        // Normal movement complete
        state = IDLE;
        isFeedSequence = false;
        Serial.println("[DEBUG] Power OFF, sequence complete");
      }
      break;

    case FEED_WAITING:
      if (elapsed >= FEED_WAIT_TIME) {
        // Wait time over
        if (targetPosition == SERVO_MAX_ANGLE) {
          // We were open, now close
          Serial.println("[DEBUG] Wait complete, closing");
          targetPosition = SERVO_MIN_ANGLE;
        } else {
          // We were closed, now open for next portion
          Serial.println("[DEBUG] Wait complete, opening for next portion");
          targetPosition = SERVO_MAX_ANGLE;
        }
        state = POWER_ON;
        stateStartTime = currentTime;
      }
      break;
  }
}

uint8_t FeedingService::getPosition() {
  return position;
}
