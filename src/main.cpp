#include "Device.hpp"
#include "HTTPConnection.hpp"

class NormalMode : public IWorkMode
{
public:
    void onInit(Transmitter &transmitter) override {
        device.println("onInit");
    }

    void onRelease(Transmitter &transmitter) override {
        device.println("onRelease");
    }

    void onReceive(Transmitter &transmitter, const String &parameter, int val) override {
        transmitter.transmit("SUPERMEGAINDICATOR", 228);
        device.println("onReceive int");
    }

    void onReceive(Transmitter &transmitter, const String &parameter, float val) override {
        transmitter.transmit("SUPERMEGAINDICATOR", 42);
        device.println("onReceive float");
    }

    void onReceive(Transmitter &transmitter, const String &parameter, bool val) override {
        transmitter.transmit("SUPERMEGAINDICATOR", 322);
        device.println("onReceive bool");
    }

    void onUpdate(Transmitter &transmitter) override {
        device.println("onUpdate");
    }
};

void setup() {
    device.setup<HTTPConnection>("test");
    device.workMode<NormalMode>("NormalMode");
}

void loop() {
    device.update();
}
