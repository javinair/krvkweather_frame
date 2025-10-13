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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "SPIFFS.h"

/************** DeepSleep **************/
#define NORMAL_SLEEP_TIME_IN_MINUTES 1      // 10 min
#define EXTENDED_SLEEP_TIME_IN_MINUTES 5    // 30 min si batería baja
#define MAX_SLEEP_TIME_IN_MINUTES 10        // 2 horas si batería críticamente baja
#define MAX_LOW_BATTERY_ATTEMPTS 3          // Después de 3 intentos, extender aún más

/************** Internal battery SoC **************/
#define BATTERY_FULL_VOLTAGE 4.0    // 100%
#define BATTERY_LOW_VOLTAGE 3.6     // 0%
#define BATTERY_CRITICAL_VOLTAGE 3.5
#define BATTERY_BUFFER_SIZE 20
BATTERY_DATA batteryDataList[BATTERY_BUFFER_SIZE];
BATTERY_DATA bd;
const float k = 3.0;    // Constante para el cálculo del SoC
Ticker batteryTicker;
int bdIndex = 0;
int bdCount = 0;
RTC_DATA_ATTR int lowBatteryAttempts = 0;  // Variable que persiste en deep sleep

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

/************** NTP **************/
const char* tzSpain = "CET-1CEST,M3.5.0,M10.5.0/3";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600; // GMT Madrid offset (GMT+1)
const int daylightOffset_sec = 3600; // Summer time offset (1 hora)
int secondsToWakeUp = 0;

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

/****************************** Methods *******************************/
void processData(const char* data, FRAME_DATA fd);
FRAME_DATA getFrameData(bool initialRead = false);
BATTERY_DATA getInaData();
int calculateNextWakeup(int timeInMinutes = NORMAL_SLEEP_TIME_IN_MINUTES);
void getLastDataFromKRVKWeather(FRAME_DATA fd);
int calculateSOC(float voltage);
int calcularSOCKRVKWeather(float voltage);
void updateBatteryDataList();
bool isLowBattery();
bool isBatteryCharging();
bool isBatteryCharged();
void syncTimeAndCalculateWakeup(void* parameter);
void processExampleData();
void calculateBatteryData();

bool isLowBattery() {
    return bd.busVoltage <= BATTERY_LOW_VOLTAGE;
}

bool isBatteryCharging() {
    return bd.shuntVoltage <= 0.0;
}

bool isBatteryCharged() {
    return bd.shuntVoltage >= -7.0 && bd.current >= -50.0 && bd.power <= 220.0;
}

void updateBatteryDataList() {
    BATTERY_DATA bd = getInaData();
    batteryDataList[bdIndex] = bd;
    bdIndex = (bdIndex + 1) % BATTERY_BUFFER_SIZE;
    if(bdCount < BATTERY_BUFFER_SIZE) {
        bdCount++;
    }
}

void getLastDataFromKRVKWeather(FRAME_DATA fd) {

    char url[100];
    snprintf(url, sizeof(url), "http://%s:%d/get_frame_data", KRVKWEAHTER_IP, KRVKWEAHTER_PORT);
    const int maxRetries = 5;
    const int retryInterval = 2000;
    int attempt = 0;
    bool success = false;

    while (attempt < maxRetries && !success) {
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;

            http.begin(url);

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
                debugger.log("MQTT recibido");

                // Procesar la respuesta JSON
                processData(payload.c_str(), fd);
                success = true;
            } else {
                debugger.log(String("Error de conexión con KRVKWeather").c_str());
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
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "No se pudo obtener datos de KRVKWeather después de %d intentos.", maxRetries);
        debugger.log(buffer);
    }
}

