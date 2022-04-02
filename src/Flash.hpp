#pragma once
#include <Arduino.h>
#include <EEPROM.h>

// ЕСЛИ РАЗМЕРЫ СТРОКИ ДРУГИЕ, ТО ВЫПОЛНЯЕМ ПОСЛЕДОВАТЕЛЬНУЮ ПЕРЕЗАПИСЬ
// (ВЫЗВАЕМ ВСЕ write, которые принимают строки)
class Flash {
public:
    void begin();

    void write();

    void writeConfigured(bool val);

    void writeSleepTime(unsigned val);

    void writeSSID(String const &val);

    void writePassword(String const &val);

    void writeIP(String const &val);

    void writePort(String const &val);

    bool readConfigured();

    unsigned readSleepTime();

    String readSSID();

    String readPassword();

    String readIP();

    unsigned readPort();

    bool hasMemory() {
        return hasMem;
    }

    void resetReboot();

private:
    unsigned configuredOffset = 0;
    unsigned sleepOffset = 0;
    unsigned ssidOffset = 0;
    unsigned passwordOffset = 0;
    unsigned ipOffset = 0;
    unsigned portOffset = 0;
    bool hasMem = false;
};

extern Flash flash;