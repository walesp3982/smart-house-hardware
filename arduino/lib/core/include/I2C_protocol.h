#pragma once
#include <stdint.h>

#define PKT_SIZE 4

struct I2CPacket {
    uint8_t node_id;
    uint8_t cmd;
    uint8_t data;
    uint8_t checksum;
};

#define CMD_PING   0x01
#define CMD_STATUS 0x02
#define CMD_SET    0x03
#define CMD_ACK    0xff

inline uint8_t pkt_checksum(const I2CPacket& p)  {
    return p.node_id ^ p.cmd ^p.data;
}

inline bool pkt_valid(const I2CPacket& p) {
    return p.checksum == pkt_checksum(p);
}