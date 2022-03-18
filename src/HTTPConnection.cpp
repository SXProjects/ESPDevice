#include "HTTPConnection.hpp"

void HTTPConnection::message(const String &msg) {
    client.POST(msg);
}

String HTTPConnection::command() {
    return client.getString();
}

bool HTTPConnection::hasCommand() {
    int status = client.GET();
    if (status != 200) {
        Serial.print("http GET error: ");
        Serial.println(status);
        diode.blink(200);
        return false;
    } else {
        diode.on();
        return true;
    }
}

bool HTTPConnection::begin(String const &serverPath, WiFiClient &wifiClient) {
    client.begin(wifiClient, serverPath);
}
