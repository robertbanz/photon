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
  target_micros_(last_time_micros_),
  slot_micros_(16000),
  current_sync_(GetSyncByteFromName(kPsyncName)),
  sequence_num_(0),
  gc_interface_(gc_interface) {
    
  // CR runs at 1200,n,8,1
  radio_serial_out_->begin(1200);
}

void CrDriver::Loop() {
  unsigned long current_micros = micros();
  if (current_micros >= target_micros_) {
    target_micros_ += slot_micros_;
    last_time_micros_ = current_micros;
    // TODO: deal with overruns.
    SlotInterrupt();
  }
}

void CrDriver::SlotInterrupt() {
  if (rf_slot_id_ >= kNumSlots) {
    rf_slot_id_ = 0;
    sequence_num_++;
  }
  DoSlot();
  // gc_interface->write events
  ++rf_slot_id_;
}

void CrDriver::DoSlot() {
  if (rf_slot_id_ == 0) {
    sync_signal_->On();
  } else if (rf_slot_id_ == 1) {
    sync_signal_->Off();
  }
  
  DoKeyUpDown(rf_slot_id_);

  DoRx(rf_slot_id_);
  
  DoTx(rf_slot_id_);
}

void CrDriver::DoKeyUpDown(unsigned int slot) {
  if (slot == 1 || slot == 30) {
    key_signal_->Off();
  } else if (slot == 24 || slot == 53) {
    key_signal_->On();
  }
}

void CrDriver::DoRx(unsigned int slot) {
  if (IsTxSlot(slot)) {
    return;
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

}  // namespace photon