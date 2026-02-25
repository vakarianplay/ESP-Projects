#include "NtpWeatherService.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>

void NtpWeatherService::begin(const AppConfig& cfg) {
    config = cfg;
    ntp.setHost(config.ntpServer.c_str());
    ntp.setGMT(0);
    ntp.begin();
    Serial.begin(9600);
    Serial.println("====NTPWEATHER BEGIN====");

    lastNtpSync = 0;
    lastWeatherUpdate = 0;

    ntp.updateNow();
    updateWeather();
}

void NtpWeatherService::tick() {
    ntp.tick();

    uint32_t now = millis();

    if (now - lastNtpSync >= NTP_SYNC_INTERVAL) {
        lastNtpSync = now;
        ntp.updateNow();
    }

    if (now - lastWeatherUpdate >= WEATHER_INTERVAL) {
        lastWeatherUpdate = now;
        updateWeather();
    }
}

void NtpWeatherService::forceNtpSync() {
    ntp.updateNow();
}

void NtpWeatherService::forceWeatherUpdate() {
    updateWeather();
}

time_t NtpWeatherService::getUnix() const {
    return ntp.getUnix() + config.tzOffsetSec;
}

String NtpWeatherService::getScrollText() const {
    return buildScrollText();
}

String NtpWeatherService::urlEncode(const String& s) const {
    String out;
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if (isalnum(c)) out += c;
        else if (c == ' ') out += '+';
        else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", (uint8_t)c);
            out += buf;
        }
    }
    return out;
}

void NtpWeatherService::updateWeather() {
    Serial.println("UPD Weather");
    if (config.weatherApiKey.length() < 8) return;
    if (WiFi.status() != WL_CONNECTED) return;

    String url = "http://api.openweathermap.org/data/2.5/weather?q=";
    url += urlEncode(config.weatherCity);
    url += "&appid=" + config.weatherApiKey;
    url += "&units=metric&lang=en";

    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    if (code == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        DeserializationError err = deserializeJson(doc, payload);
        if (!err) {
            weatherDesc = doc["weather"][0]["description"].as<String>();
            weatherTemp = doc["main"]["temp"].as<float>();
        }
    }
    http.end();
}

String NtpWeatherService::buildScrollText() const {
    tm tmNow;
    time_t t = ntp.getUnix() + config.tzOffsetSec;
    localtime_r(&t, &tmNow);

    char dateBuf[24];
    snprintf(
        dateBuf, sizeof(dateBuf), "%02d.%02d.%04d",
        tmNow.tm_mday, tmNow.tm_mon + 1, tmNow.tm_year + 1900
    );

    String s = String(dateBuf);
    s += " | ";
    s += weatherDesc;
    s += " ";
    s += String(weatherTemp, 1);
    s += " C   ";
    return s;
}