#pragma once
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "BlinkingDiode.hpp"

class IOTConnection
{
public:
    void connect(char const *device);

    void pair(char const* ssid);

    void loop();

private:
    ESP8266WebServer pairServer {80};
    BlinkingDiode diode;
    bool pairing = false;
};