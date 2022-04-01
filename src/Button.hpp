#pragma once

#include "Arduino.h"

class Button {
public:
    void setup(int pin);

    void update();

    unsigned startTime() {
        if (!buttonState) {
            return 0;
        }
        return startPressed;
    }

    unsigned endTime() {
        if (!buttonState) {
            return 0;
        }
        if (endPressed <= startPressed) {
            return millis();
        } else {
            return endPressed;
        }
    }

private:
    int buttonPin = 0;
    int buttonState = 0;
    int lastButtonState = 0;
    int startPressed = 0;
    int endPressed = 0;
};

extern Button button;

