// Copyright (c) 2016,2018 Robert Banz
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

#ifndef _COMMON_PHOTON_H
#define _COMMON_PHOTON_H

#include <vector>

namespace photon {

extern const unsigned char kEtBarker;
extern const unsigned char kEtNotBarker;
extern const unsigned char kEtPacketSize;

extern const unsigned char kRedIrStart;
extern const unsigned char kGreenIrStart;
extern const unsigned char kGreenIrSlotOneReplacement;
extern const unsigned char kPlayersPerTeam;

extern const unsigned char kRedBase;
extern const unsigned char kRedBase2;
extern const unsigned char kGreenBase;
extern const unsigned char kGreenBase2;
extern const unsigned char kRedTower;
extern const unsigned char kGreenTower;
extern const unsigned char kTarget;

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

// Returns true if the data in buffer is a valid ET packet.
bool ValidateEtPacket(const std::vector<unsigned char>& buffer);

enum TargetType { RED_POD, GREEN_POD, OTHER, INVALID };

// Return the target type for the IR code
TargetType TargetTypeFromIrCode(unsigned char value);

const char* GetSyncNameFromByte(const unsigned char byte);
unsigned char GetSyncByteFromName(const char* name);

// Returns true if the radio slot is used for transmitting.
bool IsTxSlot(unsigned int slot);

// Retruns true if the radio slot is used for receiving.
bool IsRealRxSlot(unsigned int slot);

// Mapping between sync names & bytes.
const char* GetSyncNameFromByte(const unsigned char byte);
unsigned char GetSyncByteFromName(const char* name);

}  // namespace

#endif  // _COMMON_PHOTON_H
