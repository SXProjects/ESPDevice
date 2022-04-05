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

void SmartESP::setup(IClient *newClient, unsigned int uId, unsigned v) {
    Serial.begin(115200);
    button.setup(D1);
    client = newClient;
    uniqueId = uId;
    version = v;
    devName = String("ESP32-") + uniqueId;
}

bool SmartESP::workMode(String const &deviceType, String const &name, IWorkMode *handler) {
    for (auto &dev: devices) {
        if (dev.type() == deviceType) {
            return dev.workMode(name, handler);
        }
    }
    devices.push_back(Device{deviceType});
    devices.back().workMode(name, handler);
    return true;
}

void SmartESP::update() {
    diode.update();
    button.update();
    connection.update();

    if (button.endTime() - button.startTime() > 4000) {
        release();
        flash.reset();
        connection.pair(devName);
        updated = false;
        configured = false;
    }

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

    if (!updated) {
        if (updateConfig()) {
            utils::println("switching to online config");
            updated = true;
            configured = true;
            offline = false;

            for(auto& d : devices)
            {
                d.online();
            }

            diode.smoothly(600);
        } else if (!configured) {
            if (flash.readString("connectInfo").empty()) {
                connection.pair(devName);
            } else {
                if (deserializeConfig()) {
                    utils::println("switching to offline config");
                    diode.blink(600);
                } else {
                    diode.blink(100);
                    utils::println("switching to empty configuration");
                }
                configured = true;
                offline = true;

                for(auto& d : devices)
                {
                    d.offline();
                }
            }
        }
    }

    if (updated) {
        if (message("wakeup", R"("unique_id":)" + String(uniqueId), true)) {
            offline = false;
            processCommand();
        } else {
            offline = true;
            error("wakeup", "can't update command (/commands/get)", true);
        }
    }

    for(auto& d : devices)
    {
        d.update();
    }
}

bool SmartESP::configureDevices(String const &commandName) {
    if (!devices.empty()) {
        error(commandName, "device already configured");
        return false;
    }

    JSON_FIELD(lastJson, command_name)
    if (j_command_name != commandName) {
        error(commandName, "device must be configured by device_config_info");
    }

    devices.clear();

    JSON_FIELD(lastJson, devices)

    for (size_t d = 0; d < j_devices.size(); ++d) {
        JSON_FIELD(j_devices[d], device_id)
        JSON_FIELD_V(j_devices[d], name, device_type)
        JSON_FIELD(j_devices[d], current_work_mode)
        JSON_FIELD(j_devices[d], work_modes)

        Device *dev = nullptr;

        for (auto &device: devices) {
            if (device.type() == j_device_type) {
                device.setId(j_device_id);
                dev = &device;
            }
        }

        if (dev == nullptr) {
            error(commandName, "device id: " + String(j_device_id) + " not found");
            return false;
        }

        for (size_t w = 0; w < j_work_modes.size(); ++w) {
            JSON_FIELD_V(j_work_modes[w], name, work_mode)
            JSON_FIELD(j_work_modes[w], indicators)
            JSON_FIELD(j_work_modes[w], parameters)

            CHECK_MSG(dev->addWorkMode(j_work_mode))

            for (size_t i = 0; i < j_indicators.size(); ++i) {
                JSON_FIELD(j_indicators[i], name)
                JSON_FIELD(j_indicators[i], type)
                CHECK_MSG(dev->addParameterIndicator(j_name, j_type, true))
            }

            for (size_t j = 0; j < j_parameters.size(); ++j) {
                JSON_FIELD(j_parameters[j], name)
                JSON_FIELD(j_parameters[j], type)
                CHECK_MSG(dev->addParameterIndicator(j_name, j_type, false))
            }
        }

        CHECK_MSG(dev->setWorkMode(j_current_work_mode))
    }

    return message(commandName, "", true);
}

void SmartESP::error(const String &from, const String &message, bool isGet) {
    String msg = R"({"command_name": ")" + from + R"(","error":")" + message + "\"}";
    if (isGet) {
        client->send(msg);
    } else {
        client->get(msg);
    }
    Serial.println("Error: " + msg);
}

