#ifndef _COMMON_PHOTON_H
#define _COMMON_PHOTON_H

#include <vector>

namespace photon {

constexpr unsigned char kEtBarker = 0x9d;
constexpr unsigned char kEtNotBarker = 0x62;
constexpr unsigned char kEtPacketSize = 6;

constexpr unsigned char kRedIrStart = 0x42;
constexpr unsigned char kGreenIrStart = 0x80;
constexpr unsigned char kGreenIrSlotOneReplacement = 0x94;
constexpr unsigned char kPlayersPerTeam = 20;

constexpr unsigned char kRedBase = 0x35;
constexpr unsigned char kRedBase2 = 0x36;
constexpr unsigned char kGreenBase = 0x2B;
constexpr unsigned char kGreenBase2 = 0x2D;
constexpr unsigned char kRedTower = 0x32;
constexpr unsigned char kGreenTower = 0x24;
constexpr unsigned char kTarget = 0x38;

enum TargetType { RED_POD, GREEN_POD, OTHER, INVALID };

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

}  // namespace

#endif  // _COMMON_PHOTON_H
