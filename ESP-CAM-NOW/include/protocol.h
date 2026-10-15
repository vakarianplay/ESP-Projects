#pragma once

#include <Arduino.h>
#include <stdint.h>

static constexpr uint8_t PROTO_MAGIC = 0xEC;
static constexpr uint8_t PROTO_VERSION = 1;

static constexpr size_t ESPNOW_MAX_PAYLOAD = 250;
static constexpr size_t CHUNK_DATA_SIZE = 200;

enum ChunkFlags : uint8_t {
    FLAG_START = 0x01,
    FLAG_END = 0x02
};

#pragma pack(push, 1)
struct CameraChunkHeader {
    uint8_t magic;
    uint8_t version;
    uint8_t camId;
    uint8_t flags;
    uint32_t frameId;
    uint32_t totalSize;
    uint32_t offset;
    uint16_t chunkLen;
    uint16_t chunkCrc;
};

struct SerialFrameHeader {
    char magic[4];      // "EIMG"
    uint16_t payloadLen;
    uint16_t payloadCrc;
};
#pragma pack(pop)

static_assert(sizeof(CameraChunkHeader) + CHUNK_DATA_SIZE <= ESPNOW_MAX_PAYLOAD,
              "ESP-NOW packet too large");

inline uint16_t crc16_ccitt(const uint8_t *data, size_t len,
                            uint16_t seed = 0xFFFF) {
    uint16_t crc = seed;
    while (len--) {
        crc ^= (uint16_t)(*data++) << 8;
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x8000) {
                crc = (uint16_t)((crc << 1) ^ 0x1021);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}