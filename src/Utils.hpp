#pragma once
#include <Arduino.h>

namespace utils
{
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

    inline void reset(String const& error)
    {
        Serial.println();
        Serial.println(error);
        Serial.println("resetting...");
        ESP.reset();
    }
}