#ifndef FEEDING_SERVICE_HPP
#define FEEDING_SERVICE_HPP

#include <ESP32Servo.h>

#include "ButtonService.hpp"
#include "ClockService.hpp"

// Forward declarations
class ConfigService;

#define MAX_FEED_HISTORY 10

struct FeedHistoryEntry {
  uint32_t timestamp;  // Unix timestamp
  uint8_t portion_units;  // Number of portion units fed
};

#ifndef SERVO1_PIN
#define SERVO1_PIN 21
#endif
#ifndef SERVO2_PIN
#define SERVO2_PIN 2
#endif
#ifndef TRANSISTOR_PIN
#define TRANSISTOR_PIN 5
#endif

#define SERVO_MIN_ANGLE 0
#define SERVO_MAX_ANGLE 180

// Timing constants (in milliseconds)
#define POWER_ON_DELAY 100     // Time to wait after powering on servos
#define SERVO_ATTACH_DELAY 100 // Time to wait after attaching servos before sending position
#define SERVO_MOVE_TIME 620    // Time for servos to complete movement
#define FEED_WAIT_TIME 1000    // Time to wait between open and close during feed

enum ServoState {
  IDLE,
  POWER_ON,
  ATTACH_SERVOS,
  SERVO_READY,   // Wait for servos to be ready after attach
  MOVING,
  DETACH_SERVOS,
  POWER_OFF,
  FEED_WAITING   // Waiting between open and close during feed sequence
};

class FeedingService {
public:
  FeedingService();
  void setup();
  void feed(uint8_t count = 1);  // Feed with count portions (1-5)
  void update();  // Must be called in loop()

  uint8_t getPosition();
  bool isFeeding();  // Check if currently in a feed sequence
  uint32_t getLastFeedTimestamp() const { return lastFeedUnix; }
  void recordFeedEvent(); // Manually record feed completion (e.g., if needed)
  void setClockService(ClockService* clock) { clockService = clock; }
  void setConfigService(ConfigService* config) { configService = config; }

  // Feed history management
  void addFeedToHistory(uint32_t timestamp, uint8_t portionUnits);
  uint8_t getFeedHistoryCount() const;
  const FeedHistoryEntry* getFeedHistory() const { return feedHistory; }
  void loadFeedHistory(const FeedHistoryEntry* history, uint8_t count);
  void clearFeedHistory();

private:
  uint8_t position = 0;
  uint8_t targetPosition = 0;
  bool isFeedSequence = false;  // Track if we're in a feed sequence
  uint8_t feedCount = 0;         // Remaining feedings in sequence
  uint8_t feedsCompleted = 0;    // Completed feedings in sequence

  Servo servo1 = Servo();
  Servo servo2 = Servo();

  ServoState state = IDLE;
  unsigned long stateStartTime = 0;
  uint32_t lastFeedUnix = 0;
  ClockService* clockService = nullptr;
  ConfigService* configService = nullptr;

  // Feed history (ring buffer)
  FeedHistoryEntry feedHistory[MAX_FEED_HISTORY];
  uint8_t feedHistoryCount = 0;  // Number of valid entries (0-10)
  uint8_t feedHistoryIndex = 0;  // Next write index (circular)

  void startMovement(uint8_t target, bool feedSeq = false);
  void open();
  void close();
};
#endif // FEEDING_SERVICE_HPP
