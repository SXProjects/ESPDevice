#include "HTTPConnection.hpp"

String HTTPConnection::send(const String &msg)
{
    int status = sender.POST(msg);
    if (status != 200) {
        Serial.print("http POST error: ");
        Serial.print(status);
        Serial.print(" Error msg: ");
        Serial.println(sender.getString());
        diode.blink(200);
        return "";
    } else {
        diode.on();
        return sender.getString();
    }
}

String HTTPConnection::get(const String &msg)
{
    int status = getter.GET();
    if (status != 200) {
        Serial.print("http GET error: ");
        Serial.println(status);
        Serial.print("Error msg: ");
        Serial.println(getter.getString());
        diode.blink(200);
        return "";
    } else {
        diode.on();
        return getter.getString();
    }
}

bool HTTPConnection::begin(String const &serverPath, WiFiClient &wifiClient) {
    Serial.println("HTTP INIT");

    bool send = sender.begin(wifiClient, "http://" + serverPath + "/command/send");
    bool get = getter.begin(wifiClient, "http://" + serverPath + "/command/get");
    return send && get;
}
