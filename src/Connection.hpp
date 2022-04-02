#pragma once
#include <ESP8266WebServer.h>
#include "Diode.hpp"
#include <Flash.hpp>

class IClient {
public:
    virtual bool begin(String const& serverPath, WiFiClient& wifiClient) = 0;

    virtual String send(const String &msg) = 0;

    virtual String get(String const &msg) = 0;

};

class Connection
{
public:
    bool connect(String const& device, IClient* client);

    bool pair(String const& device);

    void update();

    bool isConnected() const;

    bool isPairing() const
    {
        return pairing;
    }

private:
    String connectWifi();

    ESP8266WebServer pairServer {80};
    WiFiClient wifiClient;
    bool pairing = false;
    bool connected = false;
};