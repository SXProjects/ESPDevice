#pragma once

#include "Device.hpp"

class SmartESP {
public:
    void setup(IClient *newClient, unsigned uniqueId, unsigned version);

    bool workMode(String const &deviceType, String const &name, IWorkMode *handler);

    template<typename T>
    bool workMode(String const &deviceType, String const &name) {
        static T mode;
        return workMode(deviceType, name, new T);
    }

    void update();

private:
    void error(String const &from, String const &message, bool isGet = false);

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
    bool configured = false;
    bool updated = false;
    bool offline = true;

    Connection connection;
    IClient *client = nullptr;
    std::vector<Device> devices;
    DynamicJsonDocument lastJson{512};
    bool fatal = false;

//    unsigned lastSleepTimePoint = 0;
//    unsigned sleepTime = 0;
//    unsigned timeSinceServerUp = 0;
};

extern SmartESP esp;