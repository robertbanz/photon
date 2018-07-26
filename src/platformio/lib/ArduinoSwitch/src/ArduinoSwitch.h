#ifndef _ARDUINO_SWITCH_H
#define _ARDUINO_SWITCH_H

#include <Arduino.h>
#include <StandardCplusplus.h>
#include <utility.h>

class ArduinoSwitch {
 public:
  ArduinoSwitch(unsigned int pin, unsigned long debouncemillis = 50)
      : pin_(pin), debounce_millis_(debouncemillis) {
    pinMode(pin_, INPUT_PULLUP);
    state_ = digitalRead(pin_);
  }
  bool State() const {
    if (state_ == HIGH) {
      return true;
    } else {
      return false;
    }
  }
  void Update() {
    auto now = millis();
    if (now - debounce_last_change_ < debounce_millis_) {
      return;
    }
    auto new_state = digitalRead(pin_);
    if (new_state != state_) {
      has_changed_ = true;
      state_ = new_state;
      debounce_last_change_ = now;
    }
  }
  std::pair<bool, bool> HasChangedState() {
    std::pair<bool, bool> result(has_changed_, State());
    has_changed_ = false;
    return result;
  }

 private:
  int state_;
  bool has_changed_ = true;
  const unsigned int pin_;
  const unsigned long debounce_millis_;
  unsigned long debounce_last_change_ = 0;
};

#endif  // _ARDUINO_SWITCH_H
