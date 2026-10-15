#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "protocol.h"

#ifndef ESPNOW_CHANNEL
#define ESPNOW_CHANNEL 1
#endif

#ifndef SERIAL_BAUD
#define SERIAL_BAUD 921600
#endif

#ifdef DEBUG_TEXT
#define DBG_INIT() do { Serial.begin(115200); delay(300); } while (0)
#define DBG_PRINTF(...) Serial.printf(__VA_ARGS__)
#define DBG_PRINTLN(x) Serial.println(x)
#else
#define DBG_INIT() do { Serial.begin(SERIAL_BAUD); delay(200); } while (0)
#define DBG_PRINTF(...) do {} while (0)
#define DBG_PRINTLN(x) do {} while (0)
#endif

struct RxItem {
    uint8_t mac[6];
    uint16_t len;
    uint8_t data[ESPNOW_MAX_PAYLOAD];
};

static QueueHandle_t g_rxQueue = nullptr;
static volatile uint32_t g_queueDrops = 0;

#ifdef DEBUG_TEXT
struct CamStats {
    uint32_t packets = 0;
    uint32_t bytes = 0;
    uint32_t frameStart = 0;
    uint32_t frameEnd = 0;
    uint32_t badPackets = 0;
    uint32_t lastFrameId = 0;
};
static CamStats g_stats[256];
static uint32_t g_lastStatsPrint = 0;
#endif

static void setWifiChannel(uint8_t channel) {
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
static void onDataRecv(const esp_now_recv_info_t *info,
                       const uint8_t *data, int len) {
    if (!info || !data) {
        return;
    }
    if (len <= 0 || len > (int)ESPNOW_MAX_PAYLOAD) {
        return;
    }
    if (!g_rxQueue) {
        return;
    }

    RxItem item{};
    memcpy(item.mac, info->src_addr, 6);
    item.len = (uint16_t)len;
    memcpy(item.data, data, len);

    if (xQueueSend(g_rxQueue, &item, 0) != pdTRUE) {
        g_queueDrops++;
    }
}
#else
static void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
    if (!mac || !data) {
        return;
    }
    if (len <= 0 || len > (int)ESPNOW_MAX_PAYLOAD) {
        return;
    }
    if (!g_rxQueue) {
        return;
    }

    RxItem item{};
    memcpy(item.mac, mac, 6);
    item.len = (uint16_t)len;
    memcpy(item.data, data, len);

    if (xQueueSend(g_rxQueue, &item, 0) != pdTRUE) {
        g_queueDrops++;
    }
}
#endif

#ifndef DEBUG_TEXT
static void sendBinaryToSerial(const RxItem &item) {
    uint8_t payload[8 + ESPNOW_MAX_PAYLOAD];
    memcpy(payload, item.mac, 6);
    payload[6] = (uint8_t)(item.len & 0xFF);
    payload[7] = (uint8_t)((item.len >> 8) & 0xFF);
    memcpy(payload + 8, item.data, item.len);

    const uint16_t payloadLen = (uint16_t)(8 + item.len);

    SerialFrameHeader hdr{};
    hdr.magic[0] = 'E';
    hdr.magic[1] = 'I';
    hdr.magic[2] = 'M';
    hdr.magic[3] = 'G';
    hdr.payloadLen = payloadLen;
    hdr.payloadCrc = crc16_ccitt(payload, payloadLen);

    Serial.write((const uint8_t *)&hdr, sizeof(hdr));
    Serial.write(payload, payloadLen);
}
#endif

#ifdef DEBUG_TEXT
static void handleDebugPacket(const RxItem &item) {
    if (item.len < sizeof(CameraChunkHeader)) {
        return;
    }

    const auto *hdr = (const CameraChunkHeader *)item.data;
    CamStats &s = g_stats[hdr->camId];

    if (hdr->magic != PROTO_MAGIC || hdr->version != PROTO_VERSION) {
        s.badPackets++;
        return;
    }

    if (sizeof(CameraChunkHeader) + hdr->chunkLen != item.len) {
        s.badPackets++;
        return;
    }

    const uint8_t *chunk = item.data + sizeof(CameraChunkHeader);
    const uint16_t crc = crc16_ccitt(chunk, hdr->chunkLen);
    if (crc != hdr->chunkCrc) {
        s.badPackets++;
        return;
    }

    s.packets++;
    s.bytes += hdr->chunkLen;
    s.lastFrameId = hdr->frameId;

    if (hdr->flags & FLAG_START) {
        s.frameStart++;
        DBG_PRINTF("[RX] cam=%u frame=%lu START total=%lu\n",
                   hdr->camId,
                   (unsigned long)hdr->frameId,
                   (unsigned long)hdr->totalSize);
    }

    if (hdr->flags & FLAG_END) {
        s.frameEnd++;
        DBG_PRINTF("[RX] cam=%u frame=%lu END off=%lu len=%u\n",
                   hdr->camId,
                   (unsigned long)hdr->frameId,
                   (unsigned long)hdr->offset,
                   hdr->chunkLen);
    }
}

static void printStats() {
    const uint32_t now = millis();
    if (now - g_lastStatsPrint < 2000) {
        return;
    }
    g_lastStatsPrint = now;

    DBG_PRINTF("---- HUB stats (queueDrops=%lu) ----\n",
               (unsigned long)g_queueDrops);

    for (uint16_t cam = 0; cam < 256; cam++) {
        const CamStats &s = g_stats[cam];
        if (s.packets == 0 && s.badPackets == 0) {
            continue;
        }

        DBG_PRINTF("cam=%u pkts=%lu bytes=%lu start=%lu end=%lu bad=%lu "
                   "lastFrame=%lu\n",
                   cam,
                   (unsigned long)s.packets,
                   (unsigned long)s.bytes,
                   (unsigned long)s.frameStart,
                   (unsigned long)s.frameEnd,
                   (unsigned long)s.badPackets,
                   (unsigned long)s.lastFrameId);
    }
}
#endif

static bool initEspNow() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    setWifiChannel(ESPNOW_CHANNEL);

    if (esp_now_init() != ESP_OK) {
        return false;
    }

    esp_now_register_recv_cb(onDataRecv);

    DBG_PRINTF("HUB MAC: %s\n", WiFi.macAddress().c_str());
    DBG_PRINTF("HUB channel: %u\n", ESPNOW_CHANNEL);
#ifndef DEBUG_TEXT
    DBG_PRINTF("HUB serial baud: %u\n", SERIAL_BAUD);
#endif
    return true;
}

void setup() {
    DBG_INIT();
    DBG_PRINTLN("\nHUB boot");

    g_rxQueue = xQueueCreate(32, sizeof(RxItem));
    if (!g_rxQueue) {
        DBG_PRINTLN("Queue create FAIL");
        while (true) {
            delay(1000);
        }
    }

    DBG_PRINTLN("initEspNow...");
    if (!initEspNow()) {
        DBG_PRINTLN("initEspNow FAIL");
        while (true) {
            delay(1000);
        }
    }
    DBG_PRINTLN("initEspNow OK");

#ifdef DEBUG_TEXT
    DBG_PRINTLN("DEBUG_TEXT mode: binary stream disabled");
#endif
}

void loop() {
    RxItem item{};
    if (xQueueReceive(g_rxQueue, &item, pdMS_TO_TICKS(100)) == pdTRUE) {
#ifdef DEBUG_TEXT
        handleDebugPacket(item);
#else
        sendBinaryToSerial(item);
#endif
    }

#ifdef DEBUG_TEXT
    printStats();
#endif
}