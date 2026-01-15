#include "ModbusSlave.h"

ModbusSlave* ModbusSlave::_instance = nullptr;

ModbusSlave::ModbusSlave() 
    : _serial(2)
    , _userValue(0)
    , _userValueUpdated(false) 
{
    _instance = this;
}

void ModbusSlave::begin(const ModbusConfig& config) {
    _config = config;

    // Инициализация UART для RS485
    _serial.begin(_config.baudRate, SERIAL_8N1, _config.rxPin, _config.txPin);

    // Инициализация Modbus RTU
    _mb.begin(&_serial, _config.deRePin);
    _mb.slave(_config.slaveId);

    // Настройка регистров
    setupRegisters();
}

void ModbusSlave::begin(uint8_t slaveId, uint32_t baudRate, int8_t rxPin, int8_t txPin, int8_t deRePin) {
    ModbusConfig config;
    config.slaveId = slaveId;
    config.baudRate = baudRate;
    config.rxPin = rxPin;
    config.txPin = txPin;
    config.deRePin = deRePin;

    begin(config);
}

void ModbusSlave::setupRegisters() {
    // Добавление Holding Registers
    _mb.addHreg(static_cast<uint16_t>(ModbusRegister::MILLIS_LOW), 0);
    _mb.addHreg(static_cast<uint16_t>(ModbusRegister::MILLIS_HIGH), 0);
    _mb.addHreg(static_cast<uint16_t>(ModbusRegister::TEMPERATURE), 0);
    _mb.addHreg(static_cast<uint16_t>(ModbusRegister::CPU_FREQ), 0);
    _mb.addHreg(static_cast<uint16_t>(ModbusRegister::USER_VALUE), 0);

    // Callbacks для чтения динамических регистров
    _mb.onGetHreg(static_cast<uint16_t>(ModbusRegister::MILLIS_LOW), onPreRead);
    _mb.onGetHreg(static_cast<uint16_t>(ModbusRegister::MILLIS_HIGH), onPreRead);
    _mb.onGetHreg(static_cast<uint16_t>(ModbusRegister::TEMPERATURE), onPreRead);
    _mb.onGetHreg(static_cast<uint16_t>(ModbusRegister::CPU_FREQ), onPreRead);

    // Callback для записи user value
    _mb.onSetHreg(static_cast<uint16_t>(ModbusRegister::USER_VALUE), onUserValueWrite);
}

void ModbusSlave::update() {
    _mb.task();
    yield();
}

float ModbusSlave::readInternalTemperature() {
    return temperatureRead();
}

uint16_t ModbusSlave::getRegister(ModbusRegister reg) {
    return _mb.Hreg(static_cast<uint16_t>(reg));
}

uint32_t ModbusSlave::getMillis() {
    uint16_t low = _mb.Hreg(static_cast<uint16_t>(ModbusRegister::MILLIS_LOW));
    uint16_t high = _mb.Hreg(static_cast<uint16_t>(ModbusRegister::MILLIS_HIGH));
    return (static_cast<uint32_t>(high) << 16) | low;
}

float ModbusSlave::getTemperature() {
    return readInternalTemperature();
}

uint16_t ModbusSlave::getCpuFrequency() {
    // return getCpuFrequencyMhz();
    return analogRead(36);
}

uint16_t ModbusSlave::getUserValue() const {
    return _userValue;
}

void ModbusSlave::setUserValue(uint16_t value) {
    _userValue = value;
    _mb.Hreg(static_cast<uint16_t>(ModbusRegister::USER_VALUE), value);
}

bool ModbusSlave::isUserValueUpdated() {
    if (_userValueUpdated) {
        _userValueUpdated = false;
        return true;
    }
    return false;
}

void ModbusSlave::printDebugInfo(Stream& serial) {
    serial.println("\n--- Modbus Slave Status ---");
    serial.printf("Slave ID: %d\n", _config.slaveId);
    serial.printf("Baud Rate: %lu\n", _config.baudRate);
    serial.println("\n--- Register Values ---");
    serial.printf("Millis: %lu\n", millis());
    serial.printf("Temperature: %.1f C\n", getTemperature());
    serial.printf("Analog pin: %u MHz\n", getCpuFrequency());
    serial.printf("User Value: %u\n", _userValue);
}

// Статический callback для чтения регистров
uint16_t ModbusSlave::onPreRead(TRegister* reg, uint16_t val) {
    if (_instance == nullptr) {
        return val;
    }

    ModbusRegister regAddr = static_cast<ModbusRegister>(reg->address.address);

    switch (regAddr) {
        case ModbusRegister::MILLIS_LOW: {
            uint32_t currentMillis = millis();
            _instance->_mb.Hreg(static_cast<uint16_t>(ModbusRegister::MILLIS_LOW), 
                               static_cast<uint16_t>(currentMillis & 0xFFFF));
            _instance->_mb.Hreg(static_cast<uint16_t>(ModbusRegister::MILLIS_HIGH), 
                               static_cast<uint16_t>((currentMillis >> 16) & 0xFFFF));
            return static_cast<uint16_t>(currentMillis & 0xFFFF);
        }

        case ModbusRegister::MILLIS_HIGH: {
            uint32_t currentMillis = millis();
            return static_cast<uint16_t>((currentMillis >> 16) & 0xFFFF);
        }

        case ModbusRegister::TEMPERATURE: {
            return static_cast<uint16_t>(_instance->readInternalTemperature() * 10);
        }

        case ModbusRegister::CPU_FREQ: {
            return static_cast<uint16_t>(analogRead(36));
        }

        default:
            return val;
    }
}

// Статический callback для записи user value
uint16_t ModbusSlave::onUserValueWrite(TRegister* reg, uint16_t val) {
    if (_instance != nullptr) {
        _instance->_userValue = val;
        _instance->_userValueUpdated = true;
    }
    return val;
}