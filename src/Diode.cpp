#include "Diode.hpp"

void Diode::update() {
    if (!delayLed && !blinkInterval) {
        digitalWrite(diodePin, LOW);
        return;
    }

    if (delayLed && blinkInterval) {
        digitalWrite(diodePin, HIGH);
    }

    if (delayLed && (millis() - lastTime > delayLed)) {
        lastTime = millis();

        if (led_PWM >= 0) {
            analogWrite(diodePin, led_PWM);

            if (led_PWM > step) {
                led_PWM = -step;
            }
        } else {
            analogWrite(diodePin, -led_PWM);
        }
        ++led_PWM;
        return;
    }

    if (blinkInterval) {
        if ((millis() - lastTime > blinkInterval)) {
            lastTime = millis();
            digitalWrite(diodePin, HIGH);
        } else {
            digitalWrite(diodePin, LOW);
        }
        return;
    }
}

Diode diode = {};