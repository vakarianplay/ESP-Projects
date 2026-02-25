#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <Preferences.h>
#include <functional>
#include "AppConfig.h"

class WebConfigServer {
public:
    WebConfigServer();

    void begin();
    void handleClient();
    const AppConfig& getConfig() const;

    void setOnSave(std::function<void(const AppConfig&)> cb);

private:
    void loadConfig();
    void saveConfig();
    String buildRootPage() const;

    void handleRoot();
    void handleSave();

    WebServer httpServer;
    HTTPUpdateServer httpUpdater;
    Preferences prefs;

    AppConfig config;
    std::function<void(const AppConfig&)> onSave;
};