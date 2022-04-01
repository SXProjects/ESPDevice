#pragma once

#include "Device.hpp"

class SmartESP {
public:
    void setup(IClient *newClient, unsigned uniqueId);

    bool workMode(String const &deviceType, String const &name, IWorkMode *handler);

    template<typename T>
    bool workMode(String const &deviceType, String const &name) {
        static T mode;
        return workMode(deviceType, name, new T);
    }

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
    bool connect();

    void error(String const &from, String const &message, bool isGet = false);

    bool message(String const &from, String const &message = "", bool isGet = false);

    bool checkJsonField(ArduinoJson::DynamicJsonDocument const &json, String const &f);

    void processCommand();

    bool configureDevices(String const &commandName);

    bool transmitData(String const &commandName);

    bool setWorkMode(String const &commandName);

    void sleep(unsigned ms);

    unsigned uniqueId;
    Connection connection;
    IClient *client = nullptr;
    std::vector<Device> devices;
    DynamicJsonDocument lastJson{512};
    unsigned lastTime = 0;
    unsigned sleepTime = 0;
};
