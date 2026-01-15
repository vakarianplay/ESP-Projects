#include "ModbusSlave.h"
#include "wifiConf.h"

ModbusSlave* modbus;
WifiModule* wifi;

const unsigned long DEBUG_INTERVAL = 5000;
unsigned long lastDebugTime = 0;

void setup() {
    Serial.begin(9600);
    Serial.println("\n========================================");
    Serial.println("   ESP32 Modbus RTU Slave");
    Serial.println("========================================");

    pinMode(36, INPUT);
    analogReadResolution(16);

    // Конфигурация Modbus
    ModbusConfig config;
    // config.slaveId = 1;
    // config.baudRate = 9600;
    // config.rxPin = 16;
    // config.txPin = 17;
    // config.deRePin = 18;

    modbus = new ModbusSlave();
    wifi = new WifiModule();

    modbus->begin(config);
    wifi->wifiStart("ESP Connect");

    // modbus.begin(1, 9600, 16, 17, 4);

    Serial.println("\nModbus Slave initialized successfully!");
    Serial.println("\nRegister Map:");
    Serial.println("  [0] MILLIS_LOW   - millis() low 16 bits  [READ]");
    Serial.println("  [1] MILLIS_HIGH  - millis() high 16 bits [READ]");
    Serial.println("  [2] TEMPERATURE  - Chip temp × 10        [READ]");
    Serial.println("  [3] CPU_FREQ     - CPU frequency MHz     [READ]");
    Serial.println("  [5] USER_VALUE   - User defined value    [READ/WRITE]");
    Serial.println("\n========================================\n");
}

void loop() {
    if (modbus != nullptr)
        modbus->update();

    if (wifi != nullptr && wifi->isWifiStart())
        server.handleClient();

    if (modbus->isUserValueUpdated()) {
        Serial.printf("[EVENT] User Value written: %u\n", modbus->getUserValue());
    }

    if (millis() - lastDebugTime >= DEBUG_INTERVAL) {
        lastDebugTime = millis();
        modbus->printDebugInfo(Serial);
    }
}