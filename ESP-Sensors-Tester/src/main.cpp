#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

#include "index.h"

// ============================================================
// Поддержка платформ
// ============================================================

#if defined(ESP32)

    #include <WiFi.h>
    #include <WebServer.h>
    #include <Update.h>

    #if defined(BOARD_ESP32_WROOM32)

        constexpr char PLATFORM_NAME[] = "ESP32-WROOM-32";

        constexpr uint8_t DEFAULT_SDA = 21;
        constexpr uint8_t DEFAULT_SCL = 22;
        constexpr uint8_t DEFAULT_ONEWIRE = 4;
        constexpr uint8_t DEFAULT_DHT = 5;

        constexpr uint8_t ALLOWED_PINS[] = {
            4, 5, 13, 14,
            16, 17, 18, 19,
            21, 22, 23,
            25, 26, 27,
            32, 33
        };

    #elif defined(BOARD_ESP32_C3_SUPERMINI)

        constexpr char PLATFORM_NAME[] = "ESP32-C3 Super Mini";

        // GPIO8/9 на некоторых Super Mini связаны
        // со светодиодом и кнопкой BOOT.
        constexpr uint8_t DEFAULT_SDA = 6;
        constexpr uint8_t DEFAULT_SCL = 7;
        constexpr uint8_t DEFAULT_ONEWIRE = 4;
        constexpr uint8_t DEFAULT_DHT = 5;

        constexpr uint8_t ALLOWED_PINS[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
        };

    #elif defined(BOARD_ESP32_S3)

        constexpr char PLATFORM_NAME[] = "ESP32-S3";

        constexpr uint8_t DEFAULT_SDA = 8;
        constexpr uint8_t DEFAULT_SCL = 9;
        constexpr uint8_t DEFAULT_ONEWIRE = 4;
        constexpr uint8_t DEFAULT_DHT = 5;

        // Наличие выводов зависит от конкретной платы.
        // GPIO19/20 не включены: используются встроенным USB.
        constexpr uint8_t ALLOWED_PINS[] = {
            1, 2, 4, 5, 6, 7, 8, 9, 10,
            11, 12, 13, 14, 15, 16, 17, 18,
            21, 38, 39, 40, 41, 42, 47, 48
        };

    #elif defined(BOARD_ESP32_S2)

        constexpr char PLATFORM_NAME[] = "ESP32-S2";

        constexpr uint8_t DEFAULT_SDA = 8;
        constexpr uint8_t DEFAULT_SCL = 10;
        constexpr uint8_t DEFAULT_ONEWIRE = 4;
        constexpr uint8_t DEFAULT_DHT = 5;

        // GPIO19/20 не включены: используются USB.
        constexpr uint8_t ALLOWED_PINS[] = {
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            11, 12, 13, 14, 15, 16, 17, 18,
            21, 33, 34, 35, 36, 37, 38, 39, 40
        };

    #elif defined(BOARD_ESP32_C6)

        constexpr char PLATFORM_NAME[] = "ESP32-C6";

        constexpr uint8_t DEFAULT_SDA = 6;
        constexpr uint8_t DEFAULT_SCL = 7;
        constexpr uint8_t DEFAULT_ONEWIRE = 4;
        constexpr uint8_t DEFAULT_DHT = 5;

        // GPIO12/13 не включены: USB.
        constexpr uint8_t ALLOWED_PINS[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            11, 14, 16, 17, 18, 19, 20, 21, 22, 23
        };

    #else
        #error "Неизвестный вариант ESP32"
    #endif

    WebServer server(80);

#elif defined(ESP8266)

    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
    #include <Updater.h>

    constexpr char PLATFORM_NAME[] = "ESP8266";

    constexpr uint8_t DEFAULT_SDA = 4;
    constexpr uint8_t DEFAULT_SCL = 5;
    constexpr uint8_t DEFAULT_ONEWIRE = 13;
    constexpr uint8_t DEFAULT_DHT = 14;

    // GPIO0 и GPIO2 являются загрузочными выводами.
    // Используйте их только с учётом внешних подтяжек.
    constexpr uint8_t ALLOWED_PINS[] = {
        0, 2, 4, 5, 12, 13, 14
    };

    ESP8266WebServer server(80);

#else
    #error "Поддерживаются только ESP32 и ESP8266"
#endif

constexpr size_t ALLOWED_PINS_COUNT =
    sizeof(ALLOWED_PINS) / sizeof(ALLOWED_PINS[0]);

// ============================================================
// Настройки
// ============================================================

