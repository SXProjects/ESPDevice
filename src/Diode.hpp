#pragma once

#include <Arduino.h>

class Diode {
public:
    void setup() {
        pinMode(diodePin, OUTPUT);
    }

    void blink(unsigned millis) {
        delayLed = 0;
        blinkInterval = millis;
    }

    void smoothly(unsigned millis) {
        delayLed = millis / (step * 2);
        blinkInterval = 0;
    }

    void off() {
        delayLed = 0;
        blinkInterval = 0;
    }

    void on() {
        delayLed = 1;
        blinkInterval = 1;
    }

    void loop();

private:
    unsigned const diodePin = 2;
    unsigned blinkInterval = 0;
    unsigned delayLed = 0;
    const int step = 300;
    int led_PWM = -step;
    unsigned lastTime = 0;
};

extern Diode diode;
