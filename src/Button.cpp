#include "Button.hpp"

void Button::update() {
    buttonState = !digitalRead(buttonPin);

    if (buttonState != lastButtonState) {
        if (buttonState == HIGH) {
            startPressed = millis();
        } else {
            endPressed = millis();
        }
    }

    lastButtonState = buttonState;
}

void Button::setup(int pin) {
    buttonPin = pin;
    Serial.print("Button INIT ");
    Serial.println(buttonPin);
    pinMode(pin, INPUT_PULLUP);
}

Button button = {};