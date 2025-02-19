#include "esp_task_wdt.h"
#include "secrets.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "Debugger.h"
#include "MqttClientHandler.h"
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include "ScreenManager.hpp"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <INA226.h>
#include <time.h>
#include <HTTPClient.h>
#include <math.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Ticker.h>
#include "DataStructures.hpp"
#include "StatusLED.hpp"
#include "WakeUpResetReasons.hpp"

/************** Internal battery SoC **************/
#define BATTERY_DATA_SIZE 20
BATTERY_DATA batteryDataList[BATTERY_DATA_SIZE];
float V_max = 4.2;  // 100%
float V_min = 3.2;  // 0%
float k = 2.0;      // Constant
Ticker batteryTicker;   // Ticker to update battery data
int bdIndex = 0;
int bdCount = 0;

/************** Status LED **************/
#define LED_PIN 10
StatusLED statusLED(LED_PIN);

/************** Timers **************/
#define SHORT_PRESS_TIME 500
#define LONG_PRESS_TIME 2000
#define OTA_UPDATE_MS 1000

/************** Button **************/
#define GPIO_BUTTON GPIO_NUM_0
bool resetByButton = false;
int btnLastState = HIGH;
int btnCurrentState;
unsigned long btnPressedTime  = 0;
unsigned long btnReleasedTime = 0;
boolean btnPressedOnAwake = false;

/************** NTP **************/
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600; // GMT Madrid offset (GMT+1)
const int daylightOffset_sec = 3600; // Summer time offset (1 hora)

/************** Sensors **************/
#define BME280_ADDRESS 0x76 
Adafruit_BME280 bme;
INA226 INA0(0x40);

/************** Debug **************/
Debugger debugger(IPAddress(192, 168, 0, 255), 12345);

/************** eInk **************/
#define ENABLE_GxEPD2_GFX 0
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(/*CS*/ SS, /*DC*/ 1, /*RES*/ 2, /*BUSY*/ 3)); // 400x300, SSD1683
ScreenManager screen(display, debugger);

/************** WakeUpResetReasons **************/
WakeUpResetManager wakeUpResetManager(debugger);

// Test
char* jsonEjemplo = "{\"last_record\":{\"battery_level\":4.07,\"device_power\":0.0,\"humidity\":79.4,\"id\":3332,\"pressure\":1016.2,\"rain_last_hour\":4.53,\"rain_today\":8.0,\"solar_power\":340,\"temperature\":12.3,\"timestamp\":\"2025-02-08 19:22:23\",\"uv_index\":0.0,\"wind_direction\":\"north\",\"wind_gust\":2.0,\"wind_speed\":1.0},\"today_extremes\":{\"humidity\":{\"max\":100,\"min\":8.9},\"pressure\":{\"max\":1016.2,\"min\":1012.38},\"temperature\":{\"max\":20.6,\"min\":8.2},\"wind_speed\":{\"max\":14.0}}}";


/****************************** Methods *******************************/
void processData(const char* data);
FRAME_DATA getFrameData();
BATTERY_DATA getInaData();
int calculateNextWakeup(int timeInMinutes);
void getLastDataFromKRVKWeather();
int calculateSOC(float voltage);
void updateBatteryDataList();
bool isBatteryCharging();
bool isBatteryCharged();


bool isBatteryCharging() {
    BATTERY_DATA bd = getInaData();    
    return bd.shuntVoltage <= 0.0;
}

bool isBatteryCharged() {
    BATTERY_DATA bd = getInaData();
    return bd.shuntVoltage <= -7.0 && bd.current >= -80.0 && bd.power <= 320.0;
}

void updateBatteryDataList() {
    BATTERY_DATA bd = getInaData();
    batteryDataList[bdIndex] = bd;
    bdIndex = (bdIndex + 1) % BATTERY_DATA_SIZE;
    if(bdIndex < BATTERY_DATA_SIZE) {
        bdCount++;
    }
    debugger.log(String(bd.busVoltage).c_str());
}

String httpGETRequest(const char* serverName) {
    WiFiClient client;
    HTTPClient http;
      
    http.begin(client, serverName);
    
    // Send HTTP POST request
    int httpResponseCode = http.GET();
    
    String payload = "{}"; 
    
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      payload = http.getString();
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  
    return payload;
  }

void getLastDataFromKRVKWeather() {
    const int maxRetries = 5;
    const int retryInterval = 2000; // 2 segundos
    int attempt = 0;
    bool success = false;

    while (attempt < maxRetries && !success) {
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin("http://192.168.0.88:5050/get_frame_data");

            int httpResponseCode = http.GET();

            if (httpResponseCode == 200) {
                WiFiClient* stream = http.getStreamPtr();
                String payload;
                while (stream->connected() || stream->available()) {
                    if (stream->available()) {
                        char c = stream->read();
                        payload += c;
                    }
                }
                debugger.log("Respuesta recibida:");
                debugger.log(payload.c_str());

                // Procesar la respuesta JSON
                processData(payload.c_str());
                success = true; // La solicitud fue exitosa
            } else {
                String error = "HTTP request failed with error code: " + String(httpResponseCode);
                debugger.log(error.c_str());
            }

            http.end();
        } else {
            debugger.log("WiFi no está conectado");
        }

        if (!success) {
            attempt++;
            if (attempt < maxRetries) {
                debugger.log("Reintentando en 2 segundos...");
                delay(retryInterval);
            }
        }
    }

    if (!success) {
        debugger.log("No se pudo obtener datos de KRVKWeather después de 5 intentos.");
    }
}


