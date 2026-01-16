#ifndef MODBUS_SLAVE_H
#define MODBUS_SLAVE_H

#include <Arduino.h>
#include <ModbusRTU.h>

// Карта регистров Modbus
enum class ModbusRegister : uint16_t {
    MILLIS_LOW   = 0,   // millis() - младшие 16 бит [READ]
    MILLIS_HIGH  = 1,   // millis() - старшие 16 бит [READ]
    TEMPERATURE  = 2,   // Температура чипа × 10 [READ]
    CPU_FREQ     = 3,   // Частота CPU в МГц [READ]
    USER_VALUE   = 5,   // Пользовательское значение [READ/WRITE]

    // Служебные константы
    COUNT        = 6    // Общее количество регистров
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
    ~ModbusSlave() = default;

    void begin(const ModbusConfig& config);
    void begin(uint8_t slaveId, uint32_t baudRate, int8_t rxPin, int8_t txPin, int8_t deRePin);

    // Основной цикл обработки
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

    uint16_t _userValue;
    bool _userValueUpdated;

    void setupRegisters();
    float readInternalTemperature();

    static ModbusSlave* _instance;
    static uint16_t onPreRead(TRegister* reg, uint16_t val);
    static uint16_t onUserValueWrite(TRegister* reg, uint16_t val);
};

#endif // MODBUS_SLAVE_H