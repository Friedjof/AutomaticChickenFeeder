#ifndef BUTTON_SERVICE_HPP
#define BUTTON_SERVICE_HPP

#include <Arduino.h>
#include <Button2.h>
#include "PinConfig.h"

class ButtonService {
private:
  Button2 button;

  void (*simpleClickHandler)(Button2& btn) = nullptr;
  void (*doubleClickHandler)(Button2& btn) = nullptr;
  void (*longClickHandler)(Button2& btn) = nullptr;

public:
  ButtonService();
  void begin();
  void setSimpleClickHandler(void (*handler)(Button2& btn));
  void setDoubleClickHandler(void (*handler)(Button2& btn));
  void setLongClickHandler(void (*handler)(Button2& btn));
  void loop();
};
#endif // BUTTON_SERVICE_HPP