FRAME_DATA getFrameData() {
    FRAME_DATA frameData;
    frameData.temperature = bme.readTemperature();
    frameData.humidity = bme.readHumidity();
    frameData.wifiStrength = WiFi.RSSI();

    BATTERY_DATA bd;
    debugger.log(String(bdCount).c_str());
    if(bdCount > 0) {
        float avgVBus = 0.0f, avgVShunt = 0.0f, avgCurrent = 0.0f, avgPower = 0.0f;
        for(int i=0; i<bdCount; i++) {
            avgVBus += batteryDataList[i].busVoltage;
            avgVShunt += batteryDataList[i].shuntVoltage;
            avgCurrent += batteryDataList[i].current;
            avgPower += batteryDataList[i].power;
        }
        avgVBus /= bdCount;
        avgVShunt /= bdCount;
        avgCurrent /= bdCount;
        avgPower /= bdCount;

        bd.busVoltage = avgVBus;
        bd.shuntVoltage = avgVShunt;
        bd.current = avgCurrent;
        bd.power = avgPower;
    }   
    else {
        bd = getInaData();
    }
    
    String inaData = String(bd.busVoltage)+";"+String(bd.shuntVoltage)+";"+String(bd.current)+";"+String(bd.power);
    debugger.log(String(inaData).c_str());

    frameData.bd = bd;
    frameData.batteryLevel = calculateSOC(bd.busVoltage);
    return frameData;
}

BATTERY_DATA getInaData() {
    BATTERY_DATA bd;
    bd.busVoltage = INA0.getBusVoltage();        // Voltaje del bus (V)
    bd.shuntVoltage = INA0.getShuntVoltage_mV(); // Voltaje del shunt (mV)
    bd.current = INA0.getCurrent_mA();           // Corriente (mA)
    bd.power = INA0.getPower_mW();               // Potencia (mW)
    return bd;
}

int calculateSOC(float V) {
    if (V <= V_min) return 0.0;
    if (V >= V_max) return 100.0;

    float k = 4.0;  // Ajuste basado en la curva de descarga
    return (int)(100.0 * (1 - exp(-k * (V - V_min) / (V_max - V_min))));
}

void processData(const char* data) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
        debugger.log("deserializeJson() failed: ");
        debugger.log(error.c_str());
        return;
    }

    float temp = doc["last_record"]["temperature"]; // 20.1
    float hum = doc["last_record"]["humidity"]; // 55.3
    float press = doc["last_record"]["pressure"]; // 1005.58
    const char* avg_wind_direction = doc["last_record"]["wind_direction"]; // "north"
    int avg_wind_speed = doc["last_record"]["wind_speed"]; // 0
    int gust_wind_speed = doc["last_record"]["wind_gust"]; // 0
    int uv = doc["last_record"]["uv_index"]; // 0
    float rain_last_hour = doc["last_record"]["rain_last_hour"]; // 0
    float rain_today = doc["last_record"]["rain_today"]; // 0
    int wifi = doc["last_record"]["wifi"]; // -52
    int batPercentage = calculateSOC(doc["last_record"]["battery_level"]);
    const char* timestamp = doc["last_record"]["timestamp"]; // "north"    
    int maxTempExt = doc["today_extremes"]["temperature"]["max"]; // 20.6
    int minTempExt = doc["today_extremes"]["temperature"]["min"]; // 8.2
    int maxHumExt = doc["today_extremes"]["humidity"]["max"]; // 97.5
    int minHumExt = doc["today_extremes"]["humidity"]["min"]; // 38.9
    int solar_power = doc["last_record"]["solar_power"]; // 0

    batteryTicker.detach();
    FRAME_DATA fd = getFrameData();

    screen.updateFullScreen(temp, hum, (int)fd.temperature, (int)fd.humidity, 
                            fd.batteryLevel, fd.wifiStrength, batPercentage, wifi,
                            String(avg_wind_direction), avg_wind_speed, gust_wind_speed, 
                            timestamp, fd.bd.busVoltage, fd.bd.shuntVoltage, fd.bd.current, fd.bd.power, maxTempExt, minTempExt, maxHumExt, minHumExt, rain_last_hour, rain_today, solar_power);
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
    debugger.log("Disponible para OTA");
}

void startDeepSleep(long timeInSeconds) {
    screen.hibernate();    
    debugger.log("Entrando en modo deep sleep...");
    delay(100);
    WiFi.disconnect(true);

    if(isBatteryCharging()) {
        if(isBatteryCharged())
            statusLED.setLEDColor(statusLED.getLEDChargedColor());
        else
            statusLED.setLEDColor(statusLED.getLEDChargingColor());
    }
    else
        statusLED.setLEDOff();
        
    wakeUpResetManager.enableTimerWakeUp(timeInSeconds);
    
    esp_deep_sleep_start();
}

