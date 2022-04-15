#include "HTTPConnection.hpp"
#include "SmartESP.hpp"

#include <DHT.h>

class DHT11Sensor : public IWorkMode {
    uint8_t const pin = 32;
    DHT dht{pin, DHT11};
public:
    void onPhysicalInit(Transmitter &transmitter) override {
        utils::println("DHT11Setup");
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
            transmitter.transmit("temperature", temp);
        }

        transmitter.sleep(5000);
    }
};

class GasSensor : public IWorkMode
{
    uint8_t const pin = 33;
public:
    void onPhysicalInit(Transmitter &transmitter) override
    {
        utils::println("GasSensorSetup");
        pinMode(pin, INPUT);
    }

    void onInit(Transmitter &transmitter) override {
        if(analogRead(pin) > 300)
        {
            transmitter.transmit("gas", true);
        } else
        {
            transmitter.transmit("gas", false);
        }
        transmitter.sleep(5000);
    }
};

class IKSensor : public IWorkMode
{
    uint8_t const trig = 4;
    uint8_t const echo = 2;
public:
    void onPhysicalInit(Transmitter &transmitter) override
    {
        utils::println("IKSensorSetup");
        pinMode(trig, OUTPUT);
        pinMode(echo, INPUT);
    }

    void onInit(Transmitter &transmitter) override {
        digitalWrite(trig, LOW);
        delayMicroseconds(5);
        digitalWrite(trig, HIGH);

        // Выставив высокий уровень сигнала, ждем около 10 микросекунд. В этот момент датчик будет посылать сигналы с частотой 40 КГц.
        delayMicroseconds(10);
        digitalWrite(trig, LOW);

        //  Время задержки акустического сигнала на эхолокаторе.
        double duration = pulseIn(echo, HIGH);

        // Теперь осталось преобразовать время в расстояние
        double cm = (duration / 2.0) / 29.1;

        transmitter.transmit("distance", cm);
        transmitter.sleep(5000);
    }
};

class IndicatorDiodes : public IWorkMode
{
    uint8_t const green = 14;
    uint8_t const red = 12;
    bool active = true;
public:
    void onPhysicalInit(Transmitter &transmitter) override
    {
        utils::println("IKSensorSetup");
        pinMode(green, OUTPUT);
        pinMode(red, OUTPUT);
    }

    void onReceive(Transmitter &transmitter, const String &parameter, bool val) override {
        utils::println(parameter, " ", val);
        if(parameter == "red")
        {
            digitalWrite(red, val);
        }

        if(parameter == "green")
        {
            digitalWrite(green, val);
        }
    }
};

void setup() {
    esp.setup(13, new HTTPConnection, 1, 2);
    esp.workMode<DHT11Sensor>("dht11", "send_on_time");
    esp.workMode<GasSensor>("gas", "send_on_time");
    esp.workMode<IKSensor>("distance", "send_on_time");
    esp.workMode<IndicatorDiodes>("indicators", "send_on_time");
}

void loop() {
    esp.update();
}
