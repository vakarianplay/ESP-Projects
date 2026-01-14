#include <ModbusRTU.h>

// Настройки подключения MAX485
#define RX_PIN 16      // RO -> GPIO16 (UART2 RX)
#define TX_PIN 17      // DI -> GPIO17 (UART2 TX)
#define DE_RE_PIN 4    // DE и RE соединены вместе -> GPIO4

// Настройки Modbus
#define SLAVE_ID 1     // Адрес slave устройства
#define BAUD_RATE 9600 // Скорость обмена

// Адреса регистров (Holding Registers)
#define REG_MILLIS_LOW  0   // Младшая часть millis (16 бит)
#define REG_MILLIS_HIGH 1   // Старшая часть millis (16 бит)
#define REG_TEMPERATURE 2   // Температура чипа
#define REG_CPU_FREQ    3   // Частота процессора в МГц
// Регистр 4 пропущен (зарезервирован)
#define REG_USER_VALUE  5   // Пользовательское значение (чтение/запись)

// Объект Modbus RTU
ModbusRTU mb;

// Второй аппаратный UART
HardwareSerial SerialRS485(2);

// Переменная для хранения user value
uint16_t userValue = 0;

// Функция чтения температуры чипа ESP32
float readInternalTemperature() {
#ifdef ESP_IDF_VERSION_MAJOR
    #if ESP_IDF_VERSION_MAJOR >= 5
        // Для ESP-IDF 5.x и новее
        float temp = temperatureRead();
        return temp;
    #else
        // Для ESP-IDF 4.x
        extern float temperatureRead();
        return temperatureRead();
    #endif
#else
    return temperatureRead();
#endif
}

// Callback функция для обработки запросов на чтение регистров
uint16_t cbPreRead(TRegister* reg, uint16_t val) {
    uint16_t regAddr = reg->address.address;

    // Обновляем значения регистров перед чтением
    if (regAddr == REG_MILLIS_LOW) {
        uint32_t currentMillis = millis();
        mb.Hreg(REG_MILLIS_LOW, (uint16_t)(currentMillis & 0xFFFF));
        mb.Hreg(REG_MILLIS_HIGH, (uint16_t)((currentMillis >> 16) & 0xFFFF));
        return (uint16_t)(currentMillis & 0xFFFF);
    }
    else if (regAddr == REG_MILLIS_HIGH) {
        uint32_t currentMillis = millis();
        return (uint16_t)((currentMillis >> 16) & 0xFFFF);
    }
    else if (regAddr == REG_TEMPERATURE) {
        // Температура в десятых долях градуса (например, 456 = 45.6°C)
        float temp = readInternalTemperature();
        return (uint16_t)(temp * 10);
    }
    else if (regAddr == REG_CPU_FREQ) {
        // Частота в МГц
        return (uint16_t)getCpuFrequencyMhz();
    }

    return val;
}

// Callback функция для обработки записи в регистр user value
uint16_t cbUserValueWrite(TRegister* reg, uint16_t val) {
    userValue = val;
    Serial.printf("\n[WRITE] User Value updated: %u\n", userValue);
    return val;
}

void setup() {
    // Инициализация Serial для отладки
    Serial.begin(115200);
    Serial.println("\n=== ESP32 Modbus RTU Slave ===");
    Serial.printf("Slave ID: %d\n", SLAVE_ID);
    Serial.printf("Baud Rate: %d\n", BAUD_RATE);
    Serial.printf("DE/RE Pin: %d\n", DE_RE_PIN);

    // Инициализация UART2 для RS485
    SerialRS485.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);

    // Инициализация Modbus RTU с указанием пина DE/RE
    mb.begin(&SerialRS485, DE_RE_PIN);

    // Установка режима slave с указанным адресом
    mb.slave(SLAVE_ID);

    // Добавление Holding Registers
    mb.addHreg(REG_MILLIS_LOW, 0);      // Регистр 0: millis (младшие 16 бит)
    mb.addHreg(REG_MILLIS_HIGH, 0);     // Регистр 1: millis (старшие 16 бит)
    mb.addHreg(REG_TEMPERATURE, 0);     // Регистр 2: температура
    mb.addHreg(REG_CPU_FREQ, 0);        // Регистр 3: частота CPU
    mb.addHreg(REG_USER_VALUE, 0);      // Регистр 5: user value (R/W)

    // Установка callback для обновления данных перед чтением
    mb.onGetHreg(REG_MILLIS_LOW, cbPreRead);
    mb.onGetHreg(REG_MILLIS_HIGH, cbPreRead);
    mb.onGetHreg(REG_TEMPERATURE, cbPreRead);
    mb.onGetHreg(REG_CPU_FREQ, cbPreRead);

    // Установка callback для записи user value
    mb.onSetHreg(REG_USER_VALUE, cbUserValueWrite);

    Serial.println("Modbus slave initialized!");
    Serial.println("\nRegisters:");
    Serial.println("  0: millis (low 16 bits)     [READ]");
    Serial.println("  1: millis (high 16 bits)    [READ]");
    Serial.println("  2: chip temperature (x10)   [READ]");
    Serial.println("  3: CPU frequency (MHz)      [READ]");
    Serial.println("  5: user value               [READ/WRITE]");
}

void loop() {
    // Обработка Modbus запросов
    mb.task();
    yield();

    // Вывод отладочной информации каждые 5 секунд
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 5000) {
        lastPrint = millis();

        Serial.println("\n--- Current values ---");
        Serial.printf("Millis: %lu\n", millis());
        Serial.printf("Temperature: %.1f °C\n", readInternalTemperature());
        Serial.printf("CPU Frequency: %u MHz\n", getCpuFrequencyMhz());
        Serial.printf("User Value: %u\n", userValue);
    }
}