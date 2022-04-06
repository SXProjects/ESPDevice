#pragma once

#include <Arduino.h>
#include <EEPROM.h>
#include <Utils.hpp>
#include <ArduinoJson.hpp>
#include <vector>

enum class DataType : unsigned {
    Undefined,
    Float,
    Int,
    Bool,
    Char,
    ArrayFloat,
    ArrayInt,
    ArrayBool,
    ArrayChar,
};

using Char = char;

union DataValue {
    float f;
    int i;
    bool b;
    Char c;
};

struct FlashData {
    String key;
    DataType type;
    DataValue value;
    std::vector<uint8_t> values;
};

class Flash {
public:
    void beginWrite();

    void endWrite();

    bool read();

    bool getVerifyByte() {
        return EEPROM.read(0) == 42;
    }

    void setVerifyByte(bool v) {
        if (v) {
            EEPROM.write(0, 42);
        } else {
            EEPROM.write(0, 0);
        }
    }

    template<typename T>
    void save(String const &name, T val) {
        using Type = std::decay_t<T>;
        unsigned addr = 0;
        for (auto &d: flashData) {
            if (d.key == name) {
                saveInMemory(val, addr, name);

                if constexpr(std::is_same_v<Type, float>) {
                    if (d.type == DataType::Float) {
                        d.value.f = val;
                    }
                } else if constexpr(std::is_same_v<Type, int>) {
                    if (d.type == DataType::Int) {
                        d.value.i = val;
                    }
                } else if constexpr(std::is_same_v<Type, bool>) {
                    if (d.type == DataType::Bool) {
                        d.value.b = val;
                    }
                } else if constexpr(std::is_same_v<Type, Char>) {
                    if (d.type == DataType::Char) {
                        d.value.c = val;
                    }
                }
                return;
            }

            if (d.values.empty()) {
                addr += d.key.length() + 5 + dataTypeSize(d.type);
            } else {
                addr += d.key.length() + 6 + d.values.size();
            }
        }
    }

    template<typename T>
    void save(String const &name, T const *arr, size_t size) {
        saveInMemoryArray(arr, size, offset, name);
        offset += name.length() + 6 + sizeof(T) * size;
    }

    template<typename T>
    bool get(String const &name, T &res) {
        using Type = std::decay_t<T>;
        for (auto const &d: flashData) {
            if (d.key == name) {
                if (d.value.c == '\0') {
                    return false;
                }
                if constexpr(std::is_same_v<Type, float>) {
                    if (d.type == DataType::Float) {
                        res = d.value.f;
                        return true;
                    }
                } else if constexpr(std::is_same_v<Type, int>) {
                    if (d.type == DataType::Int) {
                        res = d.value.i;
                        return true;
                    }
                } else if constexpr(std::is_same_v<Type, bool>) {
                    if (d.type == DataType::Bool) {
                        res = d.value.b;
                        return true;
                    }
                } else if constexpr(std::is_same_v<Type, Char>) {
                    if (d.type == DataType::Char) {
                        res = d.value.c;
                        return true;
                    }
                }
            }
        }
        return false;
    }

    template<typename T>
    bool clear(String const &name) {
        unsigned addr = 0;
        for (auto &d: flashData) {
            if (d.key == name) {
                for (int i = 0; i < sizeof(T); ++i) {
                    EEPROM.write(addr + d.key.length() + 5 + i, '\0');
                }
                d.value.c = '\0';
                return true;
            }

            if (d.values.empty()) {
                addr += d.key.length() + 5 + dataTypeSize(d.type);
            } else {
                addr += d.key.length() + 6 + d.values.size();
            }
        }
        return false;
    }