void calculateBatteryData() {
    if(bdCount > 0) {
        float avgVBus = 0.0f, avgVShunt = 0.0f, avgCurrent = 0.0f, avgPower = 0.0f;
        float maxVBus = -1000.0f, minVBus = 1000.0f;
        float maxCurrent = -1000.0f, minCurrent = 1000.0f;
        for(int i=0; i<bdCount; i++) {
            avgVBus += batteryDataList[i].busVoltage;
            avgVShunt += batteryDataList[i].shuntVoltage;
            avgCurrent += batteryDataList[i].current;
            avgPower += batteryDataList[i].power;
            // if(batteryDataList[i].busVoltage > maxVBus) maxVBus = batteryDataList[i].busVoltage;
            // if(batteryDataList[i].busVoltage < minVBus) minVBus = batteryDataList[i].busVoltage;
            // if(batteryDataList[i].current > maxCurrent) maxCurrent = batteryDataList[i].current;
            // if(batteryDataList[i].current < minCurrent) minCurrent = batteryDataList[i].current;
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
}

FRAME_DATA getFrameData(bool initialRead) {
    FRAME_DATA frameData;
    frameData.temperature = bme.readTemperature();
    frameData.humidity = bme.readHumidity();
    if(initialRead) 
        frameData.wifiStrength = 0;
    else
        frameData.wifiStrength = WiFi.RSSI();
    
    String inaData = String(bd.busVoltage)+";"+String(bd.shuntVoltage)+";"+String(bd.current)+";"+String(bd.power)+";"+String(frameData.wifiStrength);
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
    if (V <= BATTERY_CRITICAL_VOLTAGE) return 0.0;
    if (V >= BATTERY_FULL_VOLTAGE) return 100.0;
    return (int)(100.0 * (1 - exp(-k * (V - BATTERY_CRITICAL_VOLTAGE) / (BATTERY_FULL_VOLTAGE - BATTERY_CRITICAL_VOLTAGE))));
}

int calcularSOCKRVKWeather(float voltage) {
    float porcentaje = ((voltage - 3.2) / (4.21 - 3.2)) * 100.0;
    
    // Limitar el resultado entre 0 y 100
    if (porcentaje < 0) porcentaje = 0;
    if (porcentaje > 100) porcentaje = 100;
    
    return static_cast<int>(porcentaje + 0.5); // redondear al entero más cercano
}


void processData(const char* data, FRAME_DATA fd) {
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
    int batPercentage = calcularSOCKRVKWeather(doc["last_record"]["battery_level"]);
    const char* timestamp = doc["last_record"]["timestamp"]; // "north"    
    float solar_voltage = doc["last_record"]["solar_voltage"]; // 0    
    int maxTempExt = doc["today_extremes"]["temperature"]["max"]; // 20.6
    int minTempExt = doc["today_extremes"]["temperature"]["min"]; // 8.2
    int maxHumExt = doc["today_extremes"]["humidity"]["max"]; // 97.5
    int minHumExt = doc["today_extremes"]["humidity"]["min"]; // 38.9
    int draughtDays = doc["extra_data"]["last_rain_days"]; // 0    

    screen.updateFullScreen(temp, hum, (int)fd.temperature, (int)fd.humidity, 
                            fd.batteryLevel, fd.wifiStrength, batPercentage, wifi,
                            String(avg_wind_direction), avg_wind_speed, gust_wind_speed, 
                            timestamp, fd.bd.busVoltage, fd.bd.shuntVoltage, fd.bd.current, fd.bd.power, maxTempExt, minTempExt, maxHumExt, minHumExt, rain_last_hour, rain_today, draughtDays, solar_voltage);
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
        // char buffer[50];
        // snprintf(buffer, sizeof(buffer), "Progreso OTA: %u%%", (progress * 100) / total);
        // debugger.log(buffer);
    });
    ArduinoOTA.onError([](ota_error_t error) {
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "Error OTA: %u", error);
        debugger.log(buffer);
    });
    ArduinoOTA.begin();
    // debugger.log("Disponible para OTA");
}