constexpr char DEFAULT_AP_SSID[] = "ESP Connect";
constexpr uint32_t I2C_FREQUENCY = 100000;

#if defined(ESP32)
constexpr uint32_t I2C_TIMEOUT_MS = 50;
#endif

struct AppConfig {
    uint8_t sda;
    uint8_t scl;
    uint8_t oneWirePin;
    uint8_t dhtPin;
    uint8_t dhtType;
    String ssid;
};

AppConfig config;

bool eepromReady = false;
bool i2cStarted = false;
bool accessPointStarted = false;

Adafruit_BME280 bme;
Adafruit_BMP280 bmp;

// ============================================================
// EEPROM
// ============================================================

constexpr int EEPROM_SIZE = 64;
constexpr int CONFIG_DATA_SIZE = 48;
constexpr int CONFIG_CHECKSUM_ADDR = 48;
constexpr int CONFIG_SSID_ADDR = 16;
constexpr int CONFIG_SSID_MAX = 32;

constexpr uint8_t CONFIG_VERSION = 2;

void setDefaultConfig() {
    config.sda = DEFAULT_SDA;
    config.scl = DEFAULT_SCL;
    config.oneWirePin = DEFAULT_ONEWIRE;
    config.dhtPin = DEFAULT_DHT;
    config.dhtType = 22;
    config.ssid = DEFAULT_AP_SSID;
}

bool isAllowedPin(int pin) {
    for (size_t i = 0; i < ALLOWED_PINS_COUNT; i++) {
        if (ALLOWED_PINS[i] == pin) {
            return true;
        }
    }

    return false;
}

bool isValidConfig(const AppConfig &value) {
    if (!isAllowedPin(value.sda) ||
        !isAllowedPin(value.scl) ||
        !isAllowedPin(value.oneWirePin) ||
        !isAllowedPin(value.dhtPin)) {
        return false;
    }

    if (value.sda == value.scl) {
        return false;
    }

    if (value.dhtType != 11 && value.dhtType != 22) {
        return false;
    }

    if (value.ssid.length() == 0 ||
        value.ssid.length() > CONFIG_SSID_MAX) {
        return false;
    }

    return true;
}

uint8_t configChecksum(const uint8_t *data) {
    uint8_t result = 0x5A;

    for (int i = 0; i < CONFIG_DATA_SIZE; i++) {
        result = static_cast<uint8_t>(
            (result ^ data[i]) + 0x3D
        );
    }

    return result;
}

bool initializeEeprom() {
#if defined(ESP32)
    return EEPROM.begin(EEPROM_SIZE);
#else
    EEPROM.begin(EEPROM_SIZE);
    return true;
#endif
}

void loadConfig() {
    setDefaultConfig();

    eepromReady = initializeEeprom();

    if (!eepromReady) {
        Serial.println("EEPROM: ошибка инициализации");
        return;
    }

    uint8_t data[CONFIG_DATA_SIZE + 1];

    for (int i = 0; i <= CONFIG_DATA_SIZE; i++) {
        data[i] = EEPROM.read(i);
    }

    const bool magicValid =
        data[0] == 'B' &&
        data[1] == 'M' &&
        data[2] == 'D' &&
        data[3] == '2';

    const bool checksumValid =
        data[CONFIG_CHECKSUM_ADDR] == configChecksum(data);

    if (!magicValid ||
        data[4] != CONFIG_VERSION ||
        !checksumValid) {

        Serial.println(
            "EEPROM: нет корректной конфигурации, используются значения по умолчанию"
        );
        return;
    }

    const uint8_t ssidLength = data[10];

    if (ssidLength == 0 || ssidLength > CONFIG_SSID_MAX) {
        Serial.println("EEPROM: некорректная длина SSID");
        return;
    }

    AppConfig loaded;

    loaded.sda = data[5];
    loaded.scl = data[6];
    loaded.oneWirePin = data[7];
    loaded.dhtPin = data[8];
    loaded.dhtType = data[9];

    loaded.ssid = "";

    for (uint8_t i = 0; i < ssidLength; i++) {
        loaded.ssid += static_cast<char>(
            data[CONFIG_SSID_ADDR + i]
        );
    }

    if (!isValidConfig(loaded)) {
        Serial.println("EEPROM: конфигурация не прошла проверку");
        return;
    }

    config = loaded;

    Serial.println("EEPROM: конфигурация загружена");
}

