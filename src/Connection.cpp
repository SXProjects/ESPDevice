#include "Connection.hpp"
#include <EEPROM.h>

unsigned const MAX_MEMORY = 256;

void saveInMemory(String const &s, int address) {
    for (size_t i = 0; i < s.length(); ++i) {
        EEPROM.write(address + i, s[i]);
        Serial.print("Wrote: ");
        Serial.println(s[i]);
    }
    EEPROM.write(s.length() + address, '\0');
}

String readFromMemory(int address) {
    int i = 0;
    String result;
    while (true) {
        uint8_t c = EEPROM.read(address + i);
        if (c == '\0')
            break;
        result += c;
    }
    return result;
}

void Connection::pair(char const *device) {
    diode.smoothly(3000);
    pairing = true;

    IPAddress local_ip(1, 2, 3, 4);
    IPAddress gateway(1, 2, 3, 4);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP(device);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    pairServer.onNotFound([this] {
        Serial.println("connected");
        pairServer.send(404, "text/plain", "404: Not found");
    });
    pairServer.on("/", [this] {
        Serial.println("New connection");
        pairServer.send(200, "text/plain", "OK");
    });
    pairServer.on("/config", HTTP_POST, [this]() {
        String content;
        int status;
        if (!pairServer.hasArg("ssid") || !pairServer.hasArg("password")
                || !pairServer.hasArg("ip") || !pairServer.hasArg("port")) {
            status = 400;
            content = R"({"error": "invalid request"})";
        } else {
            String ssid = pairServer.arg("ssid");
            String password = pairServer.arg("password");
            String ip = pairServer.arg("ip");
            String port = pairServer.arg("port");

            unsigned len = ssid.length() + password.length() + ip.length() + port.length();
            if (len > MAX_MEMORY) {
                status = 400;
                content = R"({"error":"max request arguments len is 256"})";
            } else {
                EEPROM.begin(len);
                EEPROM.write(0, 42);
                saveInMemory(ssid, 1);
                saveInMemory(password, ssid.length() + 2);
                saveInMemory(ip, ssid.length() + password.length() + 3);
                saveInMemory(port, ssid.length() + password.length() + ip.length() + 4);
                EEPROM.commit();
                status = 200;
                content = R"({"success":"connection saved"})";
            }
        }
        pairServer.send(status, "application/json", content);
        if (status == 200) {

            Serial.println("Success pair");
            ESP.reset();
        } else {
            Serial.println("Error");
        }
    });
    pairServer.begin();
}

void Connection::connect(char const *device, IClient *client) {
    diode.setup();
    EEPROM.begin(MAX_MEMORY);
    // если данные о подключении имеются
    if (EEPROM.read(0) != 42) {
        Serial.println("Pairing");
        pair(device);
    } else {
        Serial.println("Connecting to WiFi");
        client->begin(connectWifi(), wifiClient);
    }
}

void Connection::update() {
    if (pairing) {
        diode.loop();
        pairServer.handleClient();
    } else {
        if (connected && WiFi.status() != WL_CONNECTED) {
            connected = false;
            diode.blink(20);
        } else if (!connected && WiFi.status() == WL_CONNECTED) {
            connected = true;
            diode.on();
        }
    }
}

String Connection::connectWifi() {
    String ssid = readFromMemory(1);
    String password = readFromMemory(ssid.length() + 2);
    String ip = readFromMemory(ssid.length() + password.length() + 3);
    String port = readFromMemory(ssid.length() + password.length() + ip.length() + 4);

    Serial.println("ssid");
    Serial.println(ssid);
    Serial.println("password");
    Serial.println(password);
    Serial.println("ip");
    Serial.println(ip);
    Serial.println("port");
    Serial.println(port);

    WiFi.begin(ssid, password);
    Serial.println("Connecting...");

    int i = 1;
    for (; i <= 5 || WiFi.status() != WL_CONNECTED; ++i) {
        Serial.println(i);
        delay(1000);
    }

    if (i == 5) {
        Serial.println("WiFi connection error");
    } else {
        Serial.println("WiFi connection success");
    }

    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
    wifiClient.connect(ip, port.toInt());
    return ip + ":" + port + "/";
}

bool Connection::isConnected() const {
    return connected;
}
