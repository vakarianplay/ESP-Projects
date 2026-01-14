#include <ModbusRTU.h>

// Настройки подключения MAX485
#define RX_PIN 16
#define TX_PIN 17
#define DE_RE_PIN 4

// Настройки Modbus
#define SLAVE_ID 1
#define BAUD_RATE 9600

// Адреса регистров (Holding Registers)
#define REG_MILLIS_LOW  0
#define REG_MILLIS_HIGH 1
#define REG_TEMPERATURE 2
#define REG_CPU_FREQ    3
#define REG_USER_VALUE  5

// Регистры для текста (6-21 = 16 регистров = 32 символа)
#define REG_TEXT_START  6
#define REG_TEXT_LENGTH 16  // 16 регистров × 2 символа = 32 символа max
#define MAX_TEXT_LENGTH (REG_TEXT_LENGTH * 2)

// Объект Modbus RTU
ModbusRTU mb;
HardwareSerial SerialRS485(2);

// Переменные
uint16_t userValue = 0;
char textBuffer[MAX_TEXT_LENGTH + 1] = "Hello Modbus!";  // Начальный текст

// Функция чтения температуры чипа ESP32
float readInternalTemperature() {
    return temperatureRead();
}

// Обновление регистров текста из буфера
void updateTextRegisters() {
    for (int i = 0; i < REG_TEXT_LENGTH; i++) {
        char c1 = textBuffer[i * 2];
        char c2 = textBuffer[i * 2 + 1];

        // Упаковка: старший байт = первый символ, младший = второй
        uint16_t regValue = ((uint16_t)c1 << 8) | (uint8_t)c2;
        mb.Hreg(REG_TEXT_START + i, regValue);
    }
}

// Извлечение текста из регистров в буфер
void extractTextFromRegisters() {
    for (int i = 0; i < REG_TEXT_LENGTH; i++) {
        uint16_t regValue = mb.Hreg(REG_TEXT_START + i);
        textBuffer[i * 2] = (char)(regValue >> 8);      // Старший байт
        textBuffer[i * 2 + 1] = (char)(regValue & 0xFF); // Младший байт
    }
    textBuffer[MAX_TEXT_LENGTH] = '\0';  // Гарантируем нуль-терминатор
}

// Callback для чтения регистров
uint16_t cbPreRead(TRegister* reg, uint16_t val) {
    uint16_t regAddr = reg->address.address;

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
        float temp = readInternalTemperature();
        return (uint16_t)(temp * 10);
    }
    else if (regAddr == REG_CPU_FREQ) {
        return (uint16_t)getCpuFrequencyMhz();
    }

    return val;
}

// Callback для записи user value
uint16_t cbUserValueWrite(TRegister* reg, uint16_t val) {
    userValue = val;
    Serial.printf("\n[WRITE] User Value updated: %u\n", userValue);
    return val;
}

// Callback для записи текстовых регистров
uint16_t cbTextWrite(TRegister* reg, uint16_t val) {
    // После записи извлекаем весь текст
    // Используем задержку через флаг, чтобы обработать после завершения транзакции
    static bool textUpdated = false;
    textUpdated = true;

    return val;
}

// Флаг обновления текста
bool textNeedsUpdate = false;

// Callback после записи текста
uint16_t cbTextPostWrite(TRegister* reg, uint16_t val) {
    textNeedsUpdate = true;
    return val;
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== ESP32 Modbus RTU Slave with Text ===");
    Serial.printf("Slave ID: %d\n", SLAVE_ID);
    Serial.printf("Baud Rate: %d\n", BAUD_RATE);

    // Инициализация UART2 для RS485
    SerialRS485.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);

    // Инициализация Modbus RTU
    mb.begin(&SerialRS485, DE_RE_PIN);
    mb.slave(SLAVE_ID);

    // Добавление регистров данных
    mb.addHreg(REG_MILLIS_LOW, 0);
    mb.addHreg(REG_MILLIS_HIGH, 0);
    mb.addHreg(REG_TEMPERATURE, 0);
    mb.addHreg(REG_CPU_FREQ, 0);
    mb.addHreg(REG_USER_VALUE, 0);

    // Добавление регистров для текста
    for (int i = 0; i < REG_TEXT_LENGTH; i++) {
        mb.addHreg(REG_TEXT_START + i, 0);
        mb.onSetHreg(REG_TEXT_START + i, cbTextPostWrite);
    }

    // Инициализация текстовых регистров начальным значением
    updateTextRegisters();

    // Установка callbacks
    mb.onGetHreg(REG_MILLIS_LOW, cbPreRead);
    mb.onGetHreg(REG_MILLIS_HIGH, cbPreRead);
    mb.onGetHreg(REG_TEMPERATURE, cbPreRead);
    mb.onGetHreg(REG_CPU_FREQ, cbPreRead);
    mb.onSetHreg(REG_USER_VALUE, cbUserValueWrite);

    Serial.println("Modbus slave initialized!");
    Serial.println("\nRegisters:");
    Serial.println("  0:     millis (low 16 bits)     [READ]");
    Serial.println("  1:     millis (high 16 bits)    [READ]");
    Serial.println("  2:     chip temperature (x10)   [READ]");
    Serial.println("  3:     CPU frequency (MHz)      [READ]");
    Serial.println("  5:     user value               [READ/WRITE]");
    Serial.println("  6-21:  text (32 chars max)      [READ/WRITE]");
}

void loop() {
    mb.task();
    yield();

    // Проверяем, был ли обновлён текст
    if (textNeedsUpdate) {
        textNeedsUpdate = false;
        extractTextFromRegisters();

        // Выводим только печатные символы
        Serial.print("\n[WRITE] Text updated: \"");
        for (int i = 0; i < MAX_TEXT_LENGTH && textBuffer[i] != '\0'; i++) {
            if (isPrintable(textBuffer[i])) {
                Serial.print(textBuffer[i]);
            }
        }
        Serial.println("\"");
    }

    // Вывод отладочной информации каждые 5 секунд
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 5000) {
        lastPrint = millis();

        Serial.println("\n--- Current values ---");
        Serial.printf("Millis: %lu\n", millis());
        Serial.printf("Temperature: %.1f °C\n", readInternalTemperature());
        Serial.printf("CPU Frequency: %u MHz\n", getCpuFrequencyMhz());
        Serial.printf("User Value: %u\n", userValue);

        // Выводим текст
        Serial.print("Text: \"");
        for (int i = 0; i < MAX_TEXT_LENGTH && textBuffer[i] != '\0'; i++) {
            if (isPrintable(textBuffer[i])) {
                Serial.print(textBuffer[i]);
            }
        }
        Serial.println("\"");
    }
}