void startDeepSleep() {
    debugger.log("Entrando en modo deep sleep...");   
    delay(200);    
    screen.hibernate();        
    if(!isLowBattery()) { 
        if(isBatteryCharging()) {
            if(isBatteryCharged())
                statusLED.setLEDColor(statusLED.getLEDChargedColor());
            else
                statusLED.setLEDColor(statusLED.getLEDChargingColor());
        } 
        else {
            statusLED.setLEDOff();
        }
    }
    else {
        if(isBatteryCharging()) {
                statusLED.setLEDColor(statusLED.getLEDChargingColor());
        } 
        else {
            statusLED.setLEDColor(statusLED.getLEDLowBatteryColor());
        }
    }
        
    wakeUpResetManager.enableTimerWakeUp(secondsToWakeUp);
    esp_deep_sleep_start();
}

void setup() {
    setCpuFrequencyMhz(80); // Cambia la frecuencia de la CPU a 80 MHz
    bool lowBattery = false;
    /* ESP32-C3 Configuration */
    // esp_task_wdt_init(20, true); // Timeout de 10 segundos para evitar que se quede colgado
    // esp_task_wdt_add(NULL);      // Añade la tarea principal (loop) al WDT


    /* BUTTON */
    pinMode(GPIO_BUTTON, INPUT_PULLUP);
    /* DEEP SLEEP */
    esp_deep_sleep_enable_gpio_wakeup(BIT(GPIO_BUTTON), ESP_GPIO_WAKEUP_GPIO_LOW);            

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

    // uint32_t brown_reg_temp = 
    // READ_PERI_REG(RTC_CNTL_BROWN_OUT_REG); //save WatchDog register
    // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector    

    memset(batteryDataList, 0, sizeof(batteryDataList));   
    bdIndex = 0;
    bdCount = 0;
    batteryTicker.attach(0.2, updateBatteryDataList);    
    delay(1000);    
    batteryTicker.detach();
    calculateBatteryData();
    statusLED.setLEDColor(statusLED.getLEDOnColor());        
    if (bd.shuntVoltage <= 0.0) {   // Cargando
        if(lowBatteryAttempts > 0)
            lowBatteryAttempts = 0;
    } else if (bd.busVoltage <= BATTERY_LOW_VOLTAGE) {
        lowBatteryAttempts++;
    } else {
        if(lowBatteryAttempts > 0)
            lowBatteryAttempts = 0;
    }


    /* WIFI */
    WiFi.setTxPower(WIFI_POWER_2dBm);    
    WiFi.persistent(true);
    WiFi.setAutoReconnect(true);
    WiFi.mode(WIFI_MODE_STA);

    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
        }
    }

    statusLED.setLEDColor(statusLED.getLEDOnChargedColor());   
    delay(100); 
    statusLED.setLEDOff();   

    /* DEBUG */
    debugger.init();

    /* FileSystem */
    if(!SPIFFS.begin(true)) {
        debugger.log("Error al montar SPIFFS");
        return;
    }
        
    /* OTA */
    otaConfiguration();        

    /* EINK */
    screen.init();

    /* INITIAL LOG */
    if(!isLowBattery()) {
        debugger.log(String("KRVKWeather Frame iniciado [" + String(lowBatteryAttempts) + "]").c_str());
    } else {
        debugger.log(String("KRVKWeather Frame iniciado [" + String(lowBatteryAttempts) + "] LOW_BATTERY!!").c_str());
    }
    delay(300);
    
    wakeUpResetManager.logResetReason();
    wakeUpResetManager.printWakeupReason(resetByButton);

    xTaskCreate(syncTimeAndCalculateWakeup, "SyncTimeTask", 4096, NULL, 1, NULL);        

    delay(200);
    FRAME_DATA fd = getFrameData();    
    getLastDataFromKRVKWeather(fd);   
    if(!resetByButton)
        startDeepSleep();
}


