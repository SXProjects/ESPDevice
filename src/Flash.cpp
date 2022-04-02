#include "Flash.hpp"

unsigned const MAX_MEMORY = 256;

void saveInMemory(String const &s, unsigned address, String const& name) {
    for (size_t i = 0; i < s.length(); ++i) {
        EEPROM.write(address + i, s[i]);
    }
    EEPROM.write(s.length() + address, '\0');

    Serial.print("Writing " + name + " to memory: " + s);
}

void saveInMemoryUnsigned(unsigned val, unsigned address, String const& name) {
    auto *bytepointer = reinterpret_cast<uint8_t *>(&val);

    for (int i = 0; i < 4; i++) {
        EEPROM.write(address + i, bytepointer[i]);
    }

    Serial.print("Writing " + name + " to memory: " + String(val));
}

unsigned readFromMemoryUnsigned(unsigned address, String const &name) {
    Serial.print("Reading " + name + " from memory: ");

    uint8_t buffer[4];
    for (int i = 0; i < 4; i++) {
        buffer[i] = EEPROM.read(address + i);
    }

    unsigned result = (unsigned char) (buffer[0]) << 24 |
            (unsigned char) (buffer[1]) << 16 |
            (unsigned char) (buffer[2]) << 8 |
            (unsigned char) (buffer[3]);

    Serial.println(result);
    return result;
}

String readFromMemory(unsigned address, String const &name) {
    Serial.print("Reading " + name + " from memory: ");

    int i = 0;
    String result;
    while (true) {
        uint8_t c = EEPROM.read(address + i);
        if (c == '\0')
            break;
        if (i == 25) {
            return {};
        }
        result += char(c);
        ++i;
    }

    if (result == "") {
        Serial.println("error");
        flash.resetReboot();
    } else {
        Serial.println("result");
    }

    return result;
}

void Flash::begin() {
    EEPROM.begin(MAX_MEMORY);
    hasMem = true;
}

void Flash::write() {
    EEPROM.commit();
}

void Flash::writeConfigured(bool val) {
    if (val) {
        EEPROM.write(0, 42);
    } else {
        EEPROM.write(0, 0);
    }
    sleepOffset = 1;
}

bool Flash::readConfigured() {
    return EEPROM.read(0) == 42;
}

void Flash::writeSleepTime(unsigned int val) {
    saveInMemoryUnsigned(val, sleepOffset, "sleepTime");
    ssidOffset = sleepOffset + 4;
}

unsigned Flash::readSleepTime() {
    return readFromMemoryUnsigned(sleepOffset, "sleepTime");
}

void Flash::writeSSID(const String &val) {
    saveInMemory(val, ssidOffset, "ssid");
    passwordOffset = ssidOffset + val.length() + 1;
}

String Flash::readSSID() {
    return readFromMemory(ssidOffset, "ssid");
}

void Flash::writePassword(const String &val) {
    saveInMemory(val, passwordOffset, "password");
    ipOffset = passwordOffset + val.length() + 1;
}

String Flash::readPassword() {
    return readFromMemory(passwordOffset, "password");
}

void Flash::writeIP(const String &val) {
    saveInMemory(val, ipOffset, "ip");
    portOffset = ipOffset + val.length() + 1;
}

String Flash::readIP() {
    return readFromMemory(ipOffset, "ip");
}

void Flash::writePort(const String &val) {
    if (portOffset + val.length() + 1 >= MAX_MEMORY) {
        hasMem = false;
    } else {
        saveInMemory(val, portOffset, "port");
        hasMem = true;
    }
}

unsigned Flash::readPort() {
    return readFromMemory(portOffset, "port").toInt();
}

void Flash::resetReboot() {
    begin();
    writeConfigured(false);
    writeSleepTime(0);
    write();
    ESP.reset();
}

Flash flash = {};