#include <Arduino.h>
#include <HTTPClient.h>

class DataGrabber
{
public:
    DataGrabber();
    void setupUrl(String url_);
    
    String getData();
    
private:
    WiFiClient client;
    HTTPClient http;
    String url;
    String payload;
};