bool saveConfig() {
    if (!isValidConfig(config)) {
        return false;
    }

    if (!eepromReady) {
        eepromReady = initializeEeprom();
    }

    if (!eepromReady) {
        return false;
    }

    uint8_t data[CONFIG_DATA_SIZE + 1];
    memset(data, 0, sizeof(data));

    data[0] = 'B';
    data[1] = 'M';
    data[2] = 'D';
    data[3] = '2';
    data[4] = CONFIG_VERSION;

    data[5] = config.sda;
    data[6] = config.scl;
    data[7] = config.oneWirePin;
    data[8] = config.dhtPin;
    data[9] = config.dhtType;
    data[10] = static_cast<uint8_t>(config.ssid.length());

    memcpy(
        &data[CONFIG_SSID_ADDR],
        config.ssid.c_str(),
        config.ssid.length()
    );

    data[CONFIG_CHECKSUM_ADDR] = configChecksum(data);

    for (int i = 0; i <= CONFIG_DATA_SIZE; i++) {
        EEPROM.write(i, data[i]);
    }

    const bool result = EEPROM.commit();

    Serial.printf(
        "EEPROM.commit(): %s\n",
        result ? "OK" : "FAIL"
    );

    return result;
}

// ============================================================
// Вспомогательные функции
// ============================================================

String hexByte(uint8_t value) {
    char buffer[5];
    snprintf(buffer, sizeof(buffer), "0x%02X", value);
    return String(buffer);
}

String jsonEscape(const String &value) {
    String result;
    result.reserve(value.length() + 8);

    for (size_t i = 0; i < value.length(); i++) {
        const char c = value[i];

        switch (c) {
            case '"':
                result += "\\\"";
                break;
            case '\\':
                result += "\\\\";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\t':
                result += "\\t";
                break;
            default:
                if (static_cast<uint8_t>(c) >= 0x20) {
                    result += c;
                }
                break;
        }
    }

    return result;
}

void sendJson(int statusCode, const String &json) {
    server.sendHeader("Cache-Control", "no-store");
    server.send(
        statusCode,
        "application/json; charset=utf-8",
        json
    );
}

void sendError(int statusCode, const String &message) {
    String json = "{\"error\":\"";
    json += jsonEscape(message);
    json += "\"}";

    sendJson(statusCode, json);

    Serial.print("HTTP error ");
    Serial.print(statusCode);
    Serial.print(": ");
    Serial.println(message);
}

bool parsePin(const String &text, int &pin) {
    if (text.length() == 0) {
        return false;
    }

    char *end = nullptr;
    const long value = strtol(text.c_str(), &end, 10);

    if (end == text.c_str() ||
        *end != '\0' ||
        value < 0 ||
        value > 255) {
        return false;
    }

    pin = static_cast<int>(value);
    return true;
}

String makeStateJson() {
    String json;
    json.reserve(700);

    json = "{\"platform\":\"";
    json += PLATFORM_NAME;

    json += "\",\"ssid\":\"";
    json += jsonEscape(config.ssid);

    json += "\",\"apIp\":\"";
    json += WiFi.softAPIP().toString();

    json += "\",\"apStarted\":";
    json += accessPointStarted ? "true" : "false";

    json += ",\"eepromReady\":";
    json += eepromReady ? "true" : "false";

    json += ",\"sda\":";
    json += config.sda;

    json += ",\"scl\":";
    json += config.scl;

    json += ",\"oneWirePin\":";
    json += config.oneWirePin;

    json += ",\"dhtPin\":";
    json += config.dhtPin;

    json += ",\"dhtType\":";
    json += config.dhtType;

    json += ",\"allowedPins\":[";

    for (size_t i = 0; i < ALLOWED_PINS_COUNT; i++) {
        if (i > 0) {
            json += ",";
        }

        json += ALLOWED_PINS[i];
    }

    json += "]}";

    return json;
}

// ============================================================
// Wi-Fi
// ============================================================

bool startAccessPoint() {
    Serial.print("Запуск точки доступа: ");
    Serial.println(config.ssid);

    WiFi.persistent(false);
    WiFi.mode(WIFI_AP);

#if defined(ESP32)
    WiFi.setSleep(false);
#else
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
#endif

    const IPAddress ip(192, 168, 4, 1);
    const IPAddress gateway(192, 168, 4, 1);
    const IPAddress subnet(255, 255, 255, 0);

    const bool configResult =
        WiFi.softAPConfig(ip, gateway, subnet);

    const bool apResult = WiFi.softAP(
        config.ssid.c_str(),
        nullptr,
        1,
        false,
        4
    );

    delay(500);

    Serial.printf(
        "softAPConfig: %s\n",
        configResult ? "OK" : "FAIL"
    );

    Serial.printf(
        "softAP: %s\n",
        apResult ? "OK" : "FAIL"
    );

    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

    return configResult && apResult;
}

