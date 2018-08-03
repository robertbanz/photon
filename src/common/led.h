#ifndef _COMMON_LED_H
#define _COMMON_LED_H

#ifdef ARDUINO
#include <ArduinoSTL.h>
#endif // ARDUINO

#include <stdlib.h>
#include <vector>

class Led {
 public:
  virtual ~Led(){};

  virtual void Set(unsigned char value) {
    is_flashing_ = false;
    InternalSet(value);
    millis_per_step_ = 0.0;
    target_value_ = value;
    SetHardwareValue(value);
  }

  virtual unsigned char Get() const { return value_; }

  virtual void FadeTo(unsigned char value, unsigned long duration_in_millis) {
    is_flashing_ = false;
    auto now = GetMillis();
    last_heartbeat_ = now;
    target_value_ = value;

    if (target_value_ > value_) {
      direction_ = 1;
    } else if (target_value_ < value_) {
      direction_ = -1;
    } else {
      return;
    }

    int distance = abs((int)target_value_ - (int)value_);

    float millis_per_step_float = (float)duration_in_millis / distance;

    if (millis_per_step_float < 100.0) {
      unsigned int power_factor = 100.0 / millis_per_step_float;
      direction_ *= power_factor;
      millis_per_step_float *= power_factor;
    }
    millis_per_step_ = (int)(millis_per_step_float + 0.5);
  }

  virtual void Flash(const std::vector<unsigned char>& values,
                     const std::vector<unsigned long>& durations_in_millis) {
    if (values.size() == 0 || durations_in_millis.size() == 0) {
      // clearly not actually flashing.
      is_flashing_ = false;
      return;
    }
    is_flashing_ = true;
    flash_values_ = values;
    flash_durations_ = durations_in_millis;
    flash_position_ = 0;
    last_heartbeat_ = GetMillis();
    InternalSet(flash_values_[0]);
  }

  virtual void Update() {
    if (is_flashing_) {
      UpdateForFlash();
    } else {
      UpdateForFade();
    }
  }

 protected:
  virtual void SetHardwareValue(unsigned char value) = 0;
  Led() = default;

 private:
  void InternalSet(unsigned char value) {
    SetHardwareValue(value);
    value_ = value;
  }

  void UpdateForFade() {
    if (value_ == target_value_) {
      return;
    }

    auto now = GetMillis();
    auto age = now - last_heartbeat_;

    int delta = (age / millis_per_step_) * direction_;

    if (abs(delta) < 1) {
      return;  // no significant change.
    }

    int new_value = value_ + delta;

    if (direction_ < 0 && new_value < target_value_) {
      new_value = target_value_;
    } else if (direction_ > 0 && new_value > target_value_) {
      new_value = target_value_;
    }

    InternalSet(new_value);
    last_heartbeat_ = now;
  }

  void UpdateForFlash() {
    auto duration = flash_position_ < flash_durations_.size()
        ? flash_durations_[flash_position_]
        : flash_durations_.back();
    auto now = GetMillis();
    if (now - last_heartbeat_ < duration) {
      // nothing to do.
      return;
    }

    // skip ahead.
    flash_position_ = (flash_position_ + 1) % flash_values_.size();
    InternalSet(flash_values_[flash_position_]);
    last_heartbeat_ = now;
  }

  virtual unsigned long GetMillis() = 0;

  unsigned char value_ = 0;
  unsigned char target_value_ = 0;
  int direction_ = 0;
  int millis_per_step_ = 0;
  unsigned long last_heartbeat_ = 0;

  bool is_flashing_ = false;
  std::vector<unsigned char> flash_values_;
  std::vector<unsigned long> flash_durations_;
  unsigned int flash_position_ = 0;
};

#endif  // _COMMON_LED_H
