
#include "common/led.h"
#include <digitalWriteFast.h>

class ArduinoLed : public Led {
 public:
  ArduinoLed(unsigned int pin) : pin_(pin) { pinMode(pin_, OUTPUT); }

 private:
  void SetHardwareValue(unsigned char value) override {
    if (value == 0) {
      digitalWriteFast(pin_, LOW);
    } else if (value == 255) {
      digitalWriteFast(pin_, HIGH);
    } else {
      analogWrite(pin_, value);
    }
  }
  unsigned long GetMillis() override {
    return millis();
  }
  unsigned int pin_;
};
