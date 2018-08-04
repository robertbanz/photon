#ifndef _COMMON_REDGREENLED_H
#define _COMMON_REDGREENLED_H

#include "common/led.h"

class RedGreenLed : public Led {
 public:
  // Takes ownership of red & green.
  RedGreenLed(Led* red, Led* green) : red_led_(red), green_led_(green) { };
  ~RedGreenLed() {
    delete red_led_;
    delete green_led_;
  }

  Led& Red() { return *red_led_; }
  Led& Green() { return *green_led_; }

  void Set(unsigned char red, unsigned char green) {
    red_led_->Set(red);
    green_led_->Set(green);
  }
  void Set(unsigned char value) override { Set(value, value); }

  void FadeTo(unsigned char red, unsigned char green,
              unsigned long duration_in_millis) {
    red_led_->FadeTo(red, duration_in_millis);
    green_led_->FadeTo(green, duration_in_millis);
  }

  void FadeTo(unsigned char value, unsigned long duration_in_millis) override {
    FadeTo(value, value, duration_in_millis);
  }

  void Update() override {
    red_led_->Update();
    green_led_->Update();
  }

  unsigned char Get() const override {
    return (red_led_->Get() + green_led_->Get()) / 2;
  }

 private:
  void SetHardwareValue(unsigned char) override {
    // unneeded.
  }
  unsigned long GetMillis() override {
    // unused.
    return 0;
  }

  Led* red_led_;
  Led* green_led_;
};

#endif // _COMMON_REDGREENLED_H
