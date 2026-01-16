#include "ModbusSlave.h"

// Инициализация статического указателя
ModbusSlave* ModbusSlave::_instance = nullptr;

ModbusSlave::ModbusSlave() 
    : _serial(2)
    , _userValue(0)
    , _userValueUpdated(false)
    , _initialized(false)
{

}

ModbusSlave::~ModbusSlave() {
    if (_instance == this) {
        _instance = nullptr;
    }
}

void ModbusSlave::begin(const ModbusConfig& config) {
    _config = config;

    // Устанавливаем _instance ЗДЕСЬ, после полной инициализации
    _instance = this;

    Serial.printf("[ModbusSlave] Setting _instance = %p\n", (void*)this);

    // Инициализация UART для RS485
    _serial.begin(_config.baudRate, SERIAL_8N1, _config.rxPin, _config.txPin);

    // Инициализация Modbus RTU
    _mb.begin(&_serial, _config.deRePin);
    _mb.slave(_config.slaveId);

    // Настройка регистров
    setupRegisters();

    _initialized = true;

    Serial.println("[ModbusSlave] Initialization complete");
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
    _mb.addHreg(static_cast<uint16_t>(ModbusRegister::MILLIS_LOW), 0);
    _mb.addHreg(static_cast<uint16_t>(ModbusRegister::MILLIS_HIGH), 0);
    _mb.addHreg(static_cast<uint16_t>(ModbusRegister::TEMPERATURE), 0);
    _mb.addHreg(static_cast<uint16_t>(ModbusRegister::CPU_FREQ), 0);
    _mb.addHreg(static_cast<uint16_t>(ModbusRegister::USER_VALUE), 0);

    _mb.onGetHreg(static_cast<uint16_t>(ModbusRegister::MILLIS_LOW), onPreRead);
    _mb.onGetHreg(static_cast<uint16_t>(ModbusRegister::MILLIS_HIGH), onPreRead);
    _mb.onGetHreg(static_cast<uint16_t>(ModbusRegister::TEMPERATURE), onPreRead);
    _mb.onGetHreg(static_cast<uint16_t>(ModbusRegister::CPU_FREQ), onPreRead);

    _mb.onSetHreg(static_cast<uint16_t>(ModbusRegister::USER_VALUE), onUserValueWrite);

    Serial.println("[ModbusSlave] Registers configured");
}

void ModbusSlave::update() {
    if (!_initialized) return;
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
    serial.printf("Instance address: %p\n", (void*)this);
    serial.printf("Static _instance: %p\n", (void*)_instance);
    serial.printf("Slave ID: %d\n", _config.slaveId);
    serial.printf("Baud Rate: %lu\n", _config.baudRate);
    serial.println("\n--- Register Values ---");
    serial.printf("Millis: %lu\n", millis());
    serial.printf("Temperature: %.1f C\n", getTemperature());
    serial.printf("AnalogValue: %u MHz\n", getCpuFrequency());
    serial.printf("User Value (member): %u\n", _userValue);
    serial.printf("User Value (register): %u\n", _mb.Hreg(static_cast<uint16_t>(ModbusRegister::USER_VALUE)));
}

String ModbusSlave::getDebugInfoJson() {
    String json;
    json.reserve(256);

    json += "{";
    json += "\"config\":{";
    json += "\"slaveId\":" + String(_config.slaveId) + ",";
    json += "\"baudRate\":" + String(_config.baudRate) + ",";
    json += "\"rxPin\":" + String(_config.rxPin) + ",";
    json += "\"txPin\":" + String(_config.txPin) + ",";
    json += "\"deRePin\":" + String(_config.deRePin);
    json += "},";
    json += "\"registers\":{";
    json += "\"millis\":" + String(millis()) + ",";
    json += "\"temperature\":" + String(getTemperature(), 1) + ",";
    json += "\"analogValue\":" + String(getCpuFrequency()) + ",";
    json += "\"userValue\":" + String(_mb.Hreg(static_cast<uint16_t>(ModbusRegister::USER_VALUE)));
    json += "}";
    json += "}";

    return json;
}

uint16_t ModbusSlave::onPreRead(TRegister* reg, uint16_t val) {
    if (_instance == nullptr) {
        Serial.println("[ERROR] _instance is nullptr in onPreRead!");
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
            // return static_cast<uint16_t>(getCpuFrequencyMhz());
            return analogRead(36);
        }

        default:
            return val;
    }
}

uint16_t ModbusSlave::onUserValueWrite(TRegister* reg, uint16_t val) {
    if (_instance != nullptr) {
        _instance->_userValue = val;
        _instance->_userValueUpdated = true;
    }
    return val;
}