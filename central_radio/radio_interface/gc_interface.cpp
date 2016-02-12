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

#include "gc_interface.h"

#include <string>
#include <locale>

#include "photon.h"

namespace photon {

std::string GcData::ToString() const {
  char buffer[255];
  std::string out;
  snprintf(buffer, 255,
           "905 %d %s %d %d", sequence, TypeToCstring(type), slot, data);
  out.assign(buffer);
  return out;
}


std::string SerialGcConnection::ProcessInput() {
  char buffer[2];
  unsigned int num_bytes;
  std::string line;
  num_bytes = connection_->available();
  if (num_bytes > 0) {
    int read = connection_->readBytes(
        buffer, num_bytes <= sizeof(buffer) ? num_bytes : sizeof(buffer));
    input_buffer_.append(buffer, read);
    unsigned int pos = input_buffer_.find('\n');
    if (pos != std::string::npos) {
      line = input_buffer_.substr(0, pos);
      connection_->println(line.c_str());
      input_buffer_.replace(0, pos + 1, "");
    }
  }
  return line;
}


bool EthernetGcConnection::IsAlive() {
  return true;
}

void EthernetGcConnection::SendOutput(const std::string& output) {
  server_->println(output.c_str());
}

std::string EthernetGcConnection::ProcessInput() {
  EthernetClient client;
  std::string line;
  if (client = server_->available()) {
    byte buffer[100];
    unsigned int num_bytes = client.available();
    if (num_bytes > 0) {
      unsigned int read = client.read(
          buffer, num_bytes <= (unsigned char)(sizeof(buffer) ? num_bytes :
                                               sizeof(buffer)));
      input_buffer_.append((char*) buffer, read);
      unsigned int pos = input_buffer_.find('\n');
      if (pos != std::string::npos) {
        line = input_buffer_.substr(0, pos);
        input_buffer_.replace(0, pos + 1, "");
      }
    }
  }
  return line;
  
}


bool SerialGcConnection::IsAlive() {
  return true;
}

void SerialGcConnection::SendOutput(const std::string& output) {
  connection_->println(output.c_str());
}


void GcInterface::SendOutput(const GcData& data) {
  std::string datastring = data.ToString();
  for (auto i : connections_) {
    if (i->IsAlive()) {
      i->SendOutput(datastring);
    }
  }
}

void GcInterface::CheckInputs(CrDriver* driver) {
  for (auto i : connections_) {
    if (i->IsAlive()) {
      auto input = i->ProcessInput();
      if (!input.empty()) {
        ParseAndDo(driver, input, i);
      }
    }
  }
}

void GcInterface::ParseAndDo(CrDriver* driver, const std::string& input,
                             GcConnection* connection) {
  // Pull out command name.
  int pos = input.find(' ');
  auto command = input.substr(0, pos);
  auto arg = input.substr(pos + 1, std::string::npos);
  Serial.println(command.c_str());
  Serial.println(arg.c_str());
  if (command == "SETSYNC") {
    unsigned char sync = GetSyncByteFromName(arg.c_str());
    if (sync == 0x00) {
      connection->SendOutput("503 Unknown Sync");
      return;
    } else {
      connection->SendOutput("100 SYNC");
      driver->SetSync(sync);
      return;
    }
  } else if (command == "WRITE") {
    char* result;
    int data = strtol(arg.c_str(), &result, 10);
    if (result != nullptr) {
      driver->Transmit((unsigned char) data);
      connection->SendOutput("100-WRITE");
      return;
    } else {
      connection->SendOutput("503-Bad Write");
      return; // tood: print error
    }
  } else if (command == "READ" ||
             command == "ENABLE") {
    connection->SendOutput("100 OK (ignored)");
    return;
  }
}

GcInterface::GcInterface(HardwareSerial* serial, EthernetServer* server) {
  if (serial != nullptr) {
    connections_.push_back(new SerialGcConnection(serial));
  }
  if (server != nullptr) {
    connections_.push_back(new EthernetGcConnection(server));
  }
}

}  // namespace
