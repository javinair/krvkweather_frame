// MqttClientHandler.cpp
#include "MqttClientHandler.h"


MqttClientHandler::MqttClientHandler(const char* server, uint16_t port, const char* topic, Debugger debugger)
    : mqttServer(server), mqttPort(port), topic(topic), debugger(debugger) {}

void MqttClientHandler::setup() {

    // Configura el cliente MQTT
    mqttClient.onConnect([this](bool sessionPresent) { this->onMqttConnect(sessionPresent); });
    mqttClient.onDisconnect([this](AsyncMqttClientDisconnectReason reason) { this->onMqttDisconnect(reason); });
    mqttClient.onMessage([this](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
        this->onMqttMessage(topic, payload, properties, len, index, total);
    });
    mqttClient.setServer(mqttServer, mqttPort);
    connectToMqtt();
}


void MqttClientHandler::connectToMqtt() {
    debugger.log(String("Conectando al broker MQTT...").c_str());
    mqttClient.connect();
}

void MqttClientHandler::onMqttConnect(bool sessionPresent) {
    debugger.log("Conectado al broker MQTT.");
    mqttClient.subscribe(topic, 1);  // QoS 1
}

void MqttClientHandler::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    debugger.log("Desconectado del broker MQTT.");
    connectToMqtt();
}

void MqttClientHandler::onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    payload[len] = '\0';    
    debugger.log("Mensaje recibido:");
        // Llama al callback si está definido
    if (onMessageReceived) {
        onMessageReceived(payload);
    }
}
