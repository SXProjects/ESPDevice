#include "HTTPConnection.hpp"
#include "SmartESP.hpp"

class NormalMode : public IWorkMode {
public:
    void onInit(Transmitter &transmitter) override {
        utils::println("onInit");
    }

    void onRelease(Transmitter &transmitter) override {
        utils::println("onRelease");
    }

    void onReceive(Transmitter &transmitter, const String &parameter, int val) override {
        transmitter.transmit("SUPERMEGAINDICATOR", 228);
        utils::println("onReceive int");
    }

    void onReceive(Transmitter &transmitter, const String &parameter, float val) override {
        transmitter.transmit("SUPERMEGAINDICATOR", 42);
        utils::println("onReceive float");
    }

    void onReceive(Transmitter &transmitter, const String &parameter, bool val) override {
        transmitter.transmit("SUPERMEGAINDICATOR", 322);
        utils::println("onReceive bool");
    }

    void onUpdate(Transmitter &transmitter) override {
        utils::println("onUpdate");
    }
};

void setup() {
    esp.setup(new HTTPConnection, 1, 1);
    esp.workMode<NormalMode>("thermometer", "on_time");
}

void loop() {
    esp.update();
}
