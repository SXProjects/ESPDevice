#pragma once

#include "ArduinoJson/ArduinoJson.h"
#include "Connection.hpp"

enum class DataType {
    Float,
    Int,
    Bool,
};

class Transmitter {
    friend class Device;

public:
    template<typename T>
    void transmit(String const &indicator, T val) {
        checkIndicator<T>(indicator);
        json["data"]["name"] = indicator;
        json["data"]["value"] = val;
    }

private:
    void switchWorkMode(std::vector<std::pair<String, DataType>> const *i) {
        indicators = i;
    }

    String send();

    template<typename T>
    bool checkIndicator(String const &indicator) {
        for (auto const &i: *indicators) {
            if (indicator == i.first) {
                if constexpr(std::is_same_v<T, float>) {
                    return i.second == DataType::Float;
                } else if constexpr(std::is_same_v<T, bool>) {
                    return i.second == DataType::Bool;
                } else if constexpr(std::is_same_v<T, int>) {
                    return i.second == DataType::Int;
                }
            }
        }
        return false;
    }

    ArduinoJson::DynamicJsonDocument json{1024};
    std::vector<std::pair<String, DataType>> const *indicators = nullptr;
};

class IWorkMode {
public:
    virtual void onInit(Transmitter &transmitter) = 0;

    virtual void onRelease(Transmitter &transmitter) = 0;

    virtual void onReceive(Transmitter &transmitter, String const &parameter, int val) = 0;

    virtual void onReceive(Transmitter &transmitter, String const &parameter, float val) = 0;

    virtual void onReceive(Transmitter &transmitter, String const &parameter, bool val) = 0;

    virtual void onUpdate(Transmitter &transmitter) = 0;
};

class Device {
public:
    bool setup(String const &deviceType, IClient *newClient);

    template<typename T>
    bool setup(String const &deviceType) {
        static T connection;
        return setup(deviceType, &connection);
    }

    template<typename T>
    bool workMode(String const &name) {
        static T mode;
        return workMode(name, &mode);
    }

    bool workMode(String const &name, IWorkMode *handler);

    void update();

    template<typename... Types>
    void print(Types &&... args) {
        (Serial.print(args), ...);
    }

    template<typename T>
    void println(T &&arg) {
        Serial.println(arg);
    }

    template<typename T, typename... Types>
    void println(T &&arg, Types &&... args) {
        Serial.print(arg);
        println(args...);
    }

private:
    struct WorkModeData {
        String name;
        IWorkMode *handler = nullptr;
        unsigned liveTime = 0;
        unsigned sleepTime = 0;
        unsigned receiveInterval = 0;
        std::vector<std::pair<String, DataType>> indicators;
        std::vector<std::pair<String, DataType>> parameters;
    };


    void receive(String const &payload);

    void error(String const &from, String const &message);

    void success(String const &from);

    static DataType parseDataType(String const &type);

    void deviceTypeInfo(String const &commandName, DynamicJsonDocument const &json);

    void transmitData(String const &commandName, DynamicJsonDocument const &json);

    void setWorkMode(String const &commandName, DynamicJsonDocument const &json);

    Connection connection;
    std::vector<WorkModeData> workModes;
    WorkModeData *currentWorkMode = nullptr;
    Transmitter transmitter;
    IClient *client = nullptr;
    unsigned wakeupTime = 0;
    unsigned receiveTime = 0;
    bool configuring = true;
};

extern Device device;
