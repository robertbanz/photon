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


// Returns true if the data in buffer is a valid ET packet.
bool ValidateEtPacket(const std::vector<unsigned char>& buffer);

enum TargetType { RED_POD, GREEN_POD, OTHER, INVALID };

// Return the target type for the IR code
TargetType TargetTypeFromIrCode(unsigned char value);

}  // namespace

#endif  // _COMMON_PHOTON_H
