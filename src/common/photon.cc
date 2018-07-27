
#include "common/photon.h"

namespace photon {

const unsigned char kEtBarker = 0x9d;
const unsigned char kEtNotBarker = 0x62;
const unsigned char kEtPacketSize = 6;

const unsigned char kRedIrStart = 0x42;
const unsigned char kGreenIrStart = 0x80;
const unsigned char kGreenIrSlotOneReplacement = 0x94;
const unsigned char kPlayersPerTeam = 20;

const unsigned char kRedBase = 0x35;
const unsigned char kRedBase2 = 0x36;
const unsigned char kGreenBase = 0x2B;
const unsigned char kGreenBase2 = 0x2D;
const unsigned char kRedTower = 0x32;
const unsigned char kGreenTower = 0x24;
const unsigned char kTarget = 0x38;

const unsigned char kPsync = 0xc5;
const unsigned char kGsync = 0xca;
const unsigned char kEsync = 0xdd;

const char* kPsyncName = "PSYNC";
const char* kGsyncName = "GSYNC";
const char* kEsyncName = "ESYNC";
const char* kUnknownName = "UNKNOWN";

unsigned int kNumSlots = 67;

unsigned int kRedRfStart = 5;
unsigned int kGreenRfStart = 31;

bool ValidateEtPacket(const std::vector<unsigned char>& buffer) {
  if (buffer.size() != kEtPacketSize) {
    return false;
  }
  const unsigned char checksum =
      buffer[0] ^ buffer[1] ^ buffer[2] ^ buffer[3] ^ buffer[4];
  if (buffer[0] != kEtBarker) {
    return false;
  }
  if (buffer[1] != kEtNotBarker) {
    return false;
  }
  if (checksum != buffer[5]) {
    return false;
  }
  return true;
}

TargetType TargetTypeFromIrCode(unsigned char value) {
  if ((value >= kRedIrStart) && (value <= kRedIrStart + kPlayersPerTeam)) {
    return TargetType::RED_POD;
  }
  if ((value >= kGreenIrStart + 1) &&
      (value <= kGreenIrStart + kPlayersPerTeam)) {
    return TargetType::GREEN_POD;
  }
  if (value == kGreenIrSlotOneReplacement) {
    return TargetType::GREEN_POD;
  }
  if (value == kRedBase || value == kGreenBase) {
    return TargetType::OTHER;
  }
  return TargetType::INVALID;
}

bool IsTxSlot(unsigned int slot) {
  if (slot == 0 || (slot > 24 && slot < 30) || (slot > 54 && slot < 60)) {
    return true;
  }
  return false;
}

bool IsRealRxSlot(unsigned int slot) {
  if (((slot >= kRedRfStart) && (slot < kRedRfStart + 20)) ||
      ((slot >= kGreenRfStart) && (slot < kGreenRfStart + 20))) {
    return true;
  }
  return false;
}

const char* GetSyncNameFromByte(const unsigned char byte) {
  switch (byte) {
    case 0xc5:
      return kPsyncName;
      break;
    case 0xca:
      return kGsyncName;
      break;
    case 0xdd:
      return kEsyncName;
      break;
  }
  return kUnknownName;
}

unsigned char GetSyncByteFromName(const char* name) {
  if (!strcmp(kPsyncName, name)) {
    return 0xc5;
  } else if (!strcmp(kGsyncName, name)) {
    return 0xca;
  } else if (!strcmp(kEsyncName, name)) {
    return 0xdd;
  } else {
    return 0x00;
  }
}

}  // namespace photon
