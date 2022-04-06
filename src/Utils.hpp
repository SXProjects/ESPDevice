#pragma once
#include <Arduino.h>

namespace utils
{
    template<typename T>
    void print(T &&arg) {
        Serial.print(arg);
    }

    template<typename T, typename... Types>
    void print(T &&arg, Types &&... args) {
        Serial.print(arg);
        print(args...);
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

    inline void reset(String const& error)
    {
        Serial.println();
        Serial.println(error);
        Serial.println("resetting...");
        ESP.restart();
    }
}