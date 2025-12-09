#include "ButtonService.hpp"

ButtonService::ButtonService() {
  this->simpleClickHandler = nullptr;
  this->doubleClickHandler = nullptr;
  this->longClickHandler = nullptr;
}

void ButtonService::begin() {
  // Initialize button with INPUT_PULLUP mode, active low
  this->button.begin(BUTTON_PIN, INPUT_PULLUP, true);
}

void ButtonService::setSimpleClickHandler(void (*handler)(Button2& btn)) {
  if (handler) {
    this->simpleClickHandler = handler;
    this->button.setClickHandler(simpleClickHandler);
  }
}

void ButtonService::setDoubleClickHandler(void (*handler)(Button2& btn)) {
  if (handler) {
    this->doubleClickHandler = handler;
    this->button.setDoubleClickHandler(doubleClickHandler);
  }
}

void ButtonService::setLongClickHandler(void (*handler)(Button2& btn)) {
  if (handler) {
    this->longClickHandler = handler;
    this->button.setLongClickHandler(longClickHandler);
  }
}

void ButtonService::loop() {
  this->button.loop();
}
