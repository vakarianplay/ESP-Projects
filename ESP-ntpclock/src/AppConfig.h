#pragma once
#include <Arduino.h>

struct AppConfig {
    String ntpServer = "ntp1.vniiftri.ru";
    String weatherCity = "Moscow";
    String weatherApiKey = "28bc310f78d0674674d5ca06e7a2a556";
    int32_t tzOffsetSec = 3 * 3600;
};