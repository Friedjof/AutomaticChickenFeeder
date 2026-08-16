#ifndef VIBRATION_SERVICE_HPP
#define VIBRATION_SERVICE_HPP

#include <Arduino.h>

#ifndef VIBRATION_PIN
#define VIBRATION_PIN 10
#endif

class VibrationService {
public:
  VibrationService();
  void update();  // Must be called in loop()

  // Continuous shake driven by a feed cycle (start/during/after)
  void startFeedShake();
  void endFeedShake(uint32_t tailMs);

  // Single timed pulse (manual trigger / idle shake schedule)
  void triggerPulse(uint32_t durationMs);

  bool isActive() const { return pinOn; }

private:
  bool pinOn = false;
  bool feedShakeForced = false;
  unsigned long offAtMillis = 0;

  void setPin(bool on);
};

#endif // VIBRATION_SERVICE_HPP
