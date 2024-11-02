#include "wifi_config.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "Debugger.h"

Debugger debugger(IPAddress(192, 168, 0, 10), 12345);

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }

    debugger.init();

    otaConfiguration();
    debugger.log("KRVKWeather Frame ready2go!");
}

void otaConfiguration() {
    ArduinoOTA.setHostname("KRVKWEATHER_FRAME");
    ArduinoOTA.onStart([]() {
        debugger.log("Inicio de actualización OTA...");
    });
    ArduinoOTA.onEnd([]() {
        debugger.log("Actualización OTA completada.");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "Progreso OTA: %u%%", (progress * 100) / total);
        debugger.log(buffer);
    });
    ArduinoOTA.onError([](ota_error_t error) {
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "Error OTA: %u", error);
        debugger.log(buffer);
    });
    ArduinoOTA.begin();
}

void loop() {
    ArduinoOTA.handle();

    // Ejemplo de log de depuración
    debugger.log("Loop ejecutándose...");
    delay(2000);
}
