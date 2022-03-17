#include "BlinkingDiode.hpp"

void BlinkingDiode::loop() {
    if(delay_led == 0)
    {
        return;
    }

    if (millis() - led_time >= delay_led) {
        led_time = millis();

        if (led_PWM >= 0) {
            analogWrite(2, led_PWM);

            if (led_PWM > step) {
                led_PWM = -step;
            }
        } else {
            analogWrite(2, -led_PWM);
        }
        ++led_PWM;
    }
}
