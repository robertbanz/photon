
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

}  // namespace photon
