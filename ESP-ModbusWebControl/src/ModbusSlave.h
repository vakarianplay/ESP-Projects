#ifndef MODBUS_SLAVE_H
#define MODBUS_SLAVE_H

#include <Arduino.h>
#include <ModbusRTU.h>

// Карта регистров Modbus
enum class ModbusRegister : uint16_t {
    MILLIS_LOW   = 0,
    MILLIS_HIGH  = 1,
    TEMPERATURE  = 2,
    CPU_FREQ     = 3,
    USER_VALUE   = 5,
    COUNT        = 6
};

// Конфигурация Modbus Slave
struct ModbusConfig {
    uint8_t slaveId   = 1;
    uint32_t baudRate = 9600;
    int8_t rxPin      = 16;
    int8_t txPin      = 17;
    int8_t deRePin    = 18;
};

class ModbusSlave {
public:
    ModbusSlave();
    ~ModbusSlave();

    void begin(const ModbusConfig& config);
    void begin(uint8_t slaveId, uint32_t baudRate, int8_t rxPin, int8_t txPin, int8_t deRePin);

    void update();

    uint16_t getRegister(ModbusRegister reg);
    uint32_t getMillis();
    float getTemperature();
    uint16_t getCpuFrequency();
    uint16_t getUserValue() const;

    void setUserValue(uint16_t value);
    bool isUserValueUpdated();

    void printDebugInfo(Stream& serial);
    String getDebugInfoJson();

private:
    ModbusRTU _mb;
    HardwareSerial _serial;
    ModbusConfig _config;

    volatile uint16_t _userValue;      // volatile для callback
    volatile bool _userValueUpdated;   // volatile для callback

    bool _initialized;

    void setupRegisters();
    float readInternalTemperature();

    // Статический указатель на экземпляр
    static ModbusSlave* _instance;

    static uint16_t onPreRead(TRegister* reg, uint16_t val);
    static uint16_t onUserValueWrite(TRegister* reg, uint16_t val);
};

#endif