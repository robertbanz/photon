
#include <Arduino.h>

#include <ArduinoLed.h>
#include <ArduinoSTL.h>
#include <ArduinoSwitch.h>
#include <Photon.h>
#include <RedGreenLed.h>
#include <SerialUtils.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <algorithm>
#include <sstream>
#include <vector>
#include <memory>
#include "pb.h"
#include "pb_encode.h"

constexpr unsigned int kRXD = 0;
constexpr unsigned int kTXD = 1;
constexpr unsigned int kIrSerial = 2;  // should be 2. (11 for mega)
constexpr unsigned int kTrigger = 3;
constexpr unsigned int kIrSerialTxUnused = 4;
constexpr unsigned int kTargetLedRed = 5;
constexpr unsigned int kTargetLedGreen = 6;
constexpr unsigned int kFrontLedRed = 9;
constexpr unsigned int kFrontLedGreen = 10;
constexpr unsigned int kHeartbeatLed = 13;

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
ArduinoLed hearbeat_led(kHeartbeatLed);
ArduinoSwitch trigger(kTrigger);
SoftwareSerial ir_serial(kIrSerial, kIrSerialTxUnused);

photon_PhaserNotification* phaser_notification = nullptr;

void send_pending_notification() {
  if (phaser_notification == nullptr) {
    return;
  }
  uint8_t outbuf[32];
  auto ostream = pb_ostream_from_buffer(outbuf, sizeof(outbuf));
  pb_encode(&ostream, photon_PhaserNotification_fields, phaser_notification);
  Wire.beginTransmission(8);
  Wire.write(outbuf, ostream.bytes_written);
  Wire.endTransmission();
}

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
  hearbeat_led.Flash({0, 255}, {1000});
  console->begin(115200);
  ir_serial.begin(1200);
  ir_serial.listen();
  Wire.begin(photon::kPhaserI2CAddress);
}

PhaserState phaser_state;
std::vector<unsigned char> et_packet;

void loop() {
  auto now = millis();

  // Call Updaters
  trigger.Update();
  targeting_led.Update();
  front_led.Update();
  hearbeat_led.Update();

  if (phaser_notification != nullptr) {
    delete phaser_notification;
  }

  auto irserial_result = GetLastByteFromSerial(&ir_serial);
  if (irserial_result.first) {
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

    if (phaser_state.CheckForMiss(now)) {
      phaser_notification = photon::NewPhaserNotificationMiss();
      console->println("MISS");
    }

    auto trigger_state = trigger.HasChangedState();
    if (trigger_state.first) {
      console->print("TRIGGER ");
      if (trigger_state.second) {
        console->println(" ON");
        phaser_state.SetPull(now);
      } else {
        console->println(" OFF");
      }
    }

    if (phaser_state.CheckForHit(now, irserial_result.second)) {
      phaser_notification = photon::NewPhaserNotificationHit(irserial_result.second);
    } else {
      if (irserial_result.second == photon::kEtBarker) {
        et_packet = {irserial_result.second};

      } else if (et_packet.size() > 0 &&
                 et_packet.size() < photon::kEtPacketSize) {
        et_packet.push_back(irserial_result.second);
        if (et_packet.size() == photon::kEtPacketSize) {
          if (photon::ValidateEtPacket(et_packet)) {
            phaser_notification = photon::NewPhaserNotificationEtPacket(et_packet);
            et_packet.clear();
          }
        }

      } else {
        et_packet.clear();
      }
    }
  }
}
