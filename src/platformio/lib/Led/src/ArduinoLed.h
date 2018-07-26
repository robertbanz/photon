
#include "common/led.h"

class ArduinoLed : public Led {
 public:
  ArduinoLed(unsigned int pin) : pin_(pin) { pinMode(pin_, OUTPUT); }

 private:
  void SetHardwareValue(unsigned char value) override {
    analogWrite(pin_, value);
  }
  unsigned int pin_;
};