// ============================================================
// I2C
// ============================================================

void stopI2c() {
#if defined(ESP32)
    if (i2cStarted) {
        Wire.end();
    }
#endif

    i2cStarted = false;
}

bool ensureI2cStarted() {
    if (i2cStarted) {
        return true;
    }

    Serial.printf(
        "I2C: SDA=GPIO%u, SCL=GPIO%u\n",
        config.sda,
        config.scl
    );

#if defined(ESP32)

    const bool result = Wire.begin(
        config.sda,
        config.scl,
        I2C_FREQUENCY
    );

    if (!result) {
        return false;
    }

    Wire.setTimeOut(I2C_TIMEOUT_MS);

#else

    Wire.begin(config.sda, config.scl);
    Wire.setClock(I2C_FREQUENCY);
    Wire.setClockStretchLimit(150000);

#endif

    i2cStarted = true;
    return true;
}

size_t requestFromWire(uint8_t address, uint8_t quantity) {
#if defined(ESP32)
    return Wire.requestFrom(
        address,
        static_cast<size_t>(quantity)
    );
#else
    return Wire.requestFrom(address, quantity);
#endif
}

bool deviceAcknowledges(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

bool readChipId(uint8_t address, uint8_t &chipId) {
    Wire.beginTransmission(address);
    Wire.write(static_cast<uint8_t>(0xD0));

    if (Wire.endTransmission(false) != 0) {
        return false;
    }

    if (requestFromWire(address, 1) != 1 ||
        !Wire.available()) {
        return false;
    }

    chipId = static_cast<uint8_t>(Wire.read());
    return true;
}

// ============================================================
// Низкоуровневая поддержка SHT3x/SHT4x
// ============================================================

uint8_t shtCrc(const uint8_t *data) {
    uint8_t crc = 0xFF;

    for (uint8_t i = 0; i < 2; i++) {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80)
                ? static_cast<uint8_t>((crc << 1) ^ 0x31)
                : static_cast<uint8_t>(crc << 1);
        }
    }

    return crc;
}

bool shtReadSix(uint8_t address, uint8_t *data) {
    if (requestFromWire(address, 6) != 6) {
        while (Wire.available()) {
            Wire.read();
        }
        return false;
    }

    for (uint8_t i = 0; i < 6; i++) {
        if (!Wire.available()) {
            return false;
        }

        data[i] = static_cast<uint8_t>(Wire.read());
    }

    return
        shtCrc(&data[0]) == data[2] &&
        shtCrc(&data[3]) == data[5];
}

bool shtWriteTwoByteCommand(
    uint8_t address,
    uint8_t first,
    uint8_t second
) {
    Wire.beginTransmission(address);
    Wire.write(first);
    Wire.write(second);

    return Wire.endTransmission() == 0;
}

bool sht4xHasValidSerial(uint8_t address) {
    Wire.beginTransmission(address);
    Wire.write(static_cast<uint8_t>(0x89));

    if (Wire.endTransmission() != 0) {
        return false;
    }

    delay(2);

    uint8_t data[6];
    return shtReadSix(address, data);
}

bool sht4xRead(
    uint8_t address,
    float &temperature,
    float &humidity
) {
    // High precision measurement.
    if (!shtWriteTwoByteCommand(address, 0xFD, 0x00)) {
        // Для однобайтовых команд SHT4x нужен отдельный вариант.
        Wire.beginTransmission(address);
        Wire.write(static_cast<uint8_t>(0xFD));

        if (Wire.endTransmission() != 0) {
            return false;
        }
    }

    delay(12);

    uint8_t data[6];

    if (!shtReadSix(address, data)) {
        return false;
    }

    const uint16_t rawTemperature =
        (static_cast<uint16_t>(data[0]) << 8) | data[1];

    const uint16_t rawHumidity =
        (static_cast<uint16_t>(data[3]) << 8) | data[4];

    temperature =
        -45.0F + 175.0F *
        static_cast<float>(rawTemperature) / 65535.0F;

    humidity =
        -6.0F + 125.0F *
        static_cast<float>(rawHumidity) / 65535.0F;

    humidity = constrain(humidity, 0.0F, 100.0F);

    return true;
}

