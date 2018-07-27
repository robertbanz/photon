// Copyright (c) 2016, Robert Banz
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//    * Neither the name of Robert Banz nor the
//      names of its contributors may be used to endorse or promote products
//      derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ROBERT BANZ BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "cr_driver.h"
#include "Photon.h"

namespace photon {

static volatile unsigned int slot_counter = 0;

static void DoSlotInterrupt() {
  slot_counter++;
}

CrDriver::CrDriver(
  const CrDriverOptions& options, GcInterface* interface) :
    radio_serial_(options.radio_serial),
    debug_serial_(options.debug_serial),
    timer_enable_(new Signal(options.timer_enable_output_pin)),
    not_key_(new Signal(options.key_output_pin)),
    esync_(new Signal(options.esync_output_pin)),
    psync_(new Signal(options.psync_output_pin)),
    gsync_(new Signal(options.gsync_output_pin)),
    rf_slot_id_(0),
    transmit_queue_(options.debug_serial, 5, 15),
    current_sync_(GetSyncByteFromName(kPsyncName)),
    sequence_num_(0),
    gc_interface_(interface),
    options_(options),
    ir_position_(0) {

  // CR runs at 1200,n,8,1
  radio_serial_->begin(1200);

  pinMode(options_.local_mode_input_pin, INPUT_PULLUP);
  pinMode(options_.gsync_input_pin, INPUT_PULLUP);
  pinMode(options_.game_input_pin, INPUT_PULLUP);
  pinMode(options_.slave_input_pin, INPUT_PULLUP);
  pinMode(options_.slot_clock_interrupt_pin, INPUT);
  pinMode(options_.fake_mode_pin, INPUT_PULLUP);

  timer_enable_->Off();

  attachInterrupt(digitalPinToInterrupt(options_.slot_clock_interrupt_pin),
                  DoSlotInterrupt, RISING);

  not_key_->On();
  esync_->Off();
  psync_->Off();
  gsync_->Off();

  timer_enable_->On();

  if (digitalRead(options_.slave_input_pin)) {
    EnterSlaveMode();
  } else {
    EnterMasterMode();
  }

  if (digitalRead(options_.fake_mode_pin)) {
    fake_mode_ = true;
  } else {
    fake_mode_ = false;
  }

  if (digitalRead(options_.local_mode_input_pin)) {
    local_mode_ = true;
  } else {
    local_mode_ = false;
  }

  options_.ir_serial->begin(1200);
}

static inline unsigned int TimeDifference(
    unsigned long last, unsigned long now) {
  if (last < now) {
    return (now - last);
  } else if (last > now) {
    return ((0xffffffff - last) + now);
  } else {
    return 0;
  }
}

void CrDriver::Loop() {
  static unsigned int old_slot_counter = 0;
  static unsigned long old_micros = micros();

  unsigned long new_micros = micros();

  int diff = slot_counter - old_slot_counter;

  if (TimeDifference(old_micros, new_micros) > 100000) {
    old_micros = new_micros;
    CheckModeChanges();
  }

  if (slave_mode_ && searching_) {
    SlaveModeLoop();
    return;
  }

  if (diff == 0) {
    // Nope.
    return;
  }

  old_slot_counter = slot_counter;

  // Do stuff.
  SlotInterrupt(diff);
}

void CrDriver::CheckModeChanges() {
  // Check inputs for mode changes.
  if (digitalRead(options_.slave_input_pin) && !slave_mode_) {
    EnterSlaveMode();
  } else if (!digitalRead(options_.slave_input_pin) && slave_mode_) {
    // transition out of slave mode.
    EnterMasterMode();
  }
  if (digitalRead(options_.fake_mode_pin) && !fake_mode_) {
    debug_serial_->println("999-Enabling fake mode.");
    faker_.Reset();
    fake_mode_ = true;
  } else if (!digitalRead(options_.fake_mode_pin) && fake_mode_) {
    debug_serial_->println("999-Disable fake mode.");
    fake_mode_ = false;
  }

  if (digitalRead(options_.local_mode_input_pin) && !local_mode_) {
    debug_serial_->println("999-Enabling local mode.");
    local_mode_ = true;
  } else if (!digitalRead(options_.local_mode_input_pin) && local_mode_) {
    debug_serial_->println("999-Enabling remote mode.");
    local_mode_ = false;
  }
}

void CrDriver::SlotInterrupt(int diff) {
  if (diff != 1) {
    debug_serial_->print("999-skipped ");
    debug_serial_->println(diff);
  }
  rf_slot_id_ += diff;
  if (rf_slot_id_ >= kNumSlots) {
    rf_slot_id_ %= kNumSlots;
    sequence_num_++;
  }
  DoSlot();
  DoIr();
}

void CrDriver::DoSlot() {
  if (rf_slot_id_ == 0) {
    esync_->On();
  } else if (rf_slot_id_ == 1) {
    esync_->Off();
  }

  DoTx(rf_slot_id_);
  DoRx(rf_slot_id_);
  DoKeyUpDown(rf_slot_id_);
}

void CrDriver::DoIr() {
  if (ir_out_data_.empty()) {
    return;
  }
  options_.ir_serial->write(ir_out_data_[ir_position_]);
  ir_position_ = (ir_position_ + 1) % ir_out_data_.size();
}

void CrDriver::DoKeyUpDown(unsigned int slot) {
  if (!slave_mode_) {
    if (slot == 1 || slot == 30) {
      not_key_->On();
    } else if (slot == 24 || slot == 53) {
      not_key_->Off();
    }
  }
}

void CrDriver::FlushSerialInput() {
  static byte buffer[255];
  int available;
  if ((available = radio_serial_->available()) > 0) {
    radio_serial_->readBytes(buffer, available);
  }
}

bool CrDriver::GetLastByte(byte* out) {
  byte buffer[255];
  int available = radio_serial_->available();
  if (available > 0) {
    radio_serial_->readBytes(buffer, available);
    *out = buffer[available-1];
    return true;
  }
  return false;
}

void CrDriver::DoRx(unsigned int slot) {
  static GcData data;
  data.data = 0x00;
  data.slot = slot;
  data.sequence = sequence_num_;
  data.type = GcData::RX;

  bool gotbyte = GetLastByte(&data.data);

  if (slave_mode_ && slot == 3) {
    if (data.data == kPsync || data.data == kGsync || data.data == kEsync) {
      slave_syncs_missed_ = 0;
    } else {
      slave_syncs_missed_++;
      if (slave_syncs_missed_ > 5) {
        debug_serial_->println("999-Missing sync, searching...");
        SlaveResync();
      }
    }
  }

  if (!gotbyte && fake_mode_ && !slave_mode_ && (current_sync_ == kGsync)) {
    data.data = faker_.GetRxForSlot(slot);
    if (data.data != 0x00) {
      gotbyte = true;
    }
  }

  if (gotbyte) {
    gc_interface_->SendOutput(data);
  }
}

void CrDriver::DoTx(unsigned int slot) {
  GcData data;
  data.type = GcData::TX;
  data.slot = slot;
  data.sequence = sequence_num_;

  if (slave_mode_) {
    if (fake_mode_) {
      if ((slot >= kRedRfStart && slot < (kRedRfStart + 20)) ||
          (slot >= kGreenRfStart && slot < (kGreenRfStart + 20))) {
        data.data = faker_.GetRxForSlot(slot);
        if (data.data != 0x00) {
          radio_serial_->write(data.data);
          gc_interface_->SendOutput(data);
        }
      }
    }
  } else {
    if (!IsTxSlot(slot)) {
      return;
    }
    if (slot == 0) {
      if (local_mode_) {
        // send what's in digital read.
        data.data = kPsync;
        if (digitalRead(options_.gsync_input_pin)) {
          data.data = kGsync;
        }
      } else {
        data.data = current_sync_;
      }
      radio_serial_->write(data.data);
      gc_interface_->SendOutput(data);
    } else {
      // transmit from queue.
      if (!transmit_queue_.Empty()) {
        data.data = transmit_queue_.Pop();
        radio_serial_->write(data.data);
        gc_interface_->SendOutput(data);
      }
    }
  }
}

void CrDriver::EnterSlaveMode() {
  debug_serial_->println("999-Entering slave mode.");
  // Disable the timer.
  not_key_->On();
  SlaveResync();
}

void CrDriver::SlaveResync() {
  sync_count_ = 0;
  timer_enable_->Off();
  searching_ = true;
  slave_mode_ = true;
  FlushSerialInput();
}

void CrDriver::SlaveModeLoop() {
  byte data;
  if (GetLastByte(&data)) {
    if (data == kPsync || data == kGsync || data == kGsync) {
      sync_count_++;
      if (sync_count_ == 3) {
        delay(8);  // delay almost half a slot before enabling our clock,
                   // otherwise we'll be too close.
        timer_enable_->On();
        searching_ = false;
        rf_slot_id_ = 3;
        slave_syncs_missed_ = 0;
        debug_serial_->println("999-Found sync!");
      }
    }
  }
  CheckModeChanges();
}

void CrDriver::EnterMasterMode() {
  debug_serial_->println("999-Entering master mode.");
  slave_mode_ = false;
  searching_ = false;
  timer_enable_->On();
}

Faker::Faker() {
  for (unsigned int i = 0; i < sizeof(is_fake_); i++) {
    is_fake_[i] = false;
    has_base_[i] = false;
  }
  for (unsigned int i = 0; i < 10; ++i) {
    is_fake_[i] = true;
    is_fake_[i+20] = true;
  }
}

void Faker::Reset() {
  randomSeed(analogRead(0));
  for (unsigned int i = 0; i < sizeof(is_fake_); i++) {
    has_base_[i] = false;
  }
}

unsigned char Faker::GetRxForSlot(unsigned int slot) {
  int i = GetPlayerForSlot(slot);
  if (i == -1) {
    return 0x00;
  }
  if (!is_fake_[i]) {
    return 0x00;
  }
  unsigned char ourbase;
  if (i >= 20) {
    ourbase = kRedBase;
  } else {
    ourbase = kGreenBase;
  }
  int myrand = random(100);
  if (myrand == 98) {
    has_base_[i] = true;
  } else if (myrand == 97) {
    // hit own player, return own IR code.
    return GetIrForPlayer(i);
  } else if (myrand < 80) {
    myrand %= 10;
    if (i < 20) {
      myrand += 20;
    }
    if (is_fake_[myrand]) {
      return GetIrForPlayer(myrand);
    }
  }
  if (has_base_[i]) {
    return ourbase;
  } else {
    return GetIdForSlot(slot);
  }
}

int Faker::GetPlayerForSlot(unsigned int slot) {
  int possible_red = slot - kRedRfStart;
  int possible_green = slot - kGreenRfStart;
  if (possible_green >= 0 && possible_green < 20) {
    return possible_green + 20;
  } else if (possible_red >= 0 && possible_red < 20) {
    return possible_red;
  } else {
    return -1;
  }
}

unsigned char Faker::GetIdForSlot(unsigned int slot) {
  // TODO
  return 0x60;
}

unsigned char Faker::GetIrForPlayer(unsigned int i) {
  if (i < 20) {
    return i + kRedIrStart;
  }
  if (i == 20) {
    return 0x94;
  }
  return (i - 20) + kGreenIrStart;
}

}  // namespace photon
