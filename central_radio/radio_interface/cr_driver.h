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

#ifndef _CRDRIVER_H
#define _CRDRIVER_H

#include <memory>
#include <queue>

#include "gc_interface.h"

#include "Arduino.h"


namespace photon {

class TxQueue {
 public:
   TxQueue() {}
   ~TxQueue() {}
 
   void Clear() {
     while (!queue_.empty()) {
       queue_.pop();
     }
   }
 
   bool Empty() {
     return queue_.empty();
   }
 
   unsigned int Size() {
     return queue_.size();
   }
 
   unsigned char Pop() {
     unsigned char out = queue_.front();
     queue_.pop();
     return out;
   }
 
   void Insert(unsigned char in) {
     queue_.push(in);
   }
 
 private:
   std::queue<unsigned char> queue_;   

};


class Signal {
 public:
  Signal(const int led_pin) :
    led_pin_(led_pin) {
    pinMode(led_pin_, OUTPUT);
    initial_state_ = last_state_ = digitalRead(led_pin_);
  }
  ~Signal() {
    digitalWrite(led_pin_, initial_state_);
  }
  inline void On() {
    if (last_state_ != HIGH) {
      last_state_ = HIGH;
      digitalWrite(led_pin_, last_state_);
    }
  }
  inline void Off() {
    if (last_state_ != LOW) {
      last_state_ = LOW;
      digitalWrite(led_pin_, last_state_);
    }
  }
private:
  const int led_pin_;
  int initial_state_;
  int last_state_;
  int rf_slot_id_;
};

class GcInterface;

class CrDriver {
 public:
  CrDriver(HardwareSerial* radio_out, unsigned int sync_pin,
           unsigned int key_pin, unsigned int tx_pin, unsigned int rx_pin,
           GcInterface* interface);
  ~CrDriver();
  void Loop();

  void SetSync(unsigned char sync) {
    current_sync_ = sync;
  }

  void Transmit(unsigned char data) {
    transmit_queue_.Insert(data);
  }
  
 private:
  // Called at the end of every time slice. 
  void SlotInterrupt();
  
  void DoSlot();
  void DoTx(unsigned int slot);
  void DoRx(unsigned int slot);
  void DoKeyUpDown(unsigned int slot);
  
  // Owned by caller.
  HardwareSerial* radio_serial_out_;

  std::auto_ptr<Signal> sync_signal_;
  std::auto_ptr<Signal> key_signal_;
  std::auto_ptr<Signal> tx_signal_;
  std::auto_ptr<Signal> rx_signal_; 

  // Current RF slot.
  unsigned int rf_slot_id_;

  // Last time we ran the interrupt function.
  unsigned long last_time_micros_;
  
  // Next time we need to run the interrupt function.
  unsigned long target_micros_;
  
  // # of microseconds in a slot.
  int slot_micros_;

  // Transmission queue.
  TxQueue transmit_queue_;
  
  // Current sync signal.
  unsigned char current_sync_;
  
  // Current sequence id
  unsigned char sequence_num_;
  GcInterface* gc_interface_;
};

}  // namespace

#endif  // CR_DRIVER