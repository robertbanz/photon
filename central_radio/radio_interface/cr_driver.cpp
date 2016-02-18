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

#include "photon.h"

namespace photon {

CrDriver::CrDriver(
  HardwareSerial* radio_out, unsigned int sync_pin,
  unsigned int key_pin, unsigned int tx_pin, unsigned int rx_pin,
  GcInterface* gc_interface) :
  radio_serial_out_(radio_out),
  sync_signal_(new Signal(sync_pin)),
  key_signal_(new Signal(key_pin)),
  tx_signal_(new Signal(tx_pin)),
  rx_signal_(new Signal(rx_pin)),
  rf_slot_id_(0),
  last_time_micros_(micros()),
  target_micros_(last_time_micros_ + 1),
  // According to maths, this should be:
  // 16352.91179384201474726715 (close to 467/512)
  slot_micros_(16350),
  transmit_queue_(TxQueue(10)),
  current_sync_(GetSyncByteFromName(kPsyncName)),
  sequence_num_(0),
  gc_interface_(gc_interface) {
    
  // CR runs at 1200,n,8,1
  radio_serial_out_->begin(1200);
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
  unsigned long current_micros = micros();
  unsigned long diff = TimeDifference(last_time_micros_, current_micros);

  static unsigned long deviation = 0;
  static unsigned long periods = 0;
  
  static unsigned long leap_counter = 0;
  static unsigned long leap_us = 0;
  
  // adjust for drift.
  if (leap_counter % 7 == 0) {
    leap_us = 1;
  } else {
    leap_us = 0;
  }
  
  if (diff < (slot_micros_ + leap_us)) {
    return;
  }

  if (diff > (slot_micros_ * 2)) {
    Serial.println("Serious Timer Overrun.");
  }
  
  deviation += (diff - slot_micros_);
  periods++;
  
  // Do stuff.
  SlotInterrupt();

  ++leap_counter;
    
  last_time_micros_ += slot_micros_ + leap_us;
  
  if (rf_slot_id_ == 1) {
    Serial.print("Mean Dev: ");
    Serial.print(deviation / periods);
    Serial.println(" us");
    deviation = 0;
    periods = 0;
  }
}

void CrDriver::SlotInterrupt() {
  if (rf_slot_id_ == kNumSlots) {
    rf_slot_id_ = 0;
    sequence_num_++;
  }
  DoSlot();
  ++rf_slot_id_;
}

void CrDriver::DoSlot() {
  if (rf_slot_id_ == 0) {
    sync_signal_->On();
  } else if (rf_slot_id_ == 1) {
    sync_signal_->Off();
  }
  
  DoTx(rf_slot_id_);
  DoRx(rf_slot_id_);
  DoKeyUpDown(rf_slot_id_);
}

void CrDriver::DoKeyUpDown(unsigned int slot) {
  if (slot == 1 || slot == 30) {
    key_signal_->On();
  } else if (slot == 24 || slot == 53) {
    key_signal_->Off();
  }
}

void CrDriver::DoRx(unsigned int slot) {
  static byte buffer[255];
  static GcData data;
  data.data = 0x00;
  data.slot = slot;
  data.sequence = sequence_num_;
  data.type = GcData::RX;

  // Always do a read.
  int available = radio_serial_out_->available();
  
  // Get the last byte in the buffer.
  if (available > 0) {
    radio_serial_out_->readBytes(buffer, available);
    data.data = buffer[available-1];  
  }
  
  if (available > 1) {
    Serial.println("rx WTF.");
  }
  
  // Fake it if we need to.
  if (IsRealRxSlot(slot) && current_sync_ == kGsync) {
    unsigned char fake = faker_.GetRxForSlot(slot);
    if (fake != 0x00) {
      data.data = fake;
    }
  }

  if (data.data != 0x00) {
    gc_interface_->SendOutput(data);
  }
}

void CrDriver::DoTx(unsigned int slot) {
  if (!IsTxSlot(slot)) {
    return;
  }
  GcData data;
  data.type = GcData::TX;
  data.slot = slot;
  data.sequence = sequence_num_;
  if (slot == 0) {
    // transmit current sync.
    radio_serial_out_->write(current_sync_);
    data.data = current_sync_;
    gc_interface_->SendOutput(data);
  } else {
    // transmit from queue.
    if (!transmit_queue_.Empty()) {
      unsigned char out = transmit_queue_.Pop();
      data.data = out;
      radio_serial_out_->write(out);
      tx_signal_->On();
      gc_interface_->SendOutput(data);
    } else {
      tx_signal_->Off();
    }
  }
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
  srandom(micros());
  for (unsigned int i = 0; i < sizeof(is_fake_); i++) {
    has_base_[i] = false;
  }  
}

unsigned char Faker::GetRxForSlot(unsigned int slot) {
  int i = GetPlayerForSlot(slot);
  if (!is_fake_[i]) {
    return 0x00;
  }
  unsigned char ourbase;
  if (i >= 20) {
    ourbase = kRedBase;
  } else {
    ourbase = kGreenBase;
  }
  int myrand = random() % 100;
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

unsigned int Faker::GetPlayerForSlot(unsigned int slot) {
  if (slot >= kGreenRfStart) {
    return (slot - kGreenRfStart) + 20;
  } else {
    return (slot - kRedRfStart);
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