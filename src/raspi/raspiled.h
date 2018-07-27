
#include "common/led.h"
#include "pigpio.h"

class RaspiLed : public Led {
 public:
  RaspiLed(unsigned int pin) : pin_(pin) {
    gpioSetMode(pin_, PI_OUTPUT);
  }

 private:
  void SetHardwareValue(unsigned char value) override {
    if (value == 0) {
      gpioWrite(pin_, PI_OFF);
    } else {
      gpioWrite(pin_, PI_ON);
    }
  }
  unsigned int pin_;
};
