#include "Flash.hpp"

unsigned const MAX_MEMORY = 1024;


void Flash::beginWrite() {
    EEPROM.begin(MAX_MEMORY);
}

void Flash::endWrite() {
    EEPROM.write(offset, '\0');
    EEPROM.commit();
}

bool Flash::read() {
    offset = 1;
    EEPROM.begin(MAX_MEMORY);
    while (true) {
        if (EEPROM.read(offset) == '\0') {
            break;
        }

        FlashData data;
        data.key = readKeyType(offset, data.type);
        if(data.key.isEmpty())
        {
            return false;
        }
        offset += data.key.length() + 5;
        if (data.type == DataType::Undefined) {
            utils::reset("Undefined data type");
        } else if ((unsigned) data.type > 3) {
            auto type = DataType((unsigned) data.type - 4);
            data.values = readFromMemoryArray(offset, type);
            offset += data.values.size() + 1;
        } else {
            data.value = readFromMemory(offset, data.type);
            offset += dataTypeSize(data.type);
        }
        Serial.println();

        flashData.push_back(data);
    }
    EEPROM.end();
    return true;
}

DataValue Flash::readFromMemory(unsigned int address, DataType type) {
    DataValue val = {};
    if(EEPROM.read(address) == '\0')
    {
        val.c = '\0';
        return val;
    }
    switch (type) {
        case DataType::Float:
            val.f = readFromMemory<float>(address);
            utils::print(val.f, " ");
            break;
        case DataType::Int:
            val.i = readFromMemory<int>(address);
            utils::print(val.i, " ");
            break;
        case DataType::Bool:
            val.b = readFromMemory<bool>(address);
            utils::print(val.b, " ");
            break;
        case DataType::Char:
            val.c = readFromMemory<char>(address);
            utils::print(val.c, " ");
            break;
        default:
            utils::reset("readFromMemory undefined type error");
            break;
    }
    return val;
}

std::vector<uint8_t> Flash::readFromMemoryArray(unsigned int address, DataType type) {
    int i = 0;
    std::vector<uint8_t> result;
    unsigned typeSize = dataTypeSize(type);

    while (true) {
        if (EEPROM.read(address + i * typeSize) == '\0') {
            break;
        }

        DataValue val = readFromMemory(address + i * typeSize, type);
        if (i == MAX_MEMORY) {
            break;
        }

        auto bytepointer = reinterpret_cast<uint8_t *>(&val);

        for (size_t j = 0; j < typeSize; j++) {
            result.push_back(bytepointer[j]);
        }

        ++i;
    }

    if (result.empty()) {
        utils::reset("array without ending");
    }

    return result;
}

unsigned Flash::dataTypeSize(DataType type) {
    switch (type) {
        case DataType::Float:
            return sizeof(float);
        case DataType::Int:
            return sizeof(int);
        case DataType::Bool:
            return sizeof(bool);
        case DataType::Char:
            return sizeof(char);
        default:
            utils::reset("dataTypeSize undefined type error");
            return 0;
    }
}

String Flash::readKeyFromMemory(unsigned int address) {
    utils::print("Reading key from memory: ");

    int i = 0;
    String result;
    while (true) {
        uint8_t c = EEPROM.read(address + i);
        if (c == '\0')
            break;
        if (i == MAX_MEMORY) {
            return {};
        }
        result += char(c);
        ++i;
    }

    if (result == "") {
        utils::println("error");
        return "";
    } else {
        utils::print(result, " Value: ");
    }
    return result;
}

void Flash::saveKeyInMemory(const String &s, unsigned int address) {
    for (size_t i = 0; i < s.length(); ++i) {
        EEPROM.write(address + i, s[i]);
    }
    EEPROM.write(s.length() + address, '\0');

    utils::print("Writing key to memory: ", s, ", Value: ");
}

String Flash::readKeyType(unsigned int address, DataType &type) {
    String name;
    type = DataType::Undefined;
    name = readKeyFromMemory(address);
    if (name.isEmpty()) {
        utils::println("can't read key");
        return "";
    }
    type = (DataType) readFromMemory<unsigned>(address + name.length() + 1);

    return name;
}

void Flash::reset() {
    flashData.clear();
    offset = 1;
    for (unsigned i = 0; i < MAX_MEMORY; ++i) {
        EEPROM.write(i, 0);
    }
}

char const* Flash::readString(const String &name) {
    char const *jp = nullptr;
    size_t js = 0;
    if (get(name, jp, js)) {
        return jp;
    }
    return nullptr;
}

const ArduinoJson::DynamicJsonDocument &Flash::readJson(const String &name) {
    auto str = readString(name);
    if (str != nullptr) {
        ArduinoJson::deserializeJson(json, str);
    } else {
        json.clear();
    }
    return json;
}

void Flash::saveJson(const String &name, const ArduinoJson::DynamicJsonDocument &doc) {
    String res;
    ArduinoJson::serializeJson(doc, res);
    saveString(name, res);
}

void Flash::saveString(const String &name, const String &str) {
    save(name, str.begin(), str.length());
}

Flash flash = {};