void setup() {
    /* ESP32-C3 Configuration */
    esp_task_wdt_init(10, true); // Timeout de 10 segundos para evitar que se quede colgado

    /* SENSORS */
    if (!bme.begin(BME280_ADDRESS)) {
        debugger.log("¡Error al inicializar el BME280! Verifica las conexiones.");
        while (1); // Detiene el programa si no se encuentra el sensor
    }
    if (!INA0.begin()) {
         debugger.log("¡Error al inicializar el INA226! Verifica las conexiones.");
        while (1); // Detiene el programa si no se encuentra el sensor
    }    
    INA0.setMaxCurrentShunt(0.2, 0.1, true);
    INA0.setAverage(INA226_16_SAMPLES);   

    /* BUTTON */
    pinMode(GPIO_BUTTON, INPUT_PULLUP);
    btnPressedOnAwake = digitalRead(GPIO_BUTTON);

    /* WIFI */
    uint32_t brown_reg_temp = 
    READ_PERI_REG(RTC_CNTL_BROWN_OUT_REG); //save WatchDog register
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
    WiFi.mode(WIFI_MODE_STA); // turn on WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, brown_reg_temp); //enable brownout detector    

    /* DEBUG */
    debugger.init();

    memset(batteryDataList, 0, sizeof(batteryDataList));   
    bdIndex = 0;
    bdCount = 0;
    batteryTicker.attach(0.3, updateBatteryDataList);    

    /* OTA */
    otaConfiguration();        

    /* EINK */
    screen.init();

    /* DEEP SLEEP */
    esp_deep_sleep_enable_gpio_wakeup(BIT(GPIO_BUTTON), ESP_GPIO_WAKEUP_GPIO_LOW);        

    /* INITIAL LOG */
    wakeUpResetManager.logResetReason();
    wakeUpResetManager.printWakeupReason(resetByButton);
    debugger.log("KRVKWeather Frame iniciado");

    if(isBatteryCharging()) {
        if(isBatteryCharged())
        statusLED.setLEDColor(statusLED.getLEDOnChargedColor());
        else
        statusLED.setLEDColor(statusLED.getLEDOnChargingColor());
    }
    else
        statusLED.setLEDColor(statusLED.getLEDOnColor());

    getLastDataFromKRVKWeather();        
    if(!resetByButton)
        startDeepSleep(calculateNextWakeup(10));
}


/* BUTTON */
void handleShortPress() {
    debugger.log("Pulsación corta");
    getLastDataFromKRVKWeather();
}

void handleLongPress() {
    debugger.log("Pulsación larga");
    startDeepSleep(calculateNextWakeup(10));    
}

int calculateNextWakeup(int timeInMinutes) {

    // Aunque el intervalo se facilite por parámetro, se fuerza a que los minutos acaben en 1 para que dé tiempo
    // a que KRVKWeather procese y actualice los datos en su BD

    // Configuración del servidor NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // Espera a que se sincronice el tiempo
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        debugger.log("Error al obtener la hora");
        return 60;
    }    

    // Configura el tiempo actual utilizando la función time()
    time_t now;
    struct tm currentTime;
    time(&now);
    localtime_r(&now, &currentTime);

    // Calcula el timestamp de la siguiente hora del día en la que los minutos sean múltiplos de "timeInMinutes"
    struct tm nextTime = currentTime;
    nextTime.tm_min = ((currentTime.tm_min / timeInMinutes) + 1) * timeInMinutes;
    nextTime.tm_sec = 0;

    // Ajusta para que comience a partir del minuto 1 en lugar del minuto 0
    if (nextTime.tm_min % timeInMinutes == 0) {
        nextTime.tm_min += 1;
    }

    // Si los minutos son 60, incrementa la hora y ajusta los minutos a 1
    if (nextTime.tm_min >= 60) {
        nextTime.tm_min = 1;
        nextTime.tm_hour++;
    }

    time_t nextTimestamp = mktime(&nextTime);

    // Calcula la diferencia en segundos
    double differenceInSecondsDouble = difftime(nextTimestamp, now);
    int differenceInSeconds = static_cast<int>(differenceInSecondsDouble);

    // Muestra ambos timestamps y ambas horas en formato hh:mm:ss
    char currentTimeStr[20];
    strftime(currentTimeStr, sizeof(currentTimeStr), "%H:%M:%S", &currentTime);
    char nextTimeStr[20];
    strftime(nextTimeStr, sizeof(nextTimeStr), "%H:%M:%S", &nextTime);
    return differenceInSeconds;
}

void loop() {
    unsigned long currentMillis = millis();

    /* OTA */
    static unsigned long lastOTACheck = 0;    
    if (currentMillis - lastOTACheck >= OTA_UPDATE_MS) {
        lastOTACheck = currentMillis;
        ArduinoOTA.handle(); // Permite las actualizaciones OTA
    }    

    /* BUTTON */
    btnCurrentState = digitalRead(GPIO_BUTTON);
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