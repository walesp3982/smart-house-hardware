#pragma once
#include <stdint.h>

#define PKT_SIZE 4

struct I2CPacket {
    uint8_t node_id;
    uint8_t cmd;
    uint8_t data;
    uint8_t checksum;
};

constexpr uint8_t CMD_PING   = 0x01;
constexpr uint8_t CMD_STATUS = 0x02;
constexpr uint8_t CMD_SET = 0X03;
constexpr uint8_t CMD_ACK = 0xFF;
constexpr uint8_t CMD_TEMP = 0x04;


inline uint8_t pkt_checksum(const I2CPacket& p)  {
    return p.node_id ^ p.cmd ^p.data;
}

inline bool pkt_valid(const I2CPacket& p) {
    return p.checksum == pkt_checksum(p);
}