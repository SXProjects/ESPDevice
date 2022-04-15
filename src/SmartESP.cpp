#include "SmartESP.hpp"

#define JSON_FIELD(JSON, FIELD) \
if (!checkJsonField(JSON, #FIELD)) { \
error(commandName, "field \"" #FIELD "\" is not specified");\
return false; \
} \
auto const& j_##FIELD = JSON [ #FIELD ];

#define JSON_FIELD_V(JSON, FIELD, VAR) \
if (!checkJsonField(JSON, #FIELD)) { \
error(commandName, "field \"" #FIELD "\" is not specified");\
return false; \
} \
auto const& j_##VAR = JSON [ #FIELD ];

#define FIND_DEVICE(ID, NAME) \
auto NAME = findDevice(ID); \
if (!NAME) { \
error(commandName, "device is not found"); \
return false; \
}

#define CHECK_MSG(CALL)\
if (auto msg = CALL; msg != "") {\
error(commandName, msg);\
return false;\
}

void SmartESP::setup(uint8_t buttonPin, IClient *newClient, unsigned int uId, unsigned v) {
    Serial.begin(9600);
    button.setup(buttonPin);
    client = newClient;
    uniqueId = uId;
    version = v;
    devName = String("ESP32-") + uniqueId;
    if (!flash.read()) {
        utils::println("Fatal error, can't read config");
    }
}

bool SmartESP::workMode(String const &deviceType, String const &name, IWorkMode *handler) {
    for (auto &dev: devices) {
        if (dev.type() == deviceType) {
            return dev.workMode(name, handler);
        }
    }
    devices.push_back(Device{deviceType});
    devices.back().workMode(name, handler);
    utils::println("New type: ", devices.back().type());
    return true;
}

void SmartESP::update() {
    diode.update();
    button.update();

    if (button.endTime() - button.startTime() > 4000) {
        release();
        flash.reset();
        connection.pair(devName);
    }

    connection.update();

    if (connection.isPairing()) {
        return;
    }

//    if (timeSinceUp() - lastSleepTimePoint < sleepTime) {
//        return;
//    } else {
//        lastSleepTimePoint = 0;
//        sleepTime = 0;
//    }

    if (!connection.isConnected()) {
        connection.connect(devName, client);
    }

    if (connection.isPairing()) {
        return;
    }

    for (auto &d: devices) {
        d.setRoom(connection.getRoom());
        utils::println("Updating: ", d.type());
        d.update();
        auto cmd = d.command();
        if (!cmd.isEmpty()) {
            if (client->send(cmd).isEmpty()) {
                connection.connect(devName, client);
            }
        }
    }

    if (millis() - lastGetTime > getInterval) {
        lastGetTime = millis();
        if (message(R"({"room":")" + String(connection.getRoom()) + "\"}", true)) {
            processCommand();
        } else {
            error("wakeup", "can't update command (/commands/get)");
        }
    }
}

void SmartESP::processCommand() {
    if (lastJson.containsKey("receiver")) {
        transmitData("getData");
    }
}

bool SmartESP::transmitData(const String &commandName) {
    JSON_FIELD(lastJson, receiver)
    JSON_FIELD(lastJson, parameter)
    utils::println("transmit data from ", (char const *) j_receiver, "->",
            (bool) j_parameter);

    for (auto &d: devices) {
        CHECK_MSG(d.receive(j_parameter, j_receiver))
    }

    return true;
}

void SmartESP::error(const String &from, const String &message) {
    String msg = R"({"command_name": ")" + from + R"(","error":")" + message + "\"}";
    client->error(msg);
    Serial.println("Error: " + msg);
}

bool SmartESP::messageFrom(const String &from, String const &json, bool isGet) {
    String msg;
    if (json.isEmpty()) {
        msg = R"({"command_name": ")" + from + "\"}";
    } else {
        msg = R"({"command_name": ")" + from + "\"," + json + "}";
    }

    return message(msg, isGet);
}

bool SmartESP::message(bool isGet) {
    String msg;
    if (!ArduinoJson::serializeJson(lastJson, msg)) {
        utils::reset("Json message serialization error");
    }
    return message(msg, isGet);
}

bool SmartESP::message(String const &msg, bool isGet) {
    String answer;
    if (isGet) {
        answer = client->get(msg);
    } else {
        answer = client->send(msg);
    }

    if (answer.isEmpty()) {
        Serial.println("Server empty answer");
        return false;
    } else {
        utils::println("Server answer: ", answer);
        lastJson.clear();
        auto err = ArduinoJson::deserializeJson(lastJson, answer);
        if (err) {
            utils::println("Json deser err: ", err.c_str());
        }
        return !err;
    }
}

bool SmartESP::checkJsonField(ArduinoJson::DynamicJsonDocument const &json, String const &f) {
    if (!json.containsKey(f)) {
        error("json", "check error");
        return false;
    }
    return true;
}

Device *SmartESP::findDevice(unsigned int deviceId) {
    for (auto &d: devices) {
        if (d.id() == deviceId) {
            return &d;
        }
    }
    return nullptr;
}

void SmartESP::release() {
    for (auto &d: devices) {
        d.release();
    }
}

//uint64_t SmartESP::timeSinceUp() {
//    return timeSinceServerUp + millis();
//}
//
//uint64_t SmartESP::lastSleepTime() {
//    return timeSinceUp() - lastSleepTimePoint;
//}


bool SmartESP::saveConfig() {
    String connectInfo = flash.readString("connectInfo");

    flash.beginWrite();
    flash.reset();
    flash.saveString("connectInfo", connectInfo);
    flash.endWrite();

    return true;
}

SmartESP esp = {};
