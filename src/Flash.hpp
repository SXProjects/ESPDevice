#pragma once

#include <Arduino.h>
#include <EEPROM.h>

enum class DataType {
    Undefined,
    Float,
    Int,
    Bool,
    ArrayFloat,
    ArrayInt,
    ArrayBool,
};

union DataValue {
    float f;
    int i;
    bool b;
};

struct FlashData {
    String key;
    DataType type;
    DataValue value;
    std::vector<DataValue> values;
};

// ЕСЛИ РАЗМЕРЫ СТРОКИ ДРУГИЕ, ТО ВЫПОЛНЯЕМ ПОСЛЕДОВАТЕЛЬНУЮ ПЕРЕЗАПИСЬ
// (ВЫЗВАЕМ ВСЕ write, которые принимают строки)
class Flash {
public:
    void write();

    void commit();

    bool read();

    template<typename T>
    void save(String const &name, T val) {
        saveInMemory(val, offset, name);
        offset += name.length() + 5 + sizeof(T);
    }

    template<typename T>
    void save(String const &name, std::vector<T> const &arr) {
        saveInMemoryArray(arr, offset, name);
        offset = name.length() + 5 + sizeof(T) * arr.size();
    }

    void writeConfigured(bool val);

    void writeLastSleepTimePoint(unsigned val);

    void writeSSID(String const &val);

    void writePassword(String const &val);

    void writeIP(String const &val);

    void writePort(String const &val);

    bool readConfigured();

    unsigned readLastSleepTimePoint();

    String readSSID();

    String readPassword();

    String readIP();

    unsigned readPort();

    void resetReboot();

    void writeValues(std::vector<unsigned> const &times);

    std::vector<unsigned> writeSleepTimes();

private:
    DataValue readFromMemory(unsigned int address, DataType type);

    template<typename T>
    static void saveType(unsigned offset, bool isArray) {
        DataType type = DataType::Undefined;

        if constexpr(std::is_same_v<T, float>) {
            type = DataType::Float;
        } else if constexpr(std::is_same_v<T, int>) {
            type = DataType::Int;
        } else if constexpr(std::is_same_v<T, bool>) {
            type = DataType::Bool;
        } else {
            Serial.println("Undefined type error");
            flash.resetReboot();
        }

        if (isArray) {
            saveInMemory(DataType((unsigned) (type) + 3), offset);
        } else {
            saveInMemory(type, offset);
        }
    }

    template<typename T>
    void saveInMemory(T val, unsigned address, String const &name) {
        unsigned addLength = 0;
        if (!name.isEmpty()) {
            saveKeyInMemory(name, address);
            saveType<T>(address + name.length() + 1);
            addLength = name.length() + 5;
        }

        saveInMemory(val, address + addLength);
        Serial.println(val);
        offset = address + addLength + sizeof(T);
    }

    template<typename T>
    static void saveInMemoryArray(std::vector<T> const &val, unsigned address, String const &name) {
        saveKeyInMemory(name, address);
        saveType<T>(address + name.length() + 1);

        for (size_t i = 0; i < val.size(); ++i) {
            saveInMemory<T>(val[i], address + i * sizeof(T) + name.length() + 1, "");
        }
        EEPROM.write(val.size() * sizeof(T) + address, '\0');
    }

    static String readKeyType(unsigned address, DataType &type);

    static std::vector<DataValue> readFromMemoryArray(unsigned address, DataType type);

    static unsigned dataTypeSize(DataType type);

    static String readKeyFromMemory(unsigned address);

    static void saveKeyInMemory(String const &s, unsigned address);

    template<typename T>
    static T readFromMemory(unsigned address) {
        T val;
        uint8_t buffer[sizeof(T)];
        for (int i = 0; i < sizeof(T); i++) {
            buffer[i] = EEPROM.read(address);
        }

        memcpy(&val, &buffer, sizeof(T));
        return val;
    }

    template<typename T>
    static void saveInMemory(T val, unsigned address) {
        auto bytepointer = reinterpret_cast<uint8_t *>(&val);

        for (int i = 0; i < sizeof(T); i++) {
            EEPROM.write(address, bytepointer[i]);
        }
    }

    std::vector<FlashData> flashData;
    unsigned offset;
};

extern Flash flash;