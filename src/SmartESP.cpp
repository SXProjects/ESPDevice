#include "SmartESP.hpp"

#define JSON_FIELD(JSON, FIELD) \
if (!checkJsonField(JSON, #FIELD)) { \
error(commandName, "field \"" #FIELD "\" is not specified");\
return false; \
} \
auto const& j_##FIELD = JSON [ #FIELD ];

#define JSON_FIELD_V(JSON, FIELD, VAR) \
if (!checkJsonField(JSON, #FIELD)) { \
error(commandName, "field \"" #FIELD "\" is not specified");\
return false; \
} \
auto const& j_##VAR = JSON [ #FIELD ];

void SmartESP::setup(IClient *newClient, unsigned int uId) {
    Serial.begin(115200);
    client = newClient;
    uniqueId = uId;
}

bool SmartESP::workMode(String const &deviceType, String const &name, IWorkMode *handler) {

}

void SmartESP::update() {
    connection.update();

    if (button.endTime() - button.startTime() > 4000) {
        Serial.println("Device reset using the button");
        connection.reset();
        ESP.reset();
    }

    if(connection.isPairing())
    {
        return;
    }

    if (!connection.isConnected()) {
        devices.clear();
        lastJson.clear();
    }

    if (devices.empty() || !connect()) {
        Serial.println("trying to connect again and again... (");
    }

    if (message("wakeup", R"("unique_id":)" + String(uniqueId), true)) {
        processCommand();
    } else {
        error("wakeup", "can't update command (/commands/get)", true);
    }
}

bool SmartESP::configureDevices(String const &commandName) {
    if (!devices.empty()) {
        error(commandName, "device already configured");
        return false;
    }

    JSON_FIELD(lastJson, command_name)
    if (j_command_name != commandName) {
        error(commandName, "device must be configured by device_config_info");
    }
    JSON_FIELD(lastJson, devices)

    for (size_t d = 0; d < j_devices.size(); ++d) {
        JSON_FIELD(j_devices[d], device_id)
        JSON_FIELD_V(j_devices[d], name, device_type)
        JSON_FIELD(j_devices[d], current_work_mode)
        JSON_FIELD(j_devices[d], work_modes)

        for (size_t w = 0; w < j_work_modes.size(); ++w) {
            JSON_FIELD_V(j_work_modes[w], name, work_mode)
            JSON_FIELD(j_work_modes[w], live_time)
            JSON_FIELD(j_work_modes[w], sleep_time)
            JSON_FIELD(j_work_modes[w], indicators)
            JSON_FIELD(j_work_modes[w], parameters)

            for (size_t i = 0; i < j_indicators.size(); ++i) {
                JSON_FIELD(j_indicators[i], name)
                JSON_FIELD(j_indicators[i], type)
            }

            for (size_t j = 0; j < j_parameters.size(); ++j) {
                JSON_FIELD(j_parameters[j], name)
                JSON_FIELD(j_parameters[j], type)
            }
        }
    }

    return message("configureDevices", "", true);
}

bool SmartESP::connect() {
    button.setup(D1);
    if (connection.connect("ESP_" + String(uniqueId), client)) {
        if (!message("device_wakeup", R"("unique_id": )" + String(uniqueId) +
                R"(,"start":true)", true)) {
            Serial.println("invalid device configuration");
            return false;
        }

        diode.smoothly(600);
        Serial.println("configuring devices");
        return configureDevices("device_config_info");
    } else {
        Serial.println("can't connect to serer, waiting for reconnecting");
        return false;
    }
}

void SmartESP::error(const String &from, const String &message, bool isGet) {
    String msg = R"({"command_name": ")" + from + R"(","error":")" + message + "\"}";
    if (isGet) {
        client->send(msg);
    } else {
        client->get(msg);
    }
    Serial.println("Error: " + msg);
}

bool SmartESP::message(const String &from, String const &json, bool isGet) {
    String msg;
    if (json.isEmpty()) {
        msg = R"({"command_name": ")" + from + "\"}";
    } else {
        msg = R"({"command_name": ")" + from + "\"," + json + "}";
    }

    Serial.println("Message: " + msg);
    String answer;
    if (isGet) {
        answer = client->send(msg);
    } else {
        answer = client->get(msg);
    }

    if (answer.isEmpty()) {
        Serial.println("Server answer error");
        return false;
    } else {
        return ArduinoJson::deserializeJson(lastJson, json);
    }
}

bool SmartESP::checkJsonField(ArduinoJson::DynamicJsonDocument const &json, String const &f) {
    if (!json.containsKey(f)) {
        error("configureDevices", "devices is not specified");
        return false;
    }
    return true;
}

void SmartESP::processCommand() {
    if (!lastJson.containsKey("command_name")) {
        return error("undefined", "command_name is not specified");
    }

    String commandName = (char const *) lastJson["command_name"];

    if (commandName == "transmit_data") {
        transmitData(commandName);
    } else if (commandName == "set_work_mode") {
        setWorkMode(commandName);
    }

    return error("undefined", "unidentified command_name");
}

bool SmartESP::transmitData(const String &commandName) {
    JSON_FIELD(lastJson, device_id)
    JSON_FIELD(lastJson, data);

    for (size_t d = 0; d < j_data.size(); ++d) {
        JSON_FIELD(j_data[d], name)
        JSON_FIELD(j_data[d], value)
    }
}

bool SmartESP::setWorkMode(const String &commandName) {
    JSON_FIELD(lastJson, work_mode)
}

void SmartESP::sleep(unsigned int ms) {
    //TODO

    if(ms > ms * 1000 * 60 * 60 * 2)
    {
        Serial.println()
    } else if (ms < 5000) {
        lastTime = millis();
        sleepTime = ms;
    } else {
        ESP.deepSleep(ms * 1000);
    }
}