void processExampleData() {
    File file = SPIFFS.open("/example.json", "r");
    if (!file) {
        debugger.log("⚠️ Error abriendo el archivo");
        return;
    }

    String jsonStr = file.readString();
    file.close();
    FRAME_DATA fd = getFrameData();
    processData(jsonStr.c_str(), fd);
}

/* BUTTON */
void handleShortPress() {
    debugger.log("Pulsación corta");
    processExampleData();
    // getLastDataFromKRVKWeather();
}

void handleLongPress() {
    debugger.log("Pulsación larga");
    startDeepSleep();    
}

void syncTimeAndCalculateWakeup(void* parameter) {
    secondsToWakeUp = calculateNextWakeup();
    vTaskDelete(NULL); // Eliminar la tarea una vez completada
}

int calculateNextWakeup(int timeInMinutes) {

    // Aunque el intervalo se facilite por parámetro, se fuerza a que los minutos acaben en 1 para que dé tiempo
    // a que KRVKWeather procese y actualice los datos en su BD

    // Configuración del servidor NTP
    configTzTime(tzSpain, ntpServer);

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

    int _timeInMinutes = timeInMinutes;
    if(isLowBattery()) {
        _timeInMinutes = EXTENDED_SLEEP_TIME_IN_MINUTES;
        if (lowBatteryAttempts >= MAX_LOW_BATTERY_ATTEMPTS) {
            _timeInMinutes = MAX_SLEEP_TIME_IN_MINUTES;
        }          
    }

    // Calcula el timestamp de la siguiente hora del día en la que los minutos sean múltiplos de "timeInMinutes"
    struct tm nextTime = currentTime;
    nextTime.tm_min = ((currentTime.tm_min / _timeInMinutes) + 1) * _timeInMinutes;
    nextTime.tm_sec = 0;

    // // Ajusta para que comience a partir del minuto 1 en lugar del minuto 0
    // if (nextTime.tm_min % _timeInMinutes == 0) {
    //     nextTime.tm_min += 1;
    // }

    // // Si los minutos son 60, incrementa la hora y ajusta los minutos a 1
    // if (nextTime.tm_min >= 60) {
    //     nextTime.tm_min = 1;
    //     nextTime.tm_hour++;
    // }
    // Ajusta para que comience a partir del minuto 1 en lugar del minuto 0
    if (nextTime.tm_min % _timeInMinutes == 0) {
        nextTime.tm_min += 0;
    }

    // Si los minutos son 60, incrementa la hora y ajusta los minutos a 1
    if (nextTime.tm_min >= 60) {
        nextTime.tm_min = 0;
        nextTime.tm_hour++;
    }

    time_t nextTimestamp = mktime(&nextTime);

    // Calcula la diferencia en segundos
    double differenceInSecondsDouble = difftime(nextTimestamp, now);
    int differenceInSeconds = static_cast<int>(differenceInSecondsDouble);

    // Muestra ambos timestamps y ambas horas en formato hh:mm:ss
    char currentTimeStr[20];
    strftime(currentTimeStr, sizeof(currentTimeStr), "%H:%M:%S", &currentTime);
    // debugger.log(currentTimeStr);
    char nextTimeStr[20];
    strftime(nextTimeStr, sizeof(nextTimeStr), "%H:%M:%S", &nextTime);
    debugger.log((String("Próximo inicio: ") + String(nextTimeStr)).c_str());    
    return differenceInSeconds;
}

void loop() {
    unsigned long currentMillis = millis();

    /* OTA */
    static unsigned long lastOTACheck = 0;    
    if (currentMillis - lastOTACheck >= OTA_UPDATE_MS) {
        lastOTACheck = currentMillis;
        ArduinoOTA.handle();
    }    

    /* BUTTON */
    btnCurrentState = digitalRead(GPIO_BUTTON);
    if (btnLastState == HIGH && btnCurrentState == LOW) {
        btnPressedTime = millis();
    } 
    else if (btnLastState == LOW && btnCurrentState == HIGH) {
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

    btnLastState = btnCurrentState;
}