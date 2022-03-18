#pragma once

#include "Device.hpp"
#include "Connection.hpp"
#include <ESP8266HTTPClient.h>

class HTTPConnection : public IClient {
public:
    bool begin(String const& serverPath, WiFiClient& wifiClient) override;

    void message(const String &msg) override;

    String command() override;

    bool hasCommand() override;

private:
    HTTPClient client;
};