bool sht3xRead(
    uint8_t address,
    float &temperature,
    float &humidity
) {
    // High repeatability, clock stretching disabled.
    if (!shtWriteTwoByteCommand(address, 0x24, 0x00)) {
        return false;
    }

    delay(20);

    uint8_t data[6];

    if (!shtReadSix(address, data)) {
        return false;
    }

    const uint16_t rawTemperature =
        (static_cast<uint16_t>(data[0]) << 8) | data[1];

    const uint16_t rawHumidity =
        (static_cast<uint16_t>(data[3]) << 8) | data[4];

    temperature =
        -45.0F + 175.0F *
        static_cast<float>(rawTemperature) / 65535.0F;

    humidity =
        100.0F *
        static_cast<float>(rawHumidity) / 65535.0F;

    return true;
}

// ============================================================
// HTTP: сохранение настроек
// ============================================================

void handleSaveSsid() {
    if (!server.hasArg("ssid")) {
        sendError(400, "Не передан параметр ssid");
        return;
    }

    String ssid = server.arg("ssid");
    ssid.trim();

    if (ssid.length() == 0 ||
        ssid.length() > CONFIG_SSID_MAX) {
        sendError(
            400,
            "SSID должен содержать от 1 до 32 байт"
        );
        return;
    }

    for (size_t i = 0; i < ssid.length(); i++) {
        const uint8_t c =
            static_cast<uint8_t>(ssid[i]);

        if (c < 0x20 || c == 0x7F) {
            sendError(
                400,
                "SSID содержит недопустимые управляющие символы"
            );
            return;
        }
    }

    const String oldSsid = config.ssid;
    config.ssid = ssid;

    if (!saveConfig()) {
        config.ssid = oldSsid;
        sendError(500, "Не удалось сохранить SSID");
        return;
    }

    Serial.print("Новый SSID сохранён: ");
    Serial.println(config.ssid);

    sendJson(200, makeStateJson());
}

void handleSaveConfig() {
    const char *arguments[] = {
        "sda",
        "scl",
        "oneWirePin",
        "dhtPin",
        "dhtType"
    };

    for (const char *argument : arguments) {
        if (!server.hasArg(argument)) {
            sendError(
                400,
                String("Не передан параметр ") + argument
            );
            return;
        }
    }

    int sda;
    int scl;
    int oneWirePin;
    int dhtPin;
    int dhtType;

    if (!parsePin(server.arg("sda"), sda) ||
        !parsePin(server.arg("scl"), scl) ||
        !parsePin(server.arg("oneWirePin"), oneWirePin) ||
        !parsePin(server.arg("dhtPin"), dhtPin) ||
        !parsePin(server.arg("dhtType"), dhtType)) {
        sendError(400, "Некорректные числовые параметры");
        return;
    }

    AppConfig updated = config;

    updated.sda = static_cast<uint8_t>(sda);
    updated.scl = static_cast<uint8_t>(scl);
    updated.oneWirePin = static_cast<uint8_t>(oneWirePin);
    updated.dhtPin = static_cast<uint8_t>(dhtPin);
    updated.dhtType = static_cast<uint8_t>(dhtType);

    if (!isValidConfig(updated)) {
        sendError(
            400,
            "Проверьте GPIO, различие SDA/SCL и тип DHT"
        );
        return;
    }

    const AppConfig previous = config;
    config = updated;

    if (!saveConfig()) {
        config = previous;
        sendError(500, "Не удалось записать настройки");
        return;
    }

    stopI2c();
    sendJson(200, makeStateJson());
}

void handleReboot() {
    sendJson(200, "{\"ok\":true}");
    Serial.println("Перезагрузка по команде пользователя");

    delay(400);
    ESP.restart();
}

// ============================================================
// HTTP: I2C-сканер
// ============================================================

