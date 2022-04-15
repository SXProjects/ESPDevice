#pragma once

#include <Arduino.h>

class Diode {
public:
    void setup() {
        pinMode(diodePin, OUTPUT);
    }

    void blink(unsigned millis) {
        //Serial.println("diode: blink");
        delayLed = 0;
        blinkInterval = millis;
    }

    void smoothly(unsigned millis) {
        if (millis < unsigned(step * 2)) {
            //Serial.println("smoothly millis must be greater than 600");
        }
        //Serial.println("diode: smoothly");
        delayLed = millis / (step * 2);
        blinkInterval = 0;
    }

    void off() {
        //Serial.println("diode: off");
        delayLed = 0;
        blinkInterval = 0;
    }

    void on() {
        //Serial.println("diode: on");
        delayLed = 1;
        blinkInterval = 1;
    }

    void update();

private:
    unsigned const diodePin = 2;
    unsigned blinkInterval = 0;
    unsigned delayLed = 0;
    const int step = 300;
    int led_PWM = -step;
    unsigned lastTime = 0;
};

extern Diode diode;
