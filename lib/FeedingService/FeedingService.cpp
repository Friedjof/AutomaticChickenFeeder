#include <FeedingService.hpp>
#include <ConfigService.hpp>

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
        // Attach servos
        servo1.attach(SERVO1_PIN, 500, 2400);
        servo2.attach(SERVO2_PIN, 500, 2400);

        // Start at current position (will move stepwise to target)
        currentStepPosition = position;
        servo1.write(SERVO_MAX_ANGLE - currentStepPosition);
        servo2.write(currentStepPosition);

        state = SERVO_READY;
        stateStartTime = currentTime;
        Serial.printf("[DEBUG] Servos attached at position %d, will move to %d in steps\n",
                      currentStepPosition, targetPosition);
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
      if (elapsed >= STEP_DELAY) {
        // Calculate next step
        if (currentStepPosition != targetPosition) {
          // Move one step towards target
          if (currentStepPosition < targetPosition) {
            // Moving up (opening)
            if (currentStepPosition + STEP_SIZE >= targetPosition) {
              currentStepPosition = targetPosition;
            } else {
              currentStepPosition += STEP_SIZE;
            }
          } else {
            // Moving down (closing)
            if (currentStepPosition <= targetPosition + STEP_SIZE) {
              currentStepPosition = targetPosition;
            } else {
              currentStepPosition -= STEP_SIZE;
            }
          }

          // CRITICAL: Write both servos at EXACTLY the same time for synchronous movement
          // servo1 rotates opposite direction, servo2 normal direction
          servo1.write(SERVO_MAX_ANGLE - currentStepPosition);
          servo2.write(currentStepPosition);

          // Reset timer for next step
          stateStartTime = currentTime;

          Serial.printf("[DEBUG] Moving step: %d -> target: %d\n", currentStepPosition, targetPosition);
        } else {
          // Target reached, move to detach
          position = targetPosition;
          state = DETACH_SERVOS;
          stateStartTime = currentTime;
          Serial.println("[DEBUG] Movement complete");
        }
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

      // DEBUG: Log state before checking conditions
      Serial.printf("[DEBUG] POWER_OFF: isFeedSequence=%d, targetPosition=%d, feedsCompleted=%d/%d, time=%lu\n",
                    isFeedSequence, targetPosition, feedsCompleted, feedCount, millis());

      // Check if this was part of a feed sequence and we just finished opening
      if (isFeedSequence && targetPosition == SERVO_MAX_ANGLE) {
        // We just finished opening, now wait before close
        state = FEED_WAITING;
        stateStartTime = currentTime;
        Serial.printf("[DEBUG] Power OFF, waiting before close (start_time=%lu)\n", currentTime);
      } else if (isFeedSequence && targetPosition == SERVO_MIN_ANGLE) {
        // We just finished closing, check if we need more feedings
        feedsCompleted++;
        Serial.printf("[DEBUG] Completed feeding %d/%d\n", feedsCompleted, feedCount);

        if (feedsCompleted < feedCount) {
          // Start next feeding cycle
          Serial.printf("[DEBUG] Starting next portion (start_time=%lu)\n", currentTime);
          state = FEED_WAITING;
          stateStartTime = currentTime;
        } else {
          // All feedings complete
          state = IDLE;
          isFeedSequence = false;
          recordFeedEvent();
          Serial.println("[DEBUG] All portions complete");
        }
      } else {
        // Normal movement complete
        state = IDLE;
        isFeedSequence = false;
        recordFeedEvent();
        Serial.println("[DEBUG] Power OFF, sequence complete");
      }
      break;

    case FEED_WAITING:
      if (elapsed >= FEED_WAIT_TIME) {
        // Wait time over
        if (targetPosition == SERVO_MAX_ANGLE) {
          // We were open, now close
          Serial.printf("[DEBUG] Wait complete (%lu ms), closing\n", elapsed);
          targetPosition = SERVO_MIN_ANGLE;
        } else {
          // We were closed, now open for next portion
          Serial.printf("[DEBUG] Wait complete (%lu ms), opening for next portion\n", elapsed);
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

void FeedingService::recordFeedEvent() {
  if (clockService) {
    DateTime now = clockService->now();
    if (!now.isValid()) {
      Serial.println("[WARN] ClockService returned invalid time; last feed not stored");
      lastFeedUnix = 0;
      return;
    }
    lastFeedUnix = now.unixtime();
    Serial.printf("[INFO] Feed event recorded at %04u-%02u-%02u %02u:%02u:%02u\n",
                  now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

    // Add to feed history
    addFeedToHistory(lastFeedUnix, feedCount);
  } else {
    lastFeedUnix = 0;
    Serial.println("[WARN] ClockService unavailable, last feed time not recorded");
  }
}

void FeedingService::addFeedToHistory(uint32_t timestamp, uint8_t portionUnits) {
  Serial.printf("[DEBUG] addFeedToHistory called: timestamp=%lu, portionUnits=%d, current index=%d\n",
                timestamp, portionUnits, feedHistoryIndex);

  // Add entry to ring buffer
  feedHistory[feedHistoryIndex].timestamp = timestamp;
  feedHistory[feedHistoryIndex].portion_units = portionUnits;

  Serial.printf("[DEBUG] Stored at index %d: timestamp=%lu, portion_units=%d\n",
                feedHistoryIndex, feedHistory[feedHistoryIndex].timestamp,
                feedHistory[feedHistoryIndex].portion_units);

  // Move to next index (circular)
  feedHistoryIndex = (feedHistoryIndex + 1) % MAX_FEED_HISTORY;

  // Update count (max 10)
  if (feedHistoryCount < MAX_FEED_HISTORY) {
    feedHistoryCount++;
  }

  Serial.printf("[DEBUG] Feed added to history: %lu, %d units (count: %d, next index: %d)\n",
                timestamp, portionUnits, feedHistoryCount, feedHistoryIndex);

  // Immediately save to persistent storage
  if (configService) {
    configService->saveFeedHistory(feedHistory, feedHistoryCount);
    Serial.println("[INFO] Feed history saved to NVS");
  }
}

uint8_t FeedingService::getFeedHistoryCount() const {
  return feedHistoryCount;
}

void FeedingService::loadFeedHistory(const FeedHistoryEntry* history, uint8_t count) {
  if (count > MAX_FEED_HISTORY) {
    count = MAX_FEED_HISTORY;
  }

  Serial.printf("[DEBUG] loadFeedHistory: loading %d entries\n", count);

  for (uint8_t i = 0; i < count; i++) {
    feedHistory[i] = history[i];
    Serial.printf("[DEBUG] Loaded entry %d: timestamp=%lu, portion_units=%d\n",
                  i, feedHistory[i].timestamp, feedHistory[i].portion_units);
  }

  feedHistoryCount = count;
  feedHistoryIndex = count % MAX_FEED_HISTORY;

  Serial.printf("[INFO] Loaded %d feed history entries, next index will be %d\n", count, feedHistoryIndex);
}

void FeedingService::clearFeedHistory() {
  feedHistoryCount = 0;
  feedHistoryIndex = 0;
  memset(feedHistory, 0, sizeof(feedHistory));
  Serial.println("[INFO] Feed history cleared");
}