void handleI2cScan() {
    if (!ensureI2cStarted()) {
        sendError(500, "Не удалось запустить I2C");
        return;
    }

    const uint32_t startedAt = millis();

    String json;
    json.reserve(1700);
    json = "{\"devices\":[";

    uint8_t found = 0;
    uint16_t busErrors = 0;

    for (uint8_t address = 0x03;
         address <= 0x77;
         address++) {

        Wire.beginTransmission(address);
        const uint8_t result = Wire.endTransmission();

        if (result == 0) {
            if (found > 0) {
                json += ",";
            }

            json += "{\"address\":\"";
            json += hexByte(address);

            json += "\",\"decimal\":";
            json += address;

            json += ",\"note\":\"";

            if (address == 0x76 || address == 0x77) {
                json += "Возможно BME280/BMP280";
            } else if (address == 0x44 || address == 0x45) {
                json += "Возможно SHT30/SHT31/SHT40";
            } else if (address == 0x3C || address == 0x3D) {
                json += "Часто используется OLED";
            }

            json += "\"}";

            found++;

            Serial.printf(
                "I2C device: 0x%02X\n",
                address
            );
        } else if (result != 2) {
            busErrors++;
        }

        delay(1);
    }

    json += "],\"count\":";
    json += found;

    json += ",\"busErrors\":";
    json += busErrors;

    json += ",\"sda\":";
    json += config.sda;

    json += ",\"scl\":";
    json += config.scl;

    json += ",\"durationMs\":";
    json += millis() - startedAt;

    json += "}";

    sendJson(200, json);
}

// ============================================================
// Автоматическая диагностика I2C-датчиков
// ============================================================

void appendSensorJson(
    String &json,
    uint8_t &count,
    const char *name,
    uint8_t address,
    float temperature,
    bool hasPressure,
    float pressure,
    bool hasHumidity,
    float humidity
) {
    if (count > 0) {
        json += ",";
    }

    json += "{\"name\":\"";
    json += name;

    json += "\",\"address\":\"";
    json += hexByte(address);

    json += "\",\"temperature\":";

    if (isnan(temperature)) {
        json += "null";
    } else {
        json += String(temperature, 2);
    }

    json += ",\"pressure\":";

    if (hasPressure && !isnan(pressure)) {
        json += String(pressure, 2);
    } else {
        json += "null";
    }

    json += ",\"humidity\":";

    if (hasHumidity && !isnan(humidity)) {
        json += String(humidity, 2);
    } else {
        json += "null";
    }

    json += "}";
    count++;
}

void handleI2cSensors() {
    if (!ensureI2cStarted()) {
        sendError(500, "Не удалось запустить I2C");
        return;
    }

    String sensors;
    sensors.reserve(1200);

    uint8_t count = 0;

    // BME280/BMP280.
    const uint8_t boschAddresses[] = {0x76, 0x77};

    for (uint8_t address : boschAddresses) {
        if (!deviceAcknowledges(address)) {
            continue;
        }

        uint8_t chipId = 0;

        if (!readChipId(address, chipId)) {
            continue;
        }

        Serial.printf(
            "Bosch address=0x%02X, chipId=0x%02X\n",
            address,
            chipId
        );

        if (chipId == 0x60) {
            if (bme.begin(address, &Wire)) {
                delay(10);

                const float temperature =
                    bme.readTemperature();

                const float pressure =
                    bme.readPressure() / 100.0F;

                const float humidity =
                    bme.readHumidity();

                if (!isnan(temperature) &&
                    !isnan(pressure) &&
                    !isnan(humidity)) {

                    appendSensorJson(
                        sensors,
                        count,
                        "BME280",
                        address,
                        temperature,
                        true,
                        pressure,
                        true,
                        humidity
                    );
                }
            }
        } else if (
            chipId == 0x56 ||
            chipId == 0x57 ||
            chipId == 0x58
        ) {
            if (bmp.begin(address)) {
                delay(10);

                const float temperature =
                    bmp.readTemperature();

                const float pressure =
                    bmp.readPressure() / 100.0F;

                if (!isnan(temperature) && !isnan(pressure)) {
                    appendSensorJson(
                        sensors,
                        count,
                        "BMP280",
                        address,
                        temperature,
                        true,
                        pressure,
                        false,
                        NAN
                    );
                }
            }
        }
    }

    // SHT30/SHT31 и SHT40/SHT4x.
    const uint8_t shtAddresses[] = {0x44, 0x45};

    for (uint8_t address : shtAddresses) {
        if (!deviceAcknowledges(address)) {
            continue;
        }

        float temperature = NAN;
        float humidity = NAN;

        if (sht4xHasValidSerial(address)) {
            if (sht4xRead(address, temperature, humidity)) {
                appendSensorJson(
                    sensors,
                    count,
                    "SHT40 / SHT4x",
                    address,
                    temperature,
                    false,
                    NAN,
                    true,
                    humidity
                );
            }
        } else if (sht3xRead(address, temperature, humidity)) {
            // У SHT30 и SHT31 нет надёжного общего chip-id.
            // Поэтому семейство отображается совместно.
            appendSensorJson(
                sensors,
                count,
                "SHT30 / SHT31",
                address,
                temperature,
                false,
                NAN,
                true,
                humidity
            );
        }
    }

    String json;
    json.reserve(sensors.length() + 400);

    json = "{\"sensors\":[";
    json += sensors;

    json += "],\"count\":";
    json += count;

    json += ",\"sda\":";
    json += config.sda;

    json += ",\"scl\":";
    json += config.scl;

    json += ",\"message\":\"";

    if (count > 0) {
        json += "Определено и прочитано датчиков: ";
        json += count;
        json += ".";
    } else {
        json +=
            "Поддерживаемые датчики не найдены. "
            "Проверьте I2C-сканер, питание и подключение.";
    }

    json += "\"}";

    sendJson(200, json);
}

