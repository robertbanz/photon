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

#include <StandardCplusplus.h>
#include <system_configuration.h>
#include <unwind-cxx.h>
#include <utility.h>

#include "gc_interface.h"
#include "cr_driver.h"

static photon::CrDriver* crdriver = nullptr;
static photon::GcInterface* gcinterface = nullptr;

// Skanky Arduino Ethernet Config.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 3, 78 };

void setup() {
  photon::CrDriverOptions options;
  options.slot_clock_interrupt_pin = 2;
  options.timer_enable_output_pin = 3;
  options.key_output_pin = 4;
  options.esync_output_pin = 5;
  options.psync_output_pin = 6;
  options.gsync_output_pin = 7;
  options.local_mode_input_pin = 8;
  options.gsync_input_pin = 9;
  options.game_input_pin = 10;
  options.slave_input_pin = 11;
  options.fake_mode_pin = 12;
  options.radio_serial = &Serial3;
  options.debug_serial = &Serial;
  
  Serial.begin(115200);  // Set to the serial rate you want to use.
  
  gcinterface = new photon::GcInterface(&Serial, nullptr);
  crdriver = new photon::CrDriver(options, gcinterface);
}

void loop() {
  crdriver->Loop();
  gcinterface->CheckInputs(crdriver);
}
