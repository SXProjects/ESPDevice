#include "Connection.hpp"
#include <EEPROM.h>
#include <ArduinoJson.hpp>

unsigned const MAX_MEMORY = 256;

void saveInMemory(String const &s, int address) {
    for (size_t i = 0; i < s.length(); ++i) {
        EEPROM.write(address + i, s[i]);
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
        if (i == 30) {
            return {};
        }
        result += char(c);
        ++i;
    }
    return result;
}

bool Connection::pair(String const& device) {
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
    pairServer.on("/", HTTP_POST, [this]() {
        String p = pairServer.arg(0);
        ArduinoJson::DynamicJsonDocument json(1024);
        ArduinoJson::deserializeJson(json, p);

        if (json.containsKey("command_name")) {
            if (json["command_name"] != "pair") {
                pairServer.send(400, "application/json",
                        R"({"error":"undefined command"})");
                return;
            }

        } else {
            pairServer.send(400, "application/json",
                    R"({"error":"command is not specified"})");
            return;
        }

        if (!(json.containsKey("ssid") && json.containsKey("password") &&
                json.containsKey("ip") && json.containsKey("port"))) {
            pairServer.send(400, "application/json",
                    R"({"error":"invalid config"})");
            return;
        }

        String ssid = json["ssid"];
        String password = json["password"];
        String ip = json["ip"];
        String port = json["port"];

        unsigned len = ssid.length() + password.length() + ip.length() + port.length() + 4;
        if (len > MAX_MEMORY) {
            pairServer.send(400, "application/json",
                    R"({"error":"max request arguments len is 256"})");
            return;
        }

        EEPROM.begin(MAX_MEMORY);
        saveInMemory(ssid, 1);
        saveInMemory(password, ssid.length() + 2);
        saveInMemory(ip, ssid.length() + password.length() + 3);
        saveInMemory(port, ssid.length() + password.length() + ip.length() + 4);
        EEPROM.write(0, 42);
        EEPROM.commit();
        pairServer.send(200, "application/json",
                R"({"command_name":"pair"})");
        Serial.println("Success pair");
        ESP.reset();

    });
    pairServer.begin();
    return true;
}

bool Connection::connect(String const& device, IClient *client) {
    diode.setup();
    EEPROM.begin(MAX_MEMORY);
    // если данные о подключении имеются
    if (EEPROM.read(0) != 42) {
        Serial.println("Pairing init");
        pair(device);
        return false;
    } else {
        Serial.println("Connecting to WiFi");
        return client->begin(connectWifi(), wifiClient);
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
    Serial.println("SSID...");
    String ssid = readFromMemory(1);
    if (ssid == "") {
        Serial.println("Can't read ssid from memory");
        reset();
    }
    Serial.println(ssid);
    Serial.println("PASSWORD...");
    String password = readFromMemory(ssid.length() + 2);
    Serial.println(password);
    if (password == "") {
        Serial.println("Can't read password from memory");
        reset();
    }
    Serial.println("IP...");
    String ip = readFromMemory(ssid.length() + password.length() + 3);
    Serial.println(ip);
    if (ip == "") {
        Serial.println("Can't read ip from memory");
        reset();
    }
    Serial.println("PORT...");
    String port = readFromMemory(ssid.length() + password.length() + ip.length() + 4);
    Serial.println(port);
    if (port == "") {
        Serial.println("Can't read port from memory");
        reset();
    }

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
    bool diodeState = false;
    for (; i <= 5; ++i) {
        diodeState = !diodeState;
        if(diodeState)
        {
            diode.on();
        } else
        {
            diode.off();
        }

        diode.loop();

        Serial.println(i);
        delay(1000);
        if(WiFi.status() == WL_CONNECTED)
        {
            break;
        }
    }

    if (i == 5) {
        Serial.println("WiFi connection error");
        return {};
    } else {
        Serial.println("WiFi connection message");
        Serial.print("IP address:\t");
        Serial.println(WiFi.localIP());
        connected = true;
        wifiClient.connect(ip, port.toInt());
        return ip + ":" + port + "/";
    }

}

bool Connection::isConnected() const {
    return connected;
}

void Connection::reset() {
    EEPROM.begin(1);
    EEPROM.write(0, 0);
    EEPROM.commit();
    ESP.reset();
}
