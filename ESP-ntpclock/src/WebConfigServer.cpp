#include "WebConfigServer.h"
#include "index.h"
#include "NtpWeatherService.h"   // нужно для доступа к методам сервиса

WebConfigServer::WebConfigServer(NtpWeatherService* svc)
    : httpServer(80), service(svc) {}

void WebConfigServer::begin() {
    loadConfig();

    httpServer.on("/", [this]() { handleRoot(); });
    httpServer.on("/save", HTTP_POST, [this]() { handleSave(); });
    httpServer.on("/state", [this]() { handleState(); });

    httpUpdater.setup(&httpServer, "/firmware");
    httpServer.begin();
}

void WebConfigServer::handleClient() {
    httpServer.handleClient();
}

const AppConfig& WebConfigServer::getConfig() const {
    return config;
}

void WebConfigServer::setOnSave(std::function<void(const AppConfig&)> cb) {
    onSave = cb;
}

void WebConfigServer::loadConfig() {
    prefs.begin("cfg", true);
    config.ntpServer    = prefs.getString("ntp",  config.ntpServer);
    config.weatherCity  = prefs.getString("city", config.weatherCity);
    config.weatherApiKey= prefs.getString("key",  config.weatherApiKey);
    config.tzOffsetSec  = prefs.getInt   ("tz",   config.tzOffsetSec);
    prefs.end();
}

void WebConfigServer::saveConfig() {
    prefs.begin("cfg", false);
    prefs.putString("ntp",  config.ntpServer);
    prefs.putString("city", config.weatherCity);
    prefs.putString("key",  config.weatherApiKey);
    prefs.putInt   ("tz",   config.tzOffsetSec);
    prefs.end();
}

String WebConfigServer::buildRootPage() const {
    String html = FPSTR(webpage);
    html.replace("{{NTP}}",  config.ntpServer);
    html.replace("{{CITY}}", config.weatherCity);
    html.replace("{{KEY}}",  config.weatherApiKey);
    html.replace("{{TZ}}",   String(config.tzOffsetSec));
    return html;
}

void WebConfigServer::handleRoot() {
    httpServer.send(200, "text/html", buildRootPage());
}

void WebConfigServer::handleSave() {
    if (httpServer.hasArg("ntp"))  config.ntpServer   = httpServer.arg("ntp");
    if (httpServer.hasArg("city")) config.weatherCity = httpServer.arg("city");
    if (httpServer.hasArg("key"))  config.weatherApiKey = httpServer.arg("key");
    if (httpServer.hasArg("tz"))   config.tzOffsetSec = httpServer.arg("tz").toInt();

    saveConfig();
    if (onSave) onSave(config);

    httpServer.sendHeader("Location", "/");
    httpServer.send(302, "text/plain", "Saved");
}

void WebConfigServer::handleState() {
    if (!service) {
        httpServer.send(500, "application/json", "{\"err\":\"service unavailable\"}");
        return;
    }

    String json = "{";
    json += "\"time\":\""    + service->getTimeStr()    + "\",";
    json += "\"date\":\""    + service->getDateStr()    + "\",";
    json += "\"weather\":\"" + service->getWeatherStr() + "\"";
    json += "}";

    httpServer.send(200, "application/json", json);
}