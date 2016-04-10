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

#ifndef _PHOTON_H
#define _PHOTON_H

#include <map>
#include <string>

namespace photon {

extern const unsigned char kPsync;
extern const unsigned char kGsync;
extern const unsigned char kEsync;
extern const char *kPsyncName;
extern const char *kGsyncName;
extern const char *kEsyncName;
extern const char *kUnknownName;

extern unsigned int kNumSlots;

extern unsigned int kRedRfStart;
extern unsigned int kGreenRfStart;

extern unsigned char kRedBase;
extern unsigned char kGreenBase;

extern unsigned char kRedIrStart;
extern unsigned char kGreenIrStart;

const char* GetSyncNameFromByte(const unsigned char byte);
unsigned char GetSyncByteFromName(const char* name);

inline bool IsTxSlot(unsigned int slot) {
  if (slot == 0 || (slot > 24 && slot < 30) || (slot > 54 && slot < 60)) {
    return true;
  }
  return false;
}

inline bool IsRealRxSlot(unsigned int slot) {
  if (((slot >= kRedRfStart) && (slot < kRedRfStart + 20)) ||
      ((slot >= kGreenRfStart) && (slot < kGreenRfStart + 20))) {
    return true;
  }
  return false;
}

}  // namespace photon

#endif // _PHOTON_H