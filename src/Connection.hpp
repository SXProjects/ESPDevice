#pragma once
#include <ESP8266WebServer.h>
#include "Diode.hpp"

class IClient {
public:
    virtual bool begin(String const& serverPath, WiFiClient& wifiClient) = 0;

    virtual void message(String const &msg) = 0;

    virtual String command() = 0;

    virtual bool hasCommand() = 0;
};

class Connection
{
public:
    void connect(char const *device, IClient* client);

    void pair(char const* ssid);

    void update();

private:
    String connectWifi();

    ESP8266WebServer pairServer {80};
    WiFiClient wifiClient;
    bool pairing = false;
    bool connected = false;
};