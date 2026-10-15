#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "esp_camera.h"
#include "protocol.h"

#ifndef CAM_ID
#define CAM_ID 1
#endif

#ifndef ESPNOW_CHANNEL
#define ESPNOW_CHANNEL 1
#endif

#ifndef CAPTURE_INTERVAL_MS
#define CAPTURE_INTERVAL_MS 60000UL
#endif

#ifndef PRELIGHT_MS
#define PRELIGHT_MS 2000UL
#endif

#ifndef HUB_MAC_0
#error "Define HUB_MAC_0..HUB_MAC_5 in build_flags"
#endif

#ifdef DEBUG_TEXT
#define DBG_INIT() do { Serial.begin(115200); delay(300); } while (0)
#define DBG_PRINTF(...) Serial.printf(__VA_ARGS__)
#define DBG_PRINTLN(x) Serial.println(x)
#else
#define DBG_INIT() do {} while (0)
#define DBG_PRINTF(...) do {} while (0)
#define DBG_PRINTLN(x) do {} while (0)
#endif

static const uint8_t HUB_MAC[6] = {
    HUB_MAC_0, HUB_MAC_1, HUB_MAC_2,
    HUB_MAC_3, HUB_MAC_4, HUB_MAC_5
};

// AI Thinker ESP32-CAM pins
#define PWDN_GPIO_NUM   32
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM   0
#define SIOD_GPIO_NUM   26
#define SIOC_GPIO_NUM   27
#define Y9_GPIO_NUM     35
#define Y8_GPIO_NUM     34
#define Y7_GPIO_NUM     39
#define Y6_GPIO_NUM     36
#define Y5_GPIO_NUM     21
#define Y4_GPIO_NUM     19
#define Y3_GPIO_NUM     18
#define Y2_GPIO_NUM     5
#define VSYNC_GPIO_NUM  25
#define HREF_GPIO_NUM   23
#define PCLK_GPIO_NUM   22
#define FLASH_LED_PIN   4

static volatile bool g_sendDone = false;
static volatile esp_now_send_status_t g_sendStatus = ESP_NOW_SEND_FAIL;
static uint32_t g_frameId = 0;
static uint32_t g_nextShotAt = 0;

static String macToString(const uint8_t *mac) {
    char b[18];
    snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(b);
}

static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    (void)mac_addr;
    g_sendStatus = status;
    g_sendDone = true;
}

static void setWifiChannel(uint8_t channel) {
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
}

static bool initEspNow() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    setWifiChannel(ESPNOW_CHANNEL);

    if (esp_now_init() != ESP_OK) {
        return false;
    }

    esp_now_register_send_cb(onDataSent);

    esp_now_peer_info_t peer{};
    memcpy(peer.peer_addr, HUB_MAC, 6);
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;

    if (esp_now_add_peer(&peer) != ESP_OK) {
        return false;
    }

    DBG_PRINTF("CAM%d WiFi MAC: %s\n", CAM_ID, WiFi.macAddress().c_str());
    DBG_PRINTF("CAM%d HUB  MAC: %s\n", CAM_ID, macToString(HUB_MAC).c_str());
    DBG_PRINTF("CAM%d channel : %u\n", CAM_ID, ESPNOW_CHANNEL);

    return true;
}

static bool initCamera() {
    camera_config_t config{};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
#else
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
#endif

    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

#ifdef DEBUG_FORCE_QVGA
    config.frame_size = FRAMESIZE_QVGA;
#else
    config.frame_size = psramFound() ? FRAMESIZE_VGA : FRAMESIZE_QVGA;
#endif

#ifdef DEBUG_JPEG_QUALITY
    config.jpeg_quality = DEBUG_JPEG_QUALITY;
#else
    config.jpeg_quality = psramFound() ? 12 : 15;
#endif

    config.fb_count = psramFound() ? 2 : 1;

    if (esp_camera_init(&config) != ESP_OK) {
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_brightness(s, 0);
        s->set_contrast(s, 0);
        s->set_saturation(s, 0);
    }

    return true;
}

