#pragma once

#include "Device.hpp"
#include "Connection.hpp"
#include <ESP8266HTTPClient.h>

class HTTPConnection : public IClient {
public:
    bool begin(String const& serverPath, WiFiClient& wifiClient) override;

    String send(const String &msg) override;

    String get(String const &msg) override;

private:
    HTTPClient sender;
    HTTPClient getter;
};

