#include "HTTPConnection.hpp"
#include "SmartESP.hpp"

#include <DHT.h>
#define DHTPIN 15


class NormalMode : public IWorkMode {
    DHT dht{DHTPIN, DHT11};
    unsigned sendInterval = 5000;
public:
    void onPhysicalInit(Transmitter &transmitter) override {
        utils::println("onPhysicalInit");
        dht.begin();
    }

    void onInit(Transmitter &transmitter) override {
        float hum = dht.readHumidity(); //Измеряем влажность
        float temp = dht.readTemperature(); //Измеряем температуру

        if(isnan(hum))
        {
            transmitter.transmit("humidity", 0.f);
        } else
        {
            transmitter.transmit("humidity", hum);
        }

        if(isnan(temp))
        {
            transmitter.transmit("temperature", 0.f);
        } else
        {
            transmitter.transmit("temperature", hum);
        }

        utils::println("onInit");
        transmitter.sleep(5000);
    }
};

void setup() {
    esp.setup(new HTTPConnection, 1, 2);
    esp.workMode<NormalMode>("dht11", "send_on_time");
}

void loop() {
    esp.update();
}
