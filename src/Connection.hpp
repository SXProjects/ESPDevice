#pragma once
#include <WebServer.h>
#include "Diode.hpp"
#include <Flash.hpp>

class IClient {
public:
    virtual bool begin(String const& serverPath, WiFiClient& wifiClient) = 0;

    virtual String send(const String &msg) = 0;

    virtual String get(String const &msg) = 0;

    virtual String sendImage(uint8_t const* buf, size_t len) = 0;
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
    String connectWifi(String const& devName);

    WebServer pairServer {80};
    WiFiClient wifiClient;
    bool pairing = false;
    bool connected = false;
    ArduinoJson::DynamicJsonDocument const* json = nullptr;
    String ssid;
    String password;
    String ip;
    unsigned port;
};