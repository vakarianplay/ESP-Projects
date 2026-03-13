# Clock with ntp-sync and weather info

![alt text](https://img.shields.io/badge/Chip-ESP32-blue?style=for-the-badge&logo=Espressif) 

![alt text](https://img.shields.io/badge/Status-complete-darkgreen?style=for-the-badge&logo=Buddy)



## 🔌 Hardware

>* ESP32
>* LCD1602 I2C
>* jumper wires


## 📑 Dependencies

>* bblanchon/ArduinoJson
>* GyverNTP
>* WiFiManager
>* LiquidCrystal_I2C_ESP32

## 💎 Features

>* NTP time sync
>* Weather info from OpenWeatherMap (need API key)
>* Configure timezone
>* Web interface
>* OTA update

## 🚀 How to start

>* [Download .bin firmware](https://github.com/vakarianplay/ESP-Projects/raw/refs/heads/main/ESP-ntpclock/firmware_esp32.bin) or build in platform io
>* Flash your esp32 by esptool `esptool --port (your port) --baud 460800 write_flash --flash_mode dio 0x0 firmware_esp32.bin`
>* Connect LCD display: GPIO21 - SDA, GPIO22 - SLC
>* Set up your wi-fi network. Connect to AP "ESP Connect" and enter your credentials

-----------------------------------

<img src="https://github.com/user-attachments/assets/8813accc-29c2-4214-8daf-190f17cdf3bb" width="400" />

<img src="https://github.com/user-attachments/assets/39950700-855b-45a6-a059-6a8edf148f87" width="400" />

-----------------------------

<img src="https://github.com/user-attachments/assets/17205990-7605-4c23-86e8-eb816b19b318" width="600" />

-----------------------------

## Web interface for configure device




<img width="650" height="587" alt="Снимок экрана от 2026-03-08 16-39-38" src="https://github.com/user-attachments/assets/d4b17d62-2684-48d7-b029-6d139afc7135" />







