
#include <Arduino.h>

#include <ArduinoLed.h>
#include <ArduinoSwitch.h>
#include <Photon.h>
#include <RedGreenLed.h>
#include <SoftwareSerial.h>
#include <StandardCplusplus.h>
#include <SerialUtils.h>
#include <vector>

constexpr unsigned int kRXD = 0;
constexpr unsigned int kTXD = 1;
constexpr unsigned int kIrSerial = 11;  // should be 2.
constexpr unsigned int kTrigger = 3;
constexpr unsigned int kIrSerialTxUnused = 4;
constexpr unsigned int kTargetLedRed = 5;
constexpr unsigned int kTargetLedGreen = 6;
constexpr unsigned int kFrontLedRed = 9;
constexpr unsigned int kFrontLedGreen = 10;

constexpr HardwareSerial* console = &Serial;

// Millisconds in which we need to receive 3 of the same IR codes in a
// row to be considered a hit.
// ()( 6 * 10 ) / 1200) * 1000
constexpr unsigned int kTriggerWindow = 50;
constexpr unsigned int kTriggerConsecutiveCodes = 3;

RedGreenLed targeting_led(new ArduinoLed(kTargetLedRed),
                          new ArduinoLed(kTargetLedGreen));
RedGreenLed front_led(new ArduinoLed(kFrontLedRed),
                      new ArduinoLed(kFrontLedGreen));
ArduinoSwitch trigger(kTrigger);
SoftwareSerial ir_serial(kIrSerial, kIrSerialTxUnused);

class PhaserState {
 public:
  void SetPull(unsigned long now) {
    active_ = true;
    trigger_pulled_ = now;
  }

  bool CheckForMiss(unsigned long now) {
    if (active_) {
      Heartbeat(now);
      return !active_;
    }
    return false;
  }

  bool CheckForHit(unsigned long now, byte target) {
    if (!active_) {
      return false;
    }
    if (last_byte_ == target) {
      same_target_count_++;
    } else {
      same_target_count_ = 0;
      last_byte_ = target;
    }
    if (same_target_count_ >= kTriggerConsecutiveCodes) {
      Reset();
      return true;
    }
    return false;
  }

 private:
  void Reset() {
    last_byte_ = 0;
    same_target_count_ = 0;
    trigger_pulled_ = 0;
    active_ = false;
  }
  void Heartbeat(unsigned long now) {
    if (active_) {
      if (now - trigger_pulled_ > kTriggerWindow) {
        Reset();
      }
    }
  }
  bool active_ = false;
  byte last_byte_ = 0;
  unsigned int same_target_count_ = 0;
  unsigned long trigger_pulled_ = 0;
};

void setup() {
  console->begin(115200);
  ir_serial.begin(1200);
  ir_serial.listen();
}

void loop() {
  PhaserState phaser_state;
  std::vector<unsigned char> et_packet;

  while (1) {
    auto now = millis();

    // Call Updaters
    trigger.Update();
    targeting_led.Update();
    front_led.Update();

    if (phaser_state.CheckForMiss(now)) {
      console->println("PHASER MISS");
    }

    auto trigger_state = trigger.HasChangedState();
    if (trigger_state.first) {
      console->print("PHASER TRIGGER ");
      if (trigger_state.second) {
        console->println(" ON");
        phaser_state.SetPull(now);
      } else {
        console->println(" OFF");
      }
    }

    auto irserial_result = GetLastByteFromSerial(&ir_serial);
    if (irserial_result.first) {
      // console->print("PHASER IR ");
      // console->println(irserial_result.second);

      Led* target = nullptr;
      switch (photon::TargetTypeFromIrCode(irserial_result.second)) {
        case photon::TargetType::RED_POD:
          target = &targeting_led.Red();
          break;
        case photon::TargetType::GREEN_POD:
          target = &targeting_led.Green();
          break;
        case photon::TargetType::OTHER:
          target = &targeting_led;
          break;
        case photon::TargetType::INVALID:
          target = nullptr;
          break;
      }

      if (target != nullptr && target->Get() == 0) {
        target->Set(255);
        target->FadeTo(0, 25);
      }
      if (front_led.Get() == 0) {
        front_led.Set(255);
        front_led.FadeTo(0, 25);
      }

      if (phaser_state.CheckForHit(now, irserial_result.second)) {
        console->print("PHASER HIT ");
        console->println(irserial_result.second);
      } else {
        if (irserial_result.second == photon::kEtBarker) {
          et_packet = {irserial_result.second};

        } else if (et_packet.size() > 0 &&
                   et_packet.size() < photon::kEtPacketSize) {
          et_packet.push_back(irserial_result.second);
          if (et_packet.size() == photon::kEtPacketSize) {
            if (photon::ValidateEtPacket(et_packet)) {
              console->print("PHASER ET ");
              for (const auto& data : et_packet) {
                console->print(data);
                console->print(" ");
              }
              console->println();
              et_packet.clear();
            }
          }

        } else {
          et_packet.clear();
        }
      }
    }
  }
}