static bool sendWithRetry(const uint8_t *data, size_t len, uint8_t retries = 4) {
    for (uint8_t attempt = 1; attempt <= retries; attempt++) {
        g_sendDone = false;
        g_sendStatus = ESP_NOW_SEND_FAIL;

        if (esp_now_send(HUB_MAC, data, len) != ESP_OK) {
            delay(8);
            continue;
        }

        const uint32_t t0 = millis();
        while (!g_sendDone && (millis() - t0 < 150)) {
            delay(1);
        }

        if (g_sendDone && g_sendStatus == ESP_NOW_SEND_SUCCESS) {
            return true;
        }
        delay(6);
    }
    return false;
}

static bool sendFrameChunks(const uint8_t *jpg, size_t jpgLen) {
    static uint8_t txBuf[sizeof(CameraChunkHeader) + CHUNK_DATA_SIZE];

    g_frameId++;
    uint32_t offset = 0;
    uint32_t chunks = 0;

    while (offset < jpgLen) {
        const size_t remain = jpgLen - offset;
        const uint16_t chunkLen = (remain > CHUNK_DATA_SIZE)
            ? CHUNK_DATA_SIZE
            : (uint16_t)remain;

        CameraChunkHeader hdr{};
        hdr.magic = PROTO_MAGIC;
        hdr.version = PROTO_VERSION;
        hdr.camId = CAM_ID;
        hdr.flags = 0;
        if (offset == 0) {
            hdr.flags |= FLAG_START;
        }
        if (offset + chunkLen >= jpgLen) {
            hdr.flags |= FLAG_END;
        }
        hdr.frameId = g_frameId;
        hdr.totalSize = (uint32_t)jpgLen;
        hdr.offset = offset;
        hdr.chunkLen = chunkLen;
        hdr.chunkCrc = crc16_ccitt(jpg + offset, chunkLen);

        memcpy(txBuf, &hdr, sizeof(hdr));
        memcpy(txBuf + sizeof(hdr), jpg + offset, chunkLen);

        const size_t pktLen = sizeof(hdr) + chunkLen;
        if (!sendWithRetry(txBuf, pktLen)) {
            DBG_PRINTF("CAM%d frame=%lu FAIL offset=%lu\n",
                       CAM_ID,
                       (unsigned long)g_frameId,
                       (unsigned long)offset);
            return false;
        }

        offset += chunkLen;
        chunks++;
        delay(2);
    }

    DBG_PRINTF("CAM%d frame=%lu sent size=%u chunks=%lu\n",
               CAM_ID,
               (unsigned long)g_frameId,
               (unsigned)jpgLen,
               (unsigned long)chunks);
    return true;
}

static void captureAndSend() {
    DBG_PRINTF("CAM%d capture start\n", CAM_ID);

    if (PRELIGHT_MS > 0) {
        digitalWrite(FLASH_LED_PIN, HIGH);
        delay(PRELIGHT_MS);
    }

    camera_fb_t *fb = esp_camera_fb_get();

    digitalWrite(FLASH_LED_PIN, LOW);

    if (!fb) {
        DBG_PRINTF("CAM%d ERROR: fb == nullptr\n", CAM_ID);
        return;
    }

    DBG_PRINTF("CAM%d jpeg size=%u\n", CAM_ID, (unsigned)fb->len);
    const bool ok = sendFrameChunks(fb->buf, fb->len);
    DBG_PRINTF("CAM%d send result=%s\n", CAM_ID, ok ? "OK" : "FAIL");

    esp_camera_fb_return(fb);
}

void setup() {
    DBG_INIT();
    DBG_PRINTF("\nCAM%d boot\n", CAM_ID);

    pinMode(FLASH_LED_PIN, OUTPUT);
    digitalWrite(FLASH_LED_PIN, LOW);

    DBG_PRINTLN("initCamera...");
    if (!initCamera()) {
        DBG_PRINTLN("initCamera FAIL");
        while (true) {
            delay(1000);
        }
    }
    DBG_PRINTLN("initCamera OK");

    DBG_PRINTLN("initEspNow...");
    if (!initEspNow()) {
        DBG_PRINTLN("initEspNow FAIL");
        while (true) {
            delay(1000);
        }
    }
    DBG_PRINTLN("initEspNow OK");

    g_nextShotAt = millis() + 3000;
    DBG_PRINTF("first shot in 3s, interval=%lu ms\n",
               (unsigned long)CAPTURE_INTERVAL_MS);
}

void loop() {
    const uint32_t now = millis();
    if ((int32_t)(now - g_nextShotAt) >= 0) {
        g_nextShotAt += CAPTURE_INTERVAL_MS;
        captureAndSend();
    }
    delay(10);
}