#include <Arduino.h>
#include <Wire.h>
#include <WiFiManager.h>
#include <LiquidCrystal_I2C.h>

#include "AppConfig.h"
#include "NtpWeatherService.h"
#include "WebConfigServer.h"

#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

WiFiManager wifiManager;
WebConfigServer webServer;
NtpWeatherService ntpWeather;

String scrollText;
uint16_t scrollPos = 0;

uint32_t lastScrollTick = 0;
uint32_t lastLcdTick = 0;
const uint32_t SCROLL_INTERVAL = 250;

void drawTime() {
    tm tmNow;
    time_t t = ntpWeather.getUnix();
    localtime_r(&t, &tmNow);

    char buf[17];
    snprintf(
        buf, sizeof(buf), "%02d:%02d:%02d",
        tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec
    );

    lcd.setCursor(0, 0);
    lcd.print("    ");
    lcd.print(buf);
    lcd.print("   ");
}

void drawScroll() {
    if (scrollText.length() < LCD_COLS) {
        uint8_t pad = LCD_COLS - scrollText.length();
        for (uint8_t i = 0; i < pad; i++) {
            scrollText += ' ';
        }
    }

    String out;
    for (uint8_t i = 0; i < LCD_COLS; i++) {
        uint16_t idx = (scrollPos + i) % scrollText.length();
        out += scrollText[idx];
    }

    lcd.setCursor(0, 1);
    lcd.print(out);
}

void applyConfig(const AppConfig& cfg) {
    ntpWeather.begin(cfg);
    ntpWeather.forceNtpSync();
    ntpWeather.forceWeatherUpdate();
    scrollText = ntpWeather.getScrollText();
    scrollPos = 0;
}

void setup() {
    Serial.begin(115200);
    Wire.begin();

    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting...");

    wifiManager.autoConnect("Sonoff Connect");

    webServer.begin();
    webServer.setOnSave(applyConfig);

    applyConfig(webServer.getConfig());
}

void loop() {
    ntpWeather.tick();
    webServer.handleClient();

    uint32_t now = millis();

    if (now - lastLcdTick >= 500) {
        lastLcdTick = now;
        drawTime();
    }

    if (now - lastScrollTick >= SCROLL_INTERVAL) {
        lastScrollTick = now;
        scrollText = ntpWeather.getScrollText();
        drawScroll();
        scrollPos = (scrollPos + 1) % max<uint16_t>(1, scrollText.length());
    }
}