// ============================================================
// DS18B20
// ============================================================

String dsRomToString(const uint8_t *address) {
    String result;

    for (uint8_t i = 0; i < 8; i++) {
        if (i > 0) {
            result += ":";
        }

        char buffer[3];
        snprintf(buffer, sizeof(buffer), "%02X", address[i]);
        result += buffer;
    }

    return result;
}

void handleDs18b20() {
    OneWire oneWire(config.oneWirePin);
    DallasTemperature sensors(&oneWire);

    sensors.begin();
    sensors.setResolution(12);

    uint8_t deviceCount = sensors.getDeviceCount();

    if (deviceCount > 8) {
        deviceCount = 8;
    }

    String devices;
    devices.reserve(600);

    uint8_t validCount = 0;

    if (deviceCount > 0) {
        sensors.requestTemperatures();
    }

    for (uint8_t i = 0; i < deviceCount; i++) {
        DeviceAddress address;

        if (!sensors.getAddress(address, i)) {
            continue;
        }

        const float temperature =
            sensors.getTempC(address);

        if (temperature == DEVICE_DISCONNECTED_C ||
            temperature < -55.0F ||
            temperature > 125.0F) {
            continue;
        }

        if (validCount > 0) {
            devices += ",";
        }

        devices += "{\"rom\":\"";
        devices += dsRomToString(address);

        devices += "\",\"temperature\":";
        devices += String(temperature, 3);
        devices += "}";

        validCount++;
    }

    String json;
    json.reserve(devices.length() + 350);

    json = "{\"pin\":";
    json += config.oneWirePin;

    json += ",\"count\":";
    json += validCount;

    json += ",\"devices\":[";
    json += devices;

    json += "],\"message\":\"";

    if (validCount > 0) {
        json += "Прочитано DS18B20: ";
        json += validCount;
        json += ".";
    } else if (deviceCount > 0) {
        json +=
            "Устройства 1-Wire найдены, "
            "но корректную температуру получить не удалось.";
    } else {
        json +=
            "DS18B20 не найден. Проверьте DATA, GND, питание "
            "и подтяжку 4,7 кОм.";
    }

    json += "\"}";

    sendJson(200, json);
}

// ============================================================
// DHT11/DHT22
// ============================================================

void handleDht() {
    DHT dht(config.dhtPin, config.dhtType);

    dht.begin();

    // DHT22 нельзя опрашивать слишком часто.
    delay(2200);

    const float humidity = dht.readHumidity();
    const float temperature = dht.readTemperature();

    const bool ok =
        !isnan(humidity) &&
        !isnan(temperature);

    String json;
    json.reserve(400);

    json = "{\"ok\":";
    json += ok ? "true" : "false";

    json += ",\"dhtType\":";
    json += config.dhtType;

    json += ",\"pin\":";
    json += config.dhtPin;

    json += ",\"temperature\":";

    if (ok) {
        json += String(temperature, 2);
    } else {
        json += "null";
    }

    json += ",\"humidity\":";

    if (ok) {
        json += String(humidity, 2);
    } else {
        json += "null";
    }

    json += ",\"message\":\"";

    if (ok) {
        json += "Датчик успешно прочитан.";
    } else {
        json +=
            "Не удалось прочитать DHT. Проверьте тип датчика, "
            "GPIO, питание и подтяжку DATA.";
    }

    json += "\"}";

    sendJson(200, json);
}

// ============================================================
// OTA: загрузка произвольного .bin
// Используется только встроенный Update/Updater.
// ============================================================

bool otaFailed = false;
bool otaSucceeded = false;
String otaError;
size_t otaWritten = 0;

