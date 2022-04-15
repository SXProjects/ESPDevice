#pragma once

#include "Device.hpp"

class SmartESP {
public:
    void setup(uint8_t buttonPin,IClient *newClient, unsigned uniqueId, unsigned version);

    bool workMode(String const &deviceType, String const &name, IWorkMode *handler);

    template<typename T>
    bool workMode(String const &deviceType, String const &name) {
        return workMode(deviceType, name, new T);
    }

    void update();

private:
    void error(String const &from, String const &message);

    bool messageFrom(String const &from, String const &message = "", bool isGet = false);

    bool message(String const &message, bool isGet = false);

    bool message(bool isGet = false);

    bool checkJsonField(ArduinoJson::DynamicJsonDocument const &json, String const &f);

    void processCommand();

    bool configureDevices(String const &commandName);

    bool transmitData(String const &commandName);

    bool setWorkMode(String const &commandName);

    Device *findDevice(unsigned deviceId);

    void sleep(unsigned ms);

    void release();

    uint64_t timeSinceUp();

    uint64_t lastSleepTime();

    bool updateConfig();

    bool saveConfig();

    bool deserializeConfig();

    String devName;
    unsigned uniqueId = -1;
    unsigned version = -1;
    unsigned lastGetTime = 0;
    unsigned getInterval = 5000;

    Connection connection;
    IClient *client = nullptr;
    std::vector<Device> devices;
    DynamicJsonDocument lastJson{512};
    String room;

};

extern SmartESP esp;