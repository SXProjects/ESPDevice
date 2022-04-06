#pragma once

#include <ArduinoJson.h>
#include "Connection.hpp"
#include <Button.hpp>
#include <utility>

struct Indicator {
    String name;
    DataType type;
    bool send = false;
};

struct Parameter {
    String name;
    DataType type;
};

class Transmitter {
    friend class Device;

public:
    template<typename T>
    void transmit(String const &indicator, T val) {
        if (offline) {
            flash.beginWrite();
            flash.save(key + indicator, val);
            flash.endWrite();
        }
        json["data"]["name"] = indicator;
        json["data"]["value"] = val;
    }

    void sleep(unsigned time) {
        sleepTime = time;
        sleepBegin = millis();
        sendToSleep = true;
    }

    void image(uint8_t const *buf, size_t size) {
        imBuf = buf;
        imSize = size;
    }

private:
    String send();

    uint8_t const *imBuf = nullptr;
    size_t imSize = 0;

    ArduinoJson::DynamicJsonDocument json{256};
    unsigned sleepBegin = 0;
    unsigned sleepTime = 0;
    String key;
    bool offline = true;
    unsigned deviceId;
    bool sendToSleep = false;
};

class IWorkMode {
public:
    virtual void onInit(Transmitter &transmitter) {};

    virtual void onPhysicalInit(Transmitter &transmitter) {};

    virtual void onRelease(Transmitter &transmitter) {};

    virtual void onInit(Transmitter &transmitter, String const &parameter, int val) {};

    virtual void onInit(Transmitter &transmitter, String const &parameter, float val) {};

    virtual void onInit(Transmitter &transmitter, String const &parameter, bool val) {};

    virtual void onReceive(Transmitter &transmitter, String const &parameter, int val) {};

    virtual void onReceive(Transmitter &transmitter, String const &parameter, float val) {};

    virtual void onReceive(Transmitter &transmitter, String const &parameter, bool val) {};

    virtual void onUpdate(Transmitter &transmitter) {};
};

class Device {
public:
    explicit Device(String name) : deviceType(std::move(name)) {
    }

    void setId(unsigned dId) {
        deviceId = dId;
        transmitter.deviceId = dId;
    }

    String const &type() const {
        return deviceType;
    }

    unsigned id() const {
        return deviceId;
    }

    bool workMode(String const &name, IWorkMode *handler);

    String receive(JsonVariantConst const &val, String const &name);

    String setWorkMode(String const &workMode);

    String addWorkMode(String const &workMode);

    String addParameterIndicator(String const &name, String const &type, bool indicator);

    String const &mode() const {
        return currentWorkMode->name;
    }

    void init();

    void update();

    void release();

    unsigned sleepRemain();

    void online();

    void offline() {
        transmitter.offline = true;
    }

    String const &command() const {
        return cmd;
    }

    void image(uint8_t const *&buf, size_t &size) {
        buf = transmitter.imBuf;
        size = transmitter.imSize;
        transmitter.imBuf = nullptr;
        transmitter.imSize = 0;
    }

private:
    struct WorkModeData {
        String name;
        IWorkMode *handler = nullptr;
        std::vector<Indicator> indicators;
        std::vector<Parameter> parameters;
    };

    String deviceType;
    unsigned deviceId;
    std::vector<WorkModeData> workModes;
    WorkModeData *currentWorkMode = nullptr;
    Transmitter transmitter;
    String cmd;
    bool physicalInit = true;
};