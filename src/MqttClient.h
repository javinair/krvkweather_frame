#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <WiFi.h>
#include <PubSubClient.h>
#include "Debugger.h"

class MqttClient {
public:
    MqttClient(const char* server, int port, const char* topic, Debugger& debugger);
    void connectToBroker();
    void subscribe();
    void setCallback(MQTT_CALLBACK_SIGNATURE);
    void loop();

private:
    const char* _server;
    int _port;
    const char* _topic;
    WiFiClient _espClient;
    PubSubClient _client;
    Debugger& _debugger;
};

#endif // MQTTCLIENT_H
