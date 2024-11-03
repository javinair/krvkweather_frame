#include "MqttClient.h"

MqttClient::MqttClient(const char* server, int port, const char* topic, Debugger& debugger)
    : _server(server), _port(port), _topic(topic), _client(_espClient), _debugger(debugger) {
    _client.setServer(_server, _port);
}

void MqttClient::connectToBroker() {
    while (!_client.connected()) {
        _debugger.log("Conectando al broker MQTT...");
        if (_client.connect("KRVKWeather_Frame_Client")) {
            _debugger.log("Conectado al broker MQTT");
            subscribe();
        } else {
            _debugger.log(("Falló la conexión, estado=" + String(_client.state()) + ". Reintentando en 5 segundos...").c_str());
            delay(5000);
        }
    }
}

void MqttClient::subscribe() {
    _client.subscribe(_topic);
    _debugger.log("Suscrito al topic MQTT");
}

void MqttClient::setCallback(MQTT_CALLBACK_SIGNATURE) {
    _client.setCallback(callback);
}

void MqttClient::loop() {
    _client.loop();
}
