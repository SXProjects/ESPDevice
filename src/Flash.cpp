#include "Flash.hpp"

unsigned const MAX_MEMORY = 1024;


void Flash::begin() {
    EEPROM.begin(MAX_MEMORY);
    hasMem = true;
}

void Flash::commit() {
    EEPROM.write(offset, '\0');
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

void Flash::writeLastSleepTimePoint(unsigned int val) {
    saveInMemoryUnsigned(val, sleepOffset, "sleepTime");
    ssidOffset = sleepOffset + 4;
}

unsigned Flash::readLastSleepTimePoint() {
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
    writeLastSleepTimePoint(0);
    ESP.reset();
}

bool Flash::read() {
    offset = 0;
    EEPROM.begin(MAX_MEMORY);
    while (true) {
        FlashData data;
        data.key = readKeyType(offset, data.type);
        offset += data.key.length() + 5;
        if (data.type == DataType::Undefined) {
            Serial.println("Undefined data type");
            resetReboot();
        } else if ((unsigned) data.type > 3) {
            data.values = readFromMemoryArray(offset, DataType((unsigned) data.type - 3));
            offset += data.values.size() * dataTypeSize(data.type) + 1;
        } else {
            readFromMemory(offset, data.value, data.type);
            offset += dataTypeSize(data.type);
        }

        flashData.push_back(data);
        if(EEPROM.read(offset) == '\0')
        {
            break;
        }
    }
    EEPROM.end();
    return true;
}

DataValue Flash::readFromMemory(unsigned int address, DataType type) {
    DataValue val = {};
    switch (type) {
        case DataType::Float:
            val.f = readFromMemory<float>(address);
            break;
        case DataType::Int:
            val.i = readFromMemory<int>(address);
            break;
        case DataType::Bool:
            val.b = readFromMemory<bool>(address);
            break;
        default:
            Serial.println("Undefined type error");
            flash.resetReboot();
            break;
    }
    return val;
}

std::vector<DataValue> Flash::readFromMemoryArray(unsigned int address, DataType type) {
    int i = 0;
    std::vector<DataValue> result;

    while (true) {
        if (EEPROM.read(address)) {
            break;
        }

        DataValue val = readFromMemory(address + i * dataTypeSize(type), val, type);
        if (i == 25) {
            return {};
        }

        result.push_back(val);
        ++i;
    }

    if (result.empty()) {
        Serial.println("-array without ending");
        flash.resetReboot();
    } else {
        Serial.println();
    }

    return result;
}

unsigned Flash::dataTypeSize(DataType type) {
    switch (type) {
        case DataType::Undefined:
            return 0;
        case DataType::Float:
            return 4;
        case DataType::Int:
            return 4;
        case DataType::Bool:
            return 1;
    }
}

String Flash::readKeyFromMemory(unsigned int address) {
    Serial.print("Reading key from memory: ");

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
        return "";
    } else {
        Serial.print(result);
        Serial.print(", Value: ");
    }

    return result;
}

void Flash::saveKeyInMemory(const String &s, unsigned int address) {
    for (size_t i = 0; i < s.length(); ++i) {
        EEPROM.write(address + i, s[i]);
    }
    EEPROM.write(s.length() + address, '\0');

    Serial.print("Writing key to memory: " + s);
    Serial.print(", Value: ");
}

String Flash::readKeyType(unsigned int address, DataType &type) {
    String name;
    type = DataType::Undefined;
    name = readKeyFromMemory(address);
    if (name.isEmpty()) {
        flash.resetReboot();
    }
    type = readFromMemory<DataType>(address + name.length() + 1);
    return name;
}

void Flash::end() {
    EEPROM.end();
}

void writeValues(std::vector<unsigned> const &times) {

}

Flash flash = {};