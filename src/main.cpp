#include "wifi_config.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "Debugger.h"
#include <esp_sleep.h>

#define SHORT_PRESS_TIME 500
#define OTA_UPDATE_MS 1000
#define DS_CHECK_MS 10000


Debugger debugger(IPAddress(192, 168, 0, 10), 12345);

int btnLastState = HIGH;
int btnCurrentState;
unsigned long btnPressedTime  = 0;
unsigned long btnReleasedTime = 0;
boolean btnPressedOnAwake = false;

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

void setup() {
    /* WIFI */
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Conectando a WiFi...");
    }
    debugger.init();

    /* OTA */
    otaConfiguration();    

    /* BUTTON */
    pinMode(GPIO_NUM_4, INPUT_PULLUP);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0); // Habilita el wakeup en GPIO4 (0 = LOW)
    btnPressedOnAwake = !digitalRead(GPIO_NUM_4);

    debugger.log("KRVKWeather Frame iniciado");
}

void startDeepSleep(long timeInSeconds) {
    debugger.log("Entrando en modo deep sleep...");
    delay(100);
    WiFi.disconnect(true);
    esp_sleep_enable_timer_wakeup(timeInSeconds * 1000000);
    esp_deep_sleep_start();
}

void loop() {
    static unsigned long lastOTACheck = 0;
    static unsigned long lastDSCheck = 0;    
    unsigned long currentMillis = millis();

    /* OTA */
    if (currentMillis - lastOTACheck >= OTA_UPDATE_MS) {
        lastOTACheck = currentMillis;
        ArduinoOTA.handle(); // Permite las actualizaciones OTA
    }    

    /* BUTTON */
    btnCurrentState = digitalRead(GPIO_NUM_4);
    if (btnLastState == HIGH && btnCurrentState == LOW)       // button is pressed
        btnPressedTime = millis();
    else if (btnLastState == LOW && btnCurrentState == HIGH) { // button is released
        btnReleasedTime = millis();
        long pressDuration = btnReleasedTime - btnPressedTime;
        if ( pressDuration < SHORT_PRESS_TIME )
            debugger.log("Pulsación corta");
        else {
            if(btnPressedOnAwake) {
                debugger.log("Despertado mediante botón");
                btnPressedOnAwake = false;
            } else {
                debugger.log("Pulsación larga estando activo");
            }
        }      
    }
    btnLastState = btnCurrentState;

    // /* DEEP SLEEP */
    // if (currentMillis - lastDSCheck >= DS_CHECK_MS) {
    //     lastDSCheck = currentMillis;    
    //     startDeepSleep(10);
    // }
}
