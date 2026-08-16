#pragma once

#define RTC_INT_PIN 3
#define BUTTON_PIN 4
#define SERVO1_PIN 21
#define SERVO2_PIN 2
#define TRANSISTOR_PIN 5
#define VIBRATION_PIN 10

// Set to 0 (e.g. via `-D VIBRATION_MOTOR_ENABLED=0` in platformio.ini) on
// hardware builds that don't have a vibration motor wired up - VibrationService
// then never touches VIBRATION_PIN and the web UI hides the related controls.
#ifndef VIBRATION_MOTOR_ENABLED
#define VIBRATION_MOTOR_ENABLED 1
#endif
