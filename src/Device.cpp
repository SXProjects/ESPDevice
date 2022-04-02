#include "Device.hpp"
#include "Diode.hpp"

String Transmitter::send() {
    if (json.isNull()) {
        return {};
    }
    json["command_name"] = "transmit_data";
    json["device_id"] = deviceId;
    String res;
    ArduinoJson::serializeJson(json, res);
    json.clear();
    return res;
}

DataType parseDataType(const String &type) {
    if (type == "int") {
        return DataType::Int;
    } else if (type == "float") {
        return DataType::Float;
    } else if (type == "bool") {
        return DataType::Bool;
    }
    return DataType(-1);
}

bool Device::workMode(const String &name, IWorkMode *handler) {
    for (auto &wm: workModes) {
        if (wm.name == name) {
            wm.handler = handler;
            return false;
        }
    }
    workModes.push_back({name, handler});
    return false;
}

String Device::receive(const JsonVariantConst &val, const String &name) {
    if (val.is<float>()) {
        currentWorkMode->handler->onReceive(transmitter, name, (float) val);
    } else if (val.is<int>()) {
        currentWorkMode->handler->onReceive(transmitter, name, (int) val);
    } else if (val.is<bool>()) {
        currentWorkMode->handler->onReceive(transmitter, name, (bool) val);
    } else {
        return "type is not supported";
    }
    return "";
}

String Device::setWorkMode(const String &workMode) {
    for (auto &wm: workModes) {
        if (wm.name == workMode) {
            if (wm.handler) {
                release();
            }
            currentWorkMode = &wm;
            transmitter.switchWorkMode(&currentWorkMode->indicators);
            if (currentWorkMode->handler) {
                init();
            }
            return "";
        }
    }
    return "work mode is not found";
}

String Device::addParameterIndicator(const String &name, String type, bool indicator) {
    DataType dType = parseDataType(type);
    if ((int) dType == -1) {
        return "invalid data type";
    }
    if (workModes.empty()) {
        return "work mode is not specified";
    }
    if (indicator) {

        workModes.back().indicators.push_back({name, dType});
    } else {
        workModes.back().parameters.push_back({name, dType});
    }
    return "";
}

String Device::addWorkMode(String const &name) {
    for (auto &wm: workModes) {
        if (wm.name == name) {
            return "work mode already exist";
        }
    }

    workModes.push_back({name, nullptr});
    return "";
}

void Device::update() {
    currentWorkMode->handler->onUpdate(transmitter);
}

void Device::release() {
    currentWorkMode->handler->onRelease(transmitter);
}

void Device::init() {
    transmitter.sleepTime = 0;
    transmitter.sleepBegin = 0;
    currentWorkMode->handler->onInit(transmitter);
}

unsigned Device::sleepRemain() {
    unsigned timeElapsed = millis() - transmitter.sleepBegin;
    if (transmitter.sleepTime < timeElapsed) {
        return 0;
    } else {
        return transmitter.sleepTime - timeElapsed;
    }
}

