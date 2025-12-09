#ifndef FEEDING_SERVICE_HPP
#define FEEDING_SERVICE_HPP

#include <ESP32Servo.h>

#include "ButtonService.hpp"

#ifndef SERVO1_PIN
#define SERVO1_PIN 3
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
  void feed();
  void update();  // Must be called in loop()

  uint8_t getPosition();

private:
  uint8_t position = 0;
  uint8_t targetPosition = 0;
  bool isFeedSequence = false;  // Track if we're in a feed sequence

  Servo servo1 = Servo();
  Servo servo2 = Servo();

  ServoState state = IDLE;
  unsigned long stateStartTime = 0;

  void startMovement(uint8_t target, bool feedSeq = false);
  void open();
  void close();
};
#endif // FEEDING_SERVICE_HPP