bool SmartESP::message(const String &from, String const &json, bool isGet) {
    String msg;
    if (json.isEmpty()) {
        msg = R"({"command_name": ")" + from + "\"}";
    } else {
        msg = R"({"command_name": ")" + from + "\"," + json + "}";
    }

    Serial.println("Message: " + msg);
    String answer;
    if (isGet) {
        answer = client->get(msg);
    } else {
        answer = client->send(msg);
    }

    if (answer.isEmpty()) {
        Serial.println("Server answer error");
        return false;
    } else {
        return ArduinoJson::deserializeJson(lastJson, json);
    }
}

bool SmartESP::checkJsonField(ArduinoJson::DynamicJsonDocument const &json, String const &f) {
    if (!json.containsKey(f)) {
        error("configureDevices", "devices is not specified");
        return false;
    }
    return true;
}

void SmartESP::processCommand() {
    if (!lastJson.containsKey("command_name")) {
        return error("undefined", "command_name is not specified");
    }

    String commandName = (char const *) lastJson["command_name"];

    if (commandName == "transmit_data") {
        transmitData(commandName);
    } else if (commandName == "set_work_mode") {
        setWorkMode(commandName);
    }

    return error("undefined", "unidentified command_name");
}

bool SmartESP::transmitData(const String &commandName) {
    JSON_FIELD(lastJson, device_id)
    JSON_FIELD(lastJson, data)
    FIND_DEVICE(j_device_id, dev)

    for (size_t d = 0; d < j_data.size(); ++d) {
        JSON_FIELD(j_data[d], name)
        JSON_FIELD(j_data[d], value)

        CHECK_MSG(dev->receive(j_value, j_name))
    }

    return message(commandName);
}

bool SmartESP::setWorkMode(const String &commandName) {
    JSON_FIELD(lastJson, device_id)
    JSON_FIELD(lastJson, work_mode);
    FIND_DEVICE(j_device_id, dev)

    dev->setWorkMode(j_work_mode);
    return message(commandName);
}

void SmartESP::sleep(unsigned int ms) {
    if (ms > ms * 1000 * 60 * 60 * 2) {
        error("sleep", "ESP can't sleep longer then two hours");
        return;
    }

    release();

//    lastSleepTimePoint = timeSinceUp();
//    sleepTime = ms;
//
//    if (ms >= 5 * 60 * 1000) {
//        flash.begin();
//        flash.writeLastSleepTimePoint(lastSleepTimePoint);
//        ESP.deepSleep(sleepTime * 1000);
//    }
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

bool SmartESP::updateConfig() {
    if (!connection.isConnected()) {
        Serial.println("can't request config from server - device is not connected to network");
        return false;
    }

    char const *commandName = "configure";

    lastJson.clear();
    lastJson["command_name"] = commandName;
    lastJson["unique_id"] = uniqueId;
    lastJson["version"] = version;

    auto types = lastJson.createNestedArray("device_types");
    for (auto const &d: devices) {
        auto e = types.addElement();
        e["type"] = d.type();
        e["work_mode"] = d.mode();
    }

    String command;
    if (!deserializeJson(lastJson, command)) {
        utils::reset("Json updateConfig deserialization error");
    }

    if (!message(commandName, command, true)) {
        Serial.println("can't request config from server - server did not respond");
        return false;
    }

    if (lastJson.containsKey("devices")) {
        if (configureDevices("device_config_info")) {
            return saveConfig();
        } else {
            return false;
        }
    }

    return true;
}

bool SmartESP::saveConfig() {
    String connectInfo = flash.readString("connectInfo").data();

    flash.beginWrite();
    flash.reset();
    flash.saveString("connectInfo", connectInfo);
    flash.saveJson("deviceConfig", lastJson);
    flash.endWrite();

    return true;
}

bool SmartESP::deserializeConfig() {
    char const *commandName = "device_config_info";
    auto const &doc = flash.readJson("deviceConfig");
    if (doc.isNull()) {
        error(commandName, "can't read config from flash", true);
        return false;
    }
    lastJson = doc;
    if (configureDevices(commandName)) {
        return true;
    } else {
        error(commandName, "corrupted config in memory");
        lastJson.clear();
        return false;
    }
}

SmartESP esp = {};
