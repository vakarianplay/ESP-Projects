#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <Preferences.h>
#include <functional>
#include "AppConfig.h"

// Forward declaration, чтобы не тянуть весь заголовок
class NtpWeatherService;

class WebConfigServer {
public:
    explicit WebConfigServer(NtpWeatherService* svc = nullptr);

    void begin();
    void handleClient();
    const AppConfig& getConfig() const;

    void setOnSave(std::function<void(const AppConfig&)> cb);
    void setService(NtpWeatherService* svc) { service = svc; }

private:
    void loadConfig();
    void saveConfig();
    String buildRootPage() const;

    void handleRoot();
    void handleSave();
    void handleState();   // AJAX JSON

    WebServer httpServer;
    HTTPUpdateServer httpUpdater;
    Preferences prefs;

    AppConfig config;
    std::function<void(const AppConfig&)> onSave;

    NtpWeatherService* service = nullptr;
};