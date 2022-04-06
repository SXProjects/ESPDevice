#pragma once

#include "Device.hpp"
#include "Connection.hpp"
#include <HTTPClient.h>

class HTTPConnection : public IClient {
public:
    bool begin(String const& serverPath, WiFiClient& wifiClient) override;

    String send(const String &msg) override;

    String get(String const &msg) override;

    String sendImage(uint8_t const* buf, size_t len) override;

private:
    WiFiClient* client = nullptr;
    HTTPClient sender;
    HTTPClient getter;
    String path;
};

