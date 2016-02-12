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

#ifndef _GC_INTERFACE_H
#define _GC_INTERFACE_H

#include <string>
#include <vector>

#include "cr_driver.h"

#include "Arduino.h"
#include "Ethernet.h"

namespace photon {

class CrDriver;

struct GcData {
  enum Type {
    RX,
    TX
  };
  unsigned char sequence;
  Type type;
  unsigned int slot;
  unsigned char data;
  static const char* TypeToCstring(Type type) {
    if (type == RX) {
      return "rx";
    } else if (type == TX) {
      return "tx";
    } else {
      return "XX";
    }
  }
  std::string ToString() const;
};
  
enum GcCommand {
  READ,
  WRITE,
  SETSYNC,
  ENABLE,
  FAKE,
  UNKNOWN
};

GcCommand ParseGcCommand(const std::string& command);

class GcConnection {
 public:
  virtual std::string ProcessInput() = 0;
  virtual bool IsAlive() = 0;
  virtual void SendOutput(const std::string& output) = 0;
 protected:
   GcConnection() { };
   virtual ~GcConnection() { }
};

class SerialGcConnection : public GcConnection {
 public:
  SerialGcConnection(HardwareSerial* serial) : connection_(serial) { }
  ~SerialGcConnection() override { };
  std::string ProcessInput() override;
  bool IsAlive() override;
  void SendOutput(const std::string& output) override;
 private:
   HardwareSerial* connection_;
   std::string input_buffer_;
};

class EthernetGcConnection : public GcConnection {
 public:
  EthernetGcConnection(EthernetServer* server) : server_(server) { }
  ~EthernetGcConnection() override { };
  std::string ProcessInput() override;
  bool IsAlive() override;
  void SendOutput(const std::string& output) override;
 private:
   EthernetServer* server_;
   std::string input_buffer_;
};

class GcInterface {
 public:
  GcInterface(HardwareSerial* serial, EthernetServer* server);
  void SendOutput(const GcData& output);
  void CheckInputs(CrDriver* driver);
  void ParseAndDo(CrDriver* driver, const std::string& input,
                                 GcConnection* connection);
 private:
  std::vector<GcConnection*> connections_;
  EthernetServer* server_;
};

}  // namespace photon

#endif // _GC_INTERFACE_H