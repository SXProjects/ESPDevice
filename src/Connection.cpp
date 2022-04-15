#include "Connection.hpp"

bool Connection::pair(String const &device) {
    diode.smoothly(3000);
    utils::println("pairing...");
    pairing = true;

    IPAddress local_ip(1, 2, 3, 4);
    IPAddress gateway(1, 2, 3, 4);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(device.c_str());
    utils::println("AP: ", device.c_str());
    delay(2000);
    utils::println("AP config");
    WiFi.softAPConfig(local_ip, gateway, subnet);
    utils::println("AP done");

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    pairServer.onNotFound([this] {
        Serial.println("connected");
        pairServer.send(404, "text/plain", "404: Not found");
    });
    pairServer.on("/", HTTP_POST, [this]() {
        String p = pairServer.arg(0);
        ArduinoJson::DynamicJsonDocument json(256);
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
                json.containsKey("ip") && json.containsKey("port") && json.containsKey("room"))) {
            pairServer.send(400, "application/json",
                    R"({"error":"invalid config"})");
            return;
        }

        flash.beginWrite();
        flash.reset();
        flash.saveJson("connectInfo", json);
        flash.endWrite();

        pairServer.send(200, "application/json", R"({"command_name":"pair"})");
        pairing = false;

        utils::reset("Success paired");
    });
    pairServer.begin();
    utils::println("pair server begin");
    return true;
}

bool Connection::connect(String const &device, IClient *client) {
    return client->begin(connectWifi(device), wifiClient);
}

void Connection::update() {
    if (pairing) {
        diode.update();
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

String Connection::connectWifi(String const &devName) {
    if (!json) {
        json = &flash.readJson("connectInfo");

        if (!(json->containsKey("ssid") && json->containsKey("password") &&
                json->containsKey("ip") && json->containsKey("port")&& json->containsKey("room"))) {
            utils::println("Connection config invalid or not exists");
            pair(devName);
            return {};
        }
        ssid = (char const *) (*json)["ssid"];
        password = (char const *) (*json)["password"];
        ip = (char const *) (*json)["ip"];
        port = (unsigned) (*json)["port"];
        room = (char const*) (*json)["room"];
        utils::println("Room: ", room);
    }

    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Connecting to WIFI...");

    int i = 1;
    bool diodeState = false;
    for (; i <= 5; ++i) {
        diodeState = !diodeState;
        if (diodeState) {
            diode.on();
        } else {
            diode.off();
        }

        diode.update();

        Serial.println(i);
        delay(1000);
        if (WiFi.status() == WL_CONNECTED) {
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
        auto serverIp = ip;
        utils::println("Server ip: ", serverIp, " port ", port);
        if(wifiClient.connect(serverIp.c_str(), port))
        {
            return ip + ":" + port + "/";
        } else
        {
            utils::println("WiFi client error");
            return "";
        }
    }

}

bool Connection::isConnected() const {
    return connected;
}