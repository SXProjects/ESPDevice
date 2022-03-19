#include "Device.hpp"
#include "Diode.hpp"

String Transmitter::send() {
    if (json.isNull()) {
        return {};
    }
    json["command_name"] = "transmit_data";
    String res;
    ArduinoJson::serializeJson(json, res);
    json.clear();
    return res;
}

DataType Device::parseDataType(const String &type) {
    if (type == "int") {
        return DataType::Int;
    } else if (type == "float") {
        return DataType::Float;
    } else if (type == "bool") {
        return DataType::Bool;
    }
    Serial.println("Invalid data type");
    ESP.reset();
    return DataType(-1);
}

bool Device::workMode(const String &name, IWorkMode *handler) {
    if (!client) {
        error("workMode", "client is not specified");
        return false;
    }

    for (auto &wm: workModes) {
        if (wm.name == name) {
            wm.handler = handler;
            return true;
        }
    }
    return false;
}

void Device::receive(const String &payload) {
    ArduinoJson::DynamicJsonDocument json{1024};
    ArduinoJson::deserializeJson(json, payload);

    if (!json.containsKey("command_name")) {
        return error("undefined", "command_name is not specified");
    }
    String commandName = (char const *) json["command_name"];

    if (commandName == "device_type_info") {
        deviceTypeInfo(commandName, json);
    } else if (commandName == "transmit_data") {
        transmitData(commandName, json);
    } else if (commandName == "set_work_mode") {
        setWorkMode(commandName, json);
    }

    return error("undefined", "unidentified command_name");
}

void Device::deviceTypeInfo(String const &commandName, const DynamicJsonDocument &json) {
    if (!workModes.empty()) {
        return error(commandName, "device already configured");
    }

    if (!json.containsKey("work_modes")) {
        return error(commandName, "work_modes is not specified");
    }

    auto const &wm = json["work_modes"];
    for (size_t w = 0; w < wm.size(); ++w) {
        WorkModeData data;
        data.name = (char const *) wm[w]["name"];
        data.liveTime = wm[w]["live_time"];
        data.sleepTime = wm[w]["sleep_time"];

        if (data.sleepTime > 10800000) {
            return error(commandName, "ESP8266 can't sleep that long");
        }

        data.receiveInterval = wm[w]["receive_interval"];
        data.handler = nullptr;

        if (json.containsKey("indicators")) {
            auto const &idc = wm[w]["indicators"];
            for (size_t i = 0; i < idc.size(); ++i) {
                data.indicators.push_back({idc[i]["name"], parseDataType(idc[i]["type"])});
            }
        }

        if (json.containsKey("parameters")) {
            auto const &idc = wm[w]["parameters"];
            for (size_t i = 0; i < idc.size(); ++i) {
                data.parameters.push_back({idc[i]["name"], parseDataType(idc[i]["type"])});
            }
        }

        workModes.push_back(data);
    }
    return success(commandName);
}

void Device::transmitData(String const &commandName, const DynamicJsonDocument &json) {
    if (currentWorkMode == nullptr) {
        return error(commandName, "work mode is not specified");
    }

    if (!json.containsKey("data")) {
        return error(commandName, "data is not specified");
    }

    auto const &data = json["data"];
    for (size_t i = 0; i < data.size(); ++i) {
        auto const &val = data[i]["val"];
        String name = data[i]["name"];
        if (val.is<float>()) {
            currentWorkMode->handler->onReceive(transmitter, name, (float) val);
        } else if (val.is<int>()) {
            currentWorkMode->handler->onReceive(transmitter, name, (int) val);
        } else if (val.is<bool>()) {
            currentWorkMode->handler->onReceive(transmitter, name, (bool) val);
        }
    }
    return success(commandName);
}

void Device::setWorkMode(String const &commandName, const DynamicJsonDocument &json) {
    if (workModes.empty()) {
        return error(commandName, "device is not configured");
    }

    if (!json.containsKey("work_mode")) {
        return error(commandName, "work_mode is not specified");
    }

    String workMode = json["work_mode"];
    for (auto &wm: workModes) {
        if (wm.name == workMode) {
            if (wm.handler) {
                wm.handler->onRelease(transmitter);
            }
            currentWorkMode = &wm;
            transmitter.switchWorkMode(&currentWorkMode->indicators);
            if (currentWorkMode->handler) {
                currentWorkMode->handler->onInit(transmitter);
            }
            wakeupTime = millis();
            return success(commandName);
        }
    }
    return error(commandName, "work mode on device is not exist");
}

void Device::update() {
    diode.loop();
    connection.update();

    if (currentWorkMode) {
        if (!configuring) {
            diode.on();
            Serial.println("configuration done");
            configuring = true;
        }

        if (millis() - receiveTime >= currentWorkMode->receiveInterval) {
            currentWorkMode->receiveInterval = millis();
            if (client->hasCommand()) {
                currentWorkMode->receiveInterval = millis();
                receive(client->command());
            }
        }

        if (currentWorkMode->handler) {
            currentWorkMode->handler->onUpdate(transmitter);
        }

        auto msg = transmitter.send();
        if (!msg.isEmpty()) {
            client->message(msg);
        }

        if (millis() - wakeupTime > currentWorkMode->liveTime) {
            currentWorkMode->handler->onRelease(transmitter);
            ESP.deepSleep(currentWorkMode->sleepTime * 1000);
        }
    } else {
        if (millis() - receiveTime >= 100) {
            receiveTime = millis();
            if (connection.isConnected() && client->hasCommand()) {
                receive(client->command());
            }
        }
    }
}

void Device::error(const String &from, const String &message) {
    String msg = R"({"command_name": )" + from + R"(,"error":")" + message + "\"}";
    client->message(msg);
    Serial.println(msg);
}

void Device::success(const String &from) {
    String msg = R"({"command_name": )" + from + "\"}";
    client->message(msg);
    Serial.println(msg);
}

bool Device::setup(const String &deviceType, IClient *newClient) {
    Serial.begin(9600);
    client = newClient;
    connection.connect(deviceType.c_str(), client);
    success("device_wakeup");
    if (connection.isConnected()) {

        diode.smoothly(600);
    }
    Serial.println("waiting for configuration");

    return true;
}

Device device = {};