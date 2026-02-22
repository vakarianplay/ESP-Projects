#pragma once

#include <Arduino.h>
#include <GyverNTP.h>
#include "AppConfig.h"

class NtpWeatherService {
public:
    void begin(const AppConfig& cfg);
    void tick();
    void forceNtpSync();
    void forceWeatherUpdate();

    String getScrollText() const;
    time_t getUnix() const;

private:
    void updateWeather();
    String buildScrollText() const;
    String urlEncode(const String& s) const;

    AppConfig config;
    GyverNTP ntp;

    String weatherDesc = "unknown";
    float weatherTemp = 0.0f;

    uint32_t lastNtpSync = 0;
    uint32_t lastWeatherUpdate = 0;

    // static const uint32_t NTP_SYNC_INTERVAL = 60UL * 60UL * 1000;
    // static const uint32_t WEATHER_INTERVAL = 60UL * 60UL * 1000;
    static const uint32_t NTP_SYNC_INTERVAL = 60*60*3600;
    static const uint32_t WEATHER_INTERVAL = 60*60*3600;

};