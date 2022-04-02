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

void SmartESP::setup(IClient *newClient, unsigned int uId) {
    Serial.begin(115200);
    client = newClient;
    uniqueId = uId;

    flash.begin();
    rebootSleepTime = flash.readSleepTime();
}

bool SmartESP::workMode(String const &deviceType, String const &name, IWorkMode *handler) {
    for (auto &dev: devices) {
        if (dev.type() == deviceType) {
            dev.workMode(name, handler);
        }
    }
}

void SmartESP::update() {
    diode.update();
    button.update();
    connection.update();

    if (button.endTime() - button.startTime() > 4000) {
        Serial.println("Device reset using the button");
        release();
        flash.resetReboot();
    }

    if (millis() - lastSleepTime < sleepTime) {
        return;
    } else {
        lastSleepTime = 0;
        sleepTime = 0;
    }

    if (connection.isPairing()) {
        return;
    }

    if (!connection.isConnected()) {
        devices.clear();
        lastJson.clear();
    }

    if (devices.empty() || !connect()) {
        Serial.println("trying to connect again and again... (");
        return;
    }

    if (message("wakeup", R"("unique_id":)" + String(uniqueId), true)) {
        processCommand();
    } else {
        error("wakeup", "can't update command (/commands/get)", true);
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
    JSON_FIELD(lastJson, devices)

    for (size_t d = 0; d < j_devices.size(); ++d) {
        JSON_FIELD(j_devices[d], device_id)
        JSON_FIELD_V(j_devices[d], name, device_type)
        JSON_FIELD(j_devices[d], current_work_mode)
        JSON_FIELD(j_devices[d], work_modes)

        devices.push_back(Device(j_device_type, j_device_id));
        auto &dev = devices.back();

        for (size_t w = 0; w < j_work_modes.size(); ++w) {
            JSON_FIELD_V(j_work_modes[w], name, work_mode)
            JSON_FIELD(j_work_modes[w], live_time)
            JSON_FIELD(j_work_modes[w], sleep_time)
            JSON_FIELD(j_work_modes[w], receive_interval)
            JSON_FIELD(j_work_modes[w], indicators)
            JSON_FIELD(j_work_modes[w], parameters)

            CHECK_MSG(dev.addWorkMode(j_work_mode))

            for (size_t i = 0; i < j_indicators.size(); ++i) {
                JSON_FIELD(j_indicators[i], name)
                JSON_FIELD(j_indicators[i], type)
                CHECK_MSG(dev.addParameterIndicator(j_work_mode, j_sleep_time, true))
            }

            for (size_t j = 0; j < j_parameters.size(); ++j) {
                JSON_FIELD(j_parameters[j], name)
                JSON_FIELD(j_parameters[j], type)
                CHECK_MSG(dev.addParameterIndicator(j_work_mode, j_sleep_time, false))
            }
        }

        CHECK_MSG(dev.setWorkMode(j_current_work_mode))
    }

    return message(commandName, "", true);
}

bool SmartESP::connect() {
    button.setup(D1);
    if (connection.connect("ESP_" + String(uniqueId), client)) {
        if (!message("device_wakeup", R"("unique_id": )" + String(uniqueId) +
                R"(,"start":true)", true)) {
            Serial.println("invalid device configuration");
            return false;
        }

        diode.smoothly(600);
        Serial.println("configuring devices");
        return configureDevices("device_config_info");
    } else {
        Serial.println("can't connect to serer, waiting for reconnecting");
        return false;
    }
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
    release();

    if (ms > ms * 1000 * 60 * 60 * 2) {
        error("sleep", "ESP can't sleep longer then two hours");
    } else if (ms < 5 * 60 * 1000) {
        lastSleepTime = millis();
        sleepTime = ms;
        flash.begin();
        flash.writeSleepTime(sleepTime);
    } else {
        ESP.deepSleep(ms * 1000);
    }
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
