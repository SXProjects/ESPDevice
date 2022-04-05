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
    currentWorkMode = &workModes[0];
    return false;
}

String Device::receive(const JsonVariantConst &val, const String &name) {
    flash.beginWrite();
    auto key = "init-" + deviceType + "-" + currentWorkMode->name + "-" + name;
    if (val.is<float>()) {
        currentWorkMode->handler->onReceive(transmitter, name, (float) val);
        flash.save(key, (float) val);
    } else if (val.is<int>()) {
        currentWorkMode->handler->onReceive(transmitter, name, (int) val);
        flash.save(key, (int) val);
    } else if (val.is<bool>()) {
        currentWorkMode->handler->onReceive(transmitter, name, (bool) val);
        flash.save(key, (bool) val);
    } else {
        return "type is not supported";
    }
    flash.endWrite();
    return "";
}

String Device::setWorkMode(const String &workMode) {
    for (auto &wm: workModes) {
        if (wm.name == workMode) {
            if (wm.handler) {
                release();
            }

            currentWorkMode = &wm;
            transmitter.key = "send-" + deviceType + "-" + currentWorkMode->name + "-";

            if (currentWorkMode->handler) {
                init();
            }
            return "";
        }
    }
    return "work mode is not found";
}

String Device::addParameterIndicator(const String &name, String const &type, bool indicator) {
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
    if (transmitter.sendToSleep) {
        currentWorkMode->handler->onRelease(transmitter);
        transmitter.sendToSleep = false;
    }

    if (transmitter.sleepTime != 0) {
        if (millis() - transmitter.sleepBegin > transmitter.sleepTime) {
            init();
        }
    } else {
        currentWorkMode->handler->onUpdate(transmitter);
    }

    if (transmitter.sleepTime != 0) {

    }

    currentWorkMode->handler->onUpdate(transmitter);
}

void Device::release() {
    currentWorkMode->handler->onRelease(transmitter);
}

void Device::init() {
    transmitter.sleepTime = 0;
    transmitter.sleepBegin = 0;
    currentWorkMode->handler->onInit(transmitter);
    for (auto const &p: currentWorkMode->parameters) {
        auto key = "send-" + deviceType + "-" + currentWorkMode->name + "-" + p.name;
        switch (p.type) {
            case DataType::Float:
                float f;
                if (flash.get(key, f)) {
                    currentWorkMode->handler->onInit(transmitter, p.name, f);
                }
                break;
            case DataType::Int:
                int i;
                if (flash.get(key, i)) {
                    currentWorkMode->handler->onInit(transmitter, p.name, i);
                }
                break;
            case DataType::Bool:
                bool b;
                if (flash.get(key, b)) {
                    currentWorkMode->handler->onInit(transmitter, p.name, b);
                }
                break;
            default:
                break;
        }
    }
}

unsigned Device::sleepRemain() {
    unsigned timeElapsed = millis() - transmitter.sleepBegin;
    if (transmitter.sleepTime < timeElapsed) {
        return 0;
    } else {
        return transmitter.sleepTime - timeElapsed;
    }
}

void Device::online() {
    for (auto const &p: currentWorkMode->indicators) {
        auto key = "init-" + deviceType + "-" + currentWorkMode->name + "-" + p.name;

        switch (p.type) {
            case DataType::Float:
                float f;
                if (flash.get(key, f)) {
                    transmitter.transmit(currentWorkMode->name, f);
                    flash.clear<float>(key);
                }
                break;
            case DataType::Int:
                int i;
                if (flash.get(key, i)) {
                    transmitter.transmit(currentWorkMode->name, i);
                    flash.clear<int>(key);
                }
                break;
            case DataType::Bool:
                bool b;
                if (flash.get(key, b)) {
                    transmitter.transmit(currentWorkMode->name, b);
                    flash.clear<bool>(key);
                }
                break;
            default:
                break;
        }

    }
    transmitter.offline = false;
}

