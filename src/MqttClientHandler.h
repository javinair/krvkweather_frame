// MqttClientHandler.h
#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include <AsyncMqttClient.h>
#include "Debugger.h"

class MqttClientHandler {
public:
    MqttClientHandler(const char* server, uint16_t port, const char* topic, Debugger debugger);

    void setup(); // Si quieres mantener funciones recurrentes, aunque en este caso puede no ser necesario
    std::function<void(const char*)> onMessageReceived;

private:
    AsyncMqttClient mqttClient;
    const char* mqttServer;
    uint16_t mqttPort;
    const char* topic;
    // Debug
    Debugger debugger;    

    void connectToMqtt();

    // Callbacks
    void onMqttConnect(bool sessionPresent);
    void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
    void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
};

#endif
