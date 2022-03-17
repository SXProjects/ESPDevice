#pragma once

#include <Arduino.h>

class BlinkingDiode {
public:
    void setup(unsigned millis) {
        pinMode(2, OUTPUT);
        delay_led = millis / (step * 2);
    }

    void loop();

private:
    unsigned delay_led = 20;
    const int step = 300;
    int led_PWM = -step;
    unsigned led_time = 0;
};

