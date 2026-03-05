#pragma once
#include <Arduino.h>

struct AppConfig {
    String ntpServer = "ntp1.vniiftri.ru";
    String weatherCity = "Moscow";
    String weatherApiKey = "openweather api key here";
    int32_t tzOffsetSec = 3 * 3600;
};
