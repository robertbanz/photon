#ifndef _PHOTON_PHOTON_H
#define _PHOTON_PHOTON_H

#include <ArduinoSTL.h>
#include <pb.h>
#include <pb_encode.h>
#include "common/phaser.h"
#include "common/phaser.pb.h"
#include "common/photon.h"

namespace photon {
inline photon_PhaserNotification* NewPhaserNotificationMiss() {
  auto result = new photon_PhaserNotification();
  result->which_notification_oneof = photon_PhaserNotification_miss_tag;
  return result;
}
inline photon_PhaserNotification* NewPhaserNotificationHit(uint8_t target) {
  auto result = new photon_PhaserNotification();
  result->which_notification_oneof = photon_PhaserNotification_hit_tag;
  result->notification_oneof.hit.target = target;
  return result;
}
inline photon_PhaserNotification* NewPhaserNotificationEtPacket(
    const std::vector<uint8_t> data) {
  auto result = new photon_PhaserNotification();
  result->which_notification_oneof = photon_PhaserNotification_et_packet_tag;
  result->notification_oneof.et_packet.ir_code = data[2];
  result->notification_oneof.et_packet.rf_slot = data[3];
  result->notification_oneof.et_packet.mode = data[4];
  return result;
}
}  // namespace photon

#endif  // _PHOTON_PHOTON_H