void handleFirmwareUpload() {
    HTTPUpload &upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        otaFailed = false;
        otaSucceeded = false;
        otaError = "";
        otaWritten = 0;

        String filename = upload.filename;
        filename.toLowerCase();

        Serial.printf(
            "OTA start: %s\n",
            upload.filename.c_str()
        );

        if (!filename.endsWith(".bin")) {
            otaFailed = true;
            otaError = "Разрешены только файлы .bin";
            return;
        }

#if defined(ESP32)

        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            otaFailed = true;
            otaError = "Update.begin() failed";
            Update.printError(Serial);
            return;
        }

#else

        const uint32_t maxSketchSpace =
            (ESP.getFreeSketchSpace() - 0x1000) &
            0xFFFFF000;

        if (!Update.begin(maxSketchSpace)) {
            otaFailed = true;
            otaError = "Update.begin() failed";
            Update.printError(Serial);
            return;
        }

#endif

    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (otaFailed) {
            return;
        }

        const size_t written = Update.write(
            upload.buf,
            upload.currentSize
        );

        if (written != upload.currentSize) {
            otaFailed = true;
            otaError = "Ошибка записи во flash";
            Update.printError(Serial);
            return;
        }

        otaWritten += written;

        if ((otaWritten % 65536UL) < upload.currentSize) {
            Serial.printf(
                "OTA written: %u bytes\n",
                static_cast<unsigned>(otaWritten)
            );
        }

        yield();

    } else if (upload.status == UPLOAD_FILE_END) {
        if (otaFailed) {
            return;
        }

        if (otaWritten == 0) {
            otaFailed = true;
            otaError = "Загружен пустой файл";
            return;
        }

        if (!Update.end(true)) {
            otaFailed = true;
            otaError = "Update.end() failed";
            Update.printError(Serial);
            return;
        }

        if (!Update.isFinished()) {
            otaFailed = true;
            otaError = "Прошивка загружена не полностью";
            return;
        }

        otaSucceeded = true;

        Serial.printf(
            "OTA success: %u bytes\n",
            static_cast<unsigned>(otaWritten)
        );

    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        otaFailed = true;
        otaSucceeded = false;
        otaError = "Загрузка прервана";

        Serial.println("OTA aborted");
    }
}

void handleFirmwareFinished() {
    if (otaSucceeded && !otaFailed && !Update.hasError()) {
        sendJson(
            200,
            "{\"ok\":true,\"message\":\"Firmware updated\"}"
        );

        Serial.println("OTA: перезагрузка");

        delay(600);
        ESP.restart();
        return;
    }

    if (otaError.length() == 0) {
        otaError = "Неизвестная ошибка обновления";
    }

    sendError(500, otaError);
}

// ============================================================
// Arduino setup/loop
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println("ESP Sensor Diagnostic");
    Serial.print("Platform: ");
    Serial.println(PLATFORM_NAME);
    Serial.println("================================");

    setDefaultConfig();
    loadConfig();

    accessPointStarted = startAccessPoint();

    server.on("/", HTTP_GET, []() {
        server.send_P(
            200,
            "text/html; charset=utf-8",
            webpage
        );
    });

    server.on("/api/state", HTTP_GET, []() {
        sendJson(200, makeStateJson());
    });

    server.on("/api/ssid", HTTP_POST, handleSaveSsid);
    server.on("/api/config", HTTP_POST, handleSaveConfig);
    server.on("/api/reboot", HTTP_POST, handleReboot);

    server.on("/api/scan", HTTP_POST, handleI2cScan);

    server.on(
        "/api/i2c-sensors",
        HTTP_POST,
        handleI2cSensors
    );

    server.on(
        "/api/ds18b20",
        HTTP_POST,
        handleDs18b20
    );

    server.on(
        "/api/dht",
        HTTP_POST,
        handleDht
    );

    server.on(
        "/update",
        HTTP_POST,
        handleFirmwareFinished,
        handleFirmwareUpload
    );

    server.onNotFound([]() {
        server.send(
            404,
            "text/plain; charset=utf-8",
            "Not found"
        );
    });

    server.begin();

    Serial.println("HTTP server started");
    Serial.println("URL: http://192.168.4.1");
}

void loop() {
    server.handleClient();

    static uint32_t lastLog = 0;
    const uint32_t now = millis();

    if (now - lastLog >= 15000) {
        lastLog = now;

        Serial.print("ALIVE, AP=");
        Serial.print(config.ssid);
        Serial.print(", IP=");
        Serial.print(WiFi.softAPIP());
        Serial.print(", clients=");
        Serial.println(WiFi.softAPgetStationNum());
    }

    delay(2);
}