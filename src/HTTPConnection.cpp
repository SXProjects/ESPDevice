#include "HTTPConnection.hpp"

String HTTPConnection::send(const String &msg) {
    if(!sender.begin(*client, "http://" + path + "data/send"))
    {
        return "";
    }
    sender.addHeader("Content-Type", "application/json");

    int status = sender.POST(msg);
    sender.end();

    if (status != 200) {
        Serial.print("http data/send error: ");
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

String HTTPConnection::get(const String &msg) {
    bool get = getter.begin(*client, "http://" + path + "command/get");
    if(!get)
    {
        return "";
    }
    getter.addHeader("Content-Type", "application/json");
    int status = getter.POST(msg);
    getter.end();
    if (status != 200) {
        Serial.print("http command/get error: ");
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
    if(serverPath.isEmpty())
    {
        return false;
    }

    Serial.println("HTTP INIT");

    utils::println("data send path: ", "http://" + serverPath + "data/send");
    utils::println("command get path: ", "http://" + serverPath + "command/get");

    path = serverPath;
    client = &wifiClient;

    return true;
}

String HTTPConnection::sendImage(uint8_t const *fbBuf, size_t fbLen) {
    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = fbLen + extraLen;

    client->println("POST /user/image/add HTTP/1.1");
    client->println("Host: " + path);
    client->println("Content-Length: " + String(totalLen));
    client->println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    client->println();
    client->print(head);

    for (size_t n = 0; n < fbLen; n = n + 1024) {
        if (n + 1024 < fbLen) {
            client->write(fbBuf, 1024);
            fbBuf += 1024;
        } else if (fbLen % 1024 > 0) {
            size_t remainder = fbLen % 1024;
            client->write(fbBuf, remainder);
        }
    }
    client->print(tail);

    int timoutTimer = 1000;
    long startTimer = millis();
    boolean state = false;

    String body;
    String all;
    while ((startTimer + timoutTimer) > millis()) {
        Serial.print(".");
        delay(100);
        while (client->available()) {
            char c = client->read();
            if (c == '\n') {
                if (all.length() == 0) { state = true; }
                all = "";
            } else if (c != '\r') { all += String(c); }
            if (state) { body += String(c); }
            startTimer = millis();
        }
        if (body.length() > 0) { break; }
    }

    return body;
}
