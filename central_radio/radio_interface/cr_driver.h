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

#include <algorithm>
#include <memory>
#include <queue>
#include <vector>

#include "gc_interface.h"
#include "photon.h"

#include "Arduino.h"


namespace photon {

struct CrDriverOptions {
  unsigned int slot_clock_interrupt_pin;
  unsigned int timer_enable_output_pin;
  unsigned int key_output_pin;
  unsigned int esync_output_pin;
  unsigned int psync_output_pin;
  unsigned int gsync_output_pin;
  unsigned int local_mode_input_pin;
  unsigned int gsync_input_pin;
  unsigned int game_input_pin;
  unsigned int slave_input_pin;
  unsigned int fake_mode_pin;
  HardwareSerial* radio_serial;
  HardwareSerial* ir_serial;
  HardwareSerial* debug_serial;
};

class TxQueue {
 public:
   TxQueue(HardwareSerial* debug_serial, unsigned int max)
      : debug_serial_(debug_serial), max_(max) {}
   ~TxQueue() {}
 
   void Clear() {
     queue_.clear();
   }
 
   bool Empty() {
     return queue_.empty();
   }
 
   unsigned int Size() {
     return queue_.size();
   }
 
   unsigned char Pop() {
     unsigned char out = queue_.front();
     queue_.erase(queue_.begin());
     return out;
   }
 
   void Insert(unsigned char in) {
     if (queue_.size() > max_) {
       // Check to see if he's in the queue already.
       for (auto i : queue_) {
         if (i == in) {
           debug_serial_->println("999-Queue Full: not adding duplicate");
           return;
         }
       }
       // De-dup the queue.
       size_t previous = queue_.size();
       std::sort(queue_.begin(), queue_.end());
       queue_.erase(std::unique(queue_.begin(), queue_.end()), queue_.end());
       // Shuffle it so it doesn't favor putting down any one team first.
       std::random_shuffle(queue_.begin(), queue_.end());
       if (queue_.size() != previous) {
         debug_serial_->print("999-DeDup'd txqueue from ");
         debug_serial_->print(previous);
         debug_serial_->print(" to ");
         debug_serial_->println(queue_.size());
       }
     }
     // Go ahead and add it if it isn't a dup, and we're not 2x max
     if (queue_.size() < (2 * max_)) {
       queue_.push_back(in);
     } else {
       debug_serial_->println("999-Queue Overfull");
     }
   }
 
 private:
   HardwareSerial* debug_serial_;
   const unsigned int max_;
   std::vector<unsigned char> queue_;   

};

class Faker {
 public:
   Faker();
   ~Faker() { }
   // Clears has_base state;
   void Reset();
   // If there's a faker for this slot, return it, else 0.
   unsigned char GetRxForSlot(unsigned int slot);
 private:
   int GetPlayerForSlot(unsigned int slot);
   unsigned char GetIdForSlot(unsigned int slot);
   unsigned char GetIrForPlayer(unsigned int i);
   bool is_fake_[40];
   bool has_base_[40];
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
  CrDriver(const CrDriverOptions& options, GcInterface* interface);
  ~CrDriver();
  void Loop();

  void SetSync(unsigned char sync) {
    current_sync_ = sync;
    if (current_sync_ == kPsync) {
      faker_.Reset();
    }
  }

  void Transmit(unsigned char data) {
    transmit_queue_.Insert(data);
  }
  
  void SendIr(std::vector<unsigned char> data) {
    ir_out_data_.clear();
    ir_position_ = 0;
    ir_out_data_ = data;
  }
  
  void StopIr() {
    ir_out_data_.clear();
    ir_position_ = 0;
  }
  
 private:
  // Called at the end of every time slice. 
  void SlotInterrupt(int diff);
  
  void DoSlot();
  void DoIr();
  void DoTx(unsigned int slot);
  void DoRx(unsigned int slot);
  void DoKeyUpDown(unsigned int slot);

  void CheckModeChanges();

  void EnterSlaveMode();
  void EnterMasterMode();
  void SlaveResync();
  void FlushSerialInput();
  bool GetLastByte(byte* out);
  void SlaveModeLoop();
  
  // Serial line to use for rx
  HardwareSerial* radio_serial_;
  
  HardwareSerial* debug_serial_;
  
  Signal* timer_enable_;
  Signal* not_key_;
  Signal* esync_;
  Signal* psync_;
  Signal* gsync_;

  bool slave_mode_;
  bool searching_;
  int sync_count_;
  int slave_syncs_missed_;
  // If in slave mode, generates fake data for in pod tx slots.
  // If not, generates fake received data in pod tx slots.
  bool fake_mode_;
  
  // In local mode.
  bool local_mode_;
  
  // Current RF slot.
  unsigned int rf_slot_id_;

  // Last time we ran the interrupt function.
  unsigned long last_time_micros_;
  
  // Next time we need to run the interrupt function.
  unsigned long target_micros_;
  
  // # of microseconds in a slot.
  unsigned long slot_micros_;
  unsigned int slot_micros_fraction_numerator_;
  unsigned int slot_micros_fraction_denominator_;
  
  // Transmission queue.
  TxQueue transmit_queue_;
  
  // Current sync signal.
  unsigned char current_sync_;
  
  // Current sequence id
  unsigned char sequence_num_;
  GcInterface* gc_interface_;
  
  // Options
  const CrDriverOptions options_;
  
  // IR thingy.
  std::vector<unsigned char> ir_out_data_;
  unsigned int ir_position_;
  
  Faker faker_;
};

}  // namespace

#endif  // CR_DRIVER