    template<typename T>
    bool get(String const &name, T const *&begin, size_t &count) {
        using Type = std::decay_t<T>;

        for (auto const &d: flashData) {
            if (d.key == name) {
                if constexpr(std::is_same_v<Type, float>) {
                    if (d.type == DataType::ArrayFloat) {
                        begin = reinterpret_cast<float const *>(&d.values.front());
                        count = d.values.size() / sizeof(float);
                        return true;
                    }
                } else if constexpr(std::is_same_v<Type, int>) {
                    if (d.type == DataType::ArrayInt) {
                        begin = reinterpret_cast<int const *>(&d.values.front());
                        count = d.values.size() / sizeof(int);
                        return true;
                    }
                } else if constexpr(std::is_same_v<Type, bool>) {
                    if (d.type == DataType::ArrayBool) {
                        begin = reinterpret_cast<bool const *>(&d.values.front());
                        count = d.values.size() / sizeof(bool);
                        return true;
                    }
                } else if constexpr(std::is_same_v<Type, Char>) {
                    if (d.type == DataType::ArrayChar) {
                        begin = reinterpret_cast<Char const *>(&d.values.front());
                        count = d.values.size() / sizeof(Char);
                        return true;
                    }
                }
            }
        }
        return false;
    }

    char const* readString(String const &name);

    ArduinoJson::DynamicJsonDocument const &readJson(String const &name);

    void saveString(String const &name, String const &str);

    void saveJson(String const &name, ArduinoJson::DynamicJsonDocument const &doc);

    void reset();

private:
    DataValue readFromMemory(unsigned int address, DataType type);

    template<typename T>
    void saveType(unsigned address, bool isArray) {
        DataType type = DataType::Undefined;

        if constexpr(std::is_same_v<T, float>) {
            type = DataType::Float;
        } else if constexpr(std::is_same_v<T, int>) {
            type = DataType::Int;
        } else if constexpr(std::is_same_v<T, bool>) {
            type = DataType::Bool;
        } else if constexpr(std::is_same_v<T, char>) {
            type = DataType::Char;
        } else {
            utils::reset("Save type undefined type error");
        }

        if (isArray) {
            saveInMemory((unsigned) (type) + 4, address);
        } else {
            saveInMemory((unsigned) type, address);
        }
    }

    template<typename T>
    void saveInMemory(T val, unsigned address, String const &name) {
        unsigned addLength = 0;
        if (!name.isEmpty()) {
            saveKeyInMemory(name, address);
            saveType<T>(address + name.length() + 1, false);
            addLength = name.length() + 5;
        }

        saveInMemory(val, address + addLength);
        if (name.isEmpty()) {
            utils::print(val, " ");
        } else {
            utils::println(val);
        }
    }

    template<typename T>
    void saveInMemoryArray(T const *val, size_t size, unsigned address, String const &name) {
        saveKeyInMemory(name, address);
        saveType<T>(address + name.length() + 1, true);

        unsigned arrOffset = address + name.length() + 5;
        for (size_t i = 0; i < size; ++i) {
            saveInMemory<T>(val[i], arrOffset + i * sizeof(T), "");
        }
        EEPROM.write(size * sizeof(T) + arrOffset, '\0');
        Serial.println();
    }

    String readKeyType(unsigned address, DataType &type);

    std::vector<uint8_t> readFromMemoryArray(unsigned address, DataType type);

    unsigned dataTypeSize(DataType type);

    String readKeyFromMemory(unsigned address);

    void saveKeyInMemory(String const &s, unsigned address);

    template<typename T>
    T readFromMemory(unsigned address) {
        T val;
        uint8_t buffer[sizeof(T)];
        for (size_t i = 0; i < sizeof(T); i++) {
            buffer[i] = EEPROM.read(address + i);
        }

        memcpy(&val, &buffer, sizeof(T));
        return val;
    }

    template<typename T>
    void saveInMemory(T val, unsigned address) {
        auto bytepointer = reinterpret_cast<uint8_t *>(&val);

        for (size_t i = 0; i < sizeof(T); i++) {
            EEPROM.write(address + i, bytepointer[i]);
        }
    }

    std::vector<FlashData> flashData;
    ArduinoJson::DynamicJsonDocument json{512};
    unsigned offset = 1;
};

extern Flash flash;