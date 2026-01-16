#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <Arduino.h>

IPAddress local_ip(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);

HTTPUpdateServer httpUpdater;
WebServer server(80);

class WifiModule {
public:
    WifiModule();
    ~WifiModule();

    void wifiStart(String ssidName);
    bool isWifiStart();

    void setModbusObj(ModbusSlave amodbus);

private:
    ModbusSlave _modbus;
    bool wifiFlag = false;
    void handleRoot();
};

WifiModule::WifiModule() {
    Serial.println("Create wifi object");
}



void WifiModule::wifiStart(String ssidName) {
    WiFi.softAPConfig(local_ip, gateway, subnet); //Create AP mode
    WiFi.mode(WIFI_AP);

    if (WiFi.softAP(ssidName)){
        Serial.println("Init AP: " + ssidName + "\nIP: "+ local_ip.toString());
        httpUpdater.setup(&server, "/firmware");
        server.begin();
        server.on("/", [this](){this->handleRoot();});
        wifiFlag = true;
    }
}

bool WifiModule::isWifiStart() {
    return wifiFlag;
}

void WifiModule::setModbusObj(ModbusSlave amodbus) {
    _modbus = amodbus;
}

void WifiModule::handleRoot() {
    // server.send(200, "text/plane", String(_modbus.getCpuFrequency()));
    server.send(200, "application/json", _modbus.getDebugInfoJson());
}