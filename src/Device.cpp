#include "Device.hpp"
#include "Diode.hpp"

String Transmitter::send() {
    if (json.isNull()) {
        return {};
    }
    json["room"] = room;
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
    utils::println("adding work mode: ", name);
    workModes.push_back({name, handler});
    currentWorkMode = 0;
    return false;
}

String Device::receive(const JsonVariantConst &val, const String &name) {
    auto key = "init-" + deviceType + "-" + workModes[currentWorkMode].name + "-" + name;
    if (val.is<float>()) {
        workModes[currentWorkMode].handler->onReceive(transmitter, name, val.as<float>());
    } else if (val.is<int>()) {
        workModes[currentWorkMode].handler->onReceive(transmitter, name, val.as<int>());
    } else if (val.is<bool>()) {
        workModes[currentWorkMode].handler->onReceive(transmitter, name, val.as<bool>());
    } else {
        return "type is not supported";
    }
    return "";
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
    if(transmitter.firstInit)
    {
        init();
        transmitter.firstInit = false;
    }

    if (transmitter.sendToSleep) {
        workModes[currentWorkMode].handler->onRelease(transmitter);
        transmitter.sendToSleep = false;
    }

    if (transmitter.sleepTime != 0) {
        if (millis() - transmitter.sleepBegin > transmitter.sleepTime) {
            init();
        }
    } else {
        workModes[currentWorkMode].handler->onUpdate(transmitter);
    }

    cmd = transmitter.send();
}

void Device::release() {
    workModes[currentWorkMode].handler->onRelease(transmitter);
}

void Device::init() {
    transmitter.sleepTime = 0;
    transmitter.sleepBegin = 0;

    if(physicalInit)
    {
        workModes[currentWorkMode].handler->onPhysicalInit(transmitter);
        physicalInit = false;
    }

    workModes[currentWorkMode].handler->onInit(transmitter);
}

unsigned Device::sleepRemain() {
    unsigned timeElapsed = millis() - transmitter.sleepBegin;
    if (transmitter.sleepTime < timeElapsed) {
        return 0;
    } else {
        return transmitter.sleepTime - timeElapsed;
    }
}

