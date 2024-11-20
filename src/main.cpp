#include "secrets.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "Debugger.h"
#include <esp_sleep.h>
#include "MqttClientHandler.h"
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include "ScreenManager.hpp"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SHORT_PRESS_TIME 500
#define LONG_PRESS_TIME 5000
#define OTA_UPDATE_MS 1000
#define ENABLE_GxEPD2_GFX 0
#define WAKEUP_GPIO GPIO_NUM_9  // Reemplaza por el pin deseado
#define BME280_ADDRESS 0x76 
Adafruit_BME280 bme; // Crear una instancia del sensor

struct FRAME_DATA {
    int wifiStrength;
    float temperature;
    float humidity;
    int batteryLevel;
};

void processData(const char* data);
void processMqttMessage(const char* message);
FRAME_DATA getFrameData();


// EINK
// GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(/*CS=5*/ 2, /*DC=*/ 22, /*RES=*/ 21, /*BUSY=*/ 13)); // 400x300, SSD1683
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(/*CS=5*/ SS, /*DC=*/ 1, /*RES=*/ 2, /*BUSY=*/ 3)); // 400x300, SSD1683


// Debug
Debugger debugger(IPAddress(192, 168, 0, 10), 12345);

// MQTT
MqttClientHandler mqttClientHandler(MQTT_SERVER_IP, MQTT_SERVER_PORT, MQTT_TOPIC, debugger);
ScreenManager screen(display, debugger);

char* jsonEjemplo = "{\"temp\": 20.1, \"hum\": 55.3, \"press\": 1005.58, \"avg_wind_direction\": \"north\", \"avg_wind_speed\": 0.0, \"gust_wind_direction\": \"north\", \"gust_wind_speed\": 0.0, \"uv\": 0, \"rain_last_hour\": 0.0, \"rain_today\": 0.0, \"wifi\": -56,\"battery\": 3.88}";

FRAME_DATA getFrameData() {
    FRAME_DATA frameData;
    frameData.temperature = bme.readTemperature();
    frameData.humidity = bme.readHumidity();
    frameData.wifiStrength = WiFi.RSSI();
    frameData.batteryLevel = 25;    //TODO
    debugger.log(String(frameData.temperature).c_str());
    debugger.log(String(frameData.humidity).c_str());
    debugger.log(String(frameData.wifiStrength).c_str());
    debugger.log(String(frameData.batteryLevel).c_str());
    return frameData;
}

void processData(const char* data) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
        debugger.log("deserializeJson() failed: ");
        debugger.log(error.c_str());
        return;
    }

    float temp = doc["temp"]; // 20.1
    float hum = doc["hum"]; // 55.3
    float press = doc["press"]; // 1005.58
    const char* avg_wind_direction = doc["avg_wind_direction"]; // "north"
    int avg_wind_speed = doc["avg_wind_speed"]; // 0
    const char* gust_wind_direction = doc["gust_wind_direction"]; // "north"
    int gust_wind_speed = doc["gust_wind_speed"]; // 0
    int uv = doc["uv"]; // 0
    int rain_last_hour = doc["rain_last_hour"]; // 0
    int rain_today = doc["rain_today"]; // 0
    int wifi = doc["wifi"]; // -52
    float battery = doc["battery"];
    int batPercentage = (int)(((battery - 3.2)/(4.22-3.2))*100);

    FRAME_DATA fd = getFrameData();

    screen.updateFullScreen((int)fd.temperature, (int)fd.humidity, fd.batteryLevel, fd.wifiStrength, batPercentage, wifi);
}

void processMqttMessage(const char* message) {
    debugger.log(String(message).c_str());    
    processData(message);
}


// Botón
int btnLastState = HIGH;
int btnCurrentState;
unsigned long btnPressedTime  = 0;
unsigned long btnReleasedTime = 0;
boolean btnPressedOnAwake = false;

void updateButtonScreen() {
    processData(jsonEjemplo);
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

void setup() {
    /* BUTTON */
    pinMode(WAKEUP_GPIO, INPUT_PULLUP);
    // Habilita el wake-up por GPIO (LOW significa que despierta cuando el botón es presionado)
    // Calcula el bitmask para el GPIO seleccionado
    // uint64_t gpio_bitmask = 1ULL << WAKEUP_GPIO;   
    // esp_deep_sleep_enable_gpio_wakeup(gpio_bitmask, ESP_GPIO_WAKEUP_GPIO_LOW);
    btnPressedOnAwake = !digitalRead(WAKEUP_GPIO);

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

    debugger.log("KRVKWeather Frame iniciado");
    // pinMode(GPIO_NUM_2, OUTPUT);
    // digitalWrite(GPIO_NUM_2, HIGH);

    // Configuración del cliente MQTT 
     mqttClientHandler.setup(); 
     mqttClientHandler.onMessageReceived = [](const char* message) {
        processMqttMessage(message);
    };

    if (!bme.begin(BME280_ADDRESS)) {
        debugger.log("¡Error al inicializar el BME280! Verifica las conexiones.");
        while (1); // Detiene el programa si no se encuentra el sensor
    }

    screen.init();
}

void startDeepSleep(long timeInSeconds) {
    screen.hibernate();    
    debugger.log("Entrando en modo deep sleep...");
    delay(100);
    WiFi.disconnect(true);
    esp_sleep_enable_timer_wakeup(timeInSeconds * 1000000);
    esp_deep_sleep_start();
}

/* BUTTON */
void handleShortPress() {
    debugger.log("Pulsación corta");
    updateButtonScreen();
}

void handleLongPress() {
    if (btnPressedOnAwake) {
        debugger.log("Despertado mediante botón");
    } else {
        startDeepSleep(60);
    }
    btnPressedOnAwake = false;    
}

void loop() {

    static unsigned long lastOTACheck = 0;
    unsigned long currentMillis = millis();

    /* OTA */
    if (currentMillis - lastOTACheck >= OTA_UPDATE_MS) {
        lastOTACheck = currentMillis;
        ArduinoOTA.handle(); // Permite las actualizaciones OTA
    }    

    /* BUTTON */
    btnCurrentState = digitalRead(WAKEUP_GPIO);
    if (btnLastState == HIGH && btnCurrentState == LOW) {
        // Button pressed, record time
        btnPressedTime = millis();
    } 
    else if (btnLastState == LOW && btnCurrentState == HIGH) {
        // Button released, calculate press duration
        btnReleasedTime = millis();
        long pressDuration = btnReleasedTime - btnPressedTime;

        if (pressDuration < SHORT_PRESS_TIME) {
            handleShortPress();
        } 
        else if (pressDuration > LONG_PRESS_TIME) {
            handleLongPress();
        } 
        else {
            debugger.log("Pulsación larga estando activo");
        }
    }

    // Update last button state
    btnLastState = btnCurrentState;
}