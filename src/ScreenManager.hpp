#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "FreeMonoBold42ptClock.h"
#include "FreeSansBold42ptClock.h"
#include "FreeSans10ptClock.h"
#include <Fonts/FreeSans12pt7b.h>
#include "FreeSansBold16ptClock.h"
#include "Debugger.h"
#include <GxEPD2_BW.h>
#include <Fonts/TomThumb.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/Org_01.h>
#include "bitmaps.hpp"

class ScreenManager {
private:
    GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT>& display;
    Debugger& debugger;   

public:
    // Constructor
    ScreenManager(GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT>& disp, Debugger& debugger) 
        : display(disp), debugger(debugger) {}

    void hibernate() {
        display.hibernate();
    }

    void init() {
        display.init(115200,true,50,false);
        display.setTextColor(GxEPD_BLACK);    
        display.setRotation(1);
    }

    void printExternalData(int x, int y, int value, String symbol) {
        int roundedValue = round(value);
        if(roundedValue > 99)
            roundedValue = 99;
        int16_t tbx, tby; uint16_t tbw, tbh;
        int16_t fixed_x = x;
        display.setFont(&FreeMonoBold42pt7b);
        display.getTextBounds(String(roundedValue).c_str(), x, y, &tbx, &tby, &tbw, &tbh);        
        if(roundedValue < 10) // Es solo un dígito. Se le añade un 0 delante para calcular el tamaño del texto
            fixed_x = x+(tbw*1.5);

        display.setCursor(fixed_x,y+tbh);    
        display.print(String(roundedValue).c_str());

        int16_t tbxSimbolo, tbySimbolo; uint16_t tbwSimbolo, tbhSimbolo;           
        display.setFont(&FreeSans18pt7b); 
        display.getTextBounds(symbol, x, y, &tbxSimbolo, &tbySimbolo, &tbwSimbolo, &tbhSimbolo);      
        display.setCursor(fixed_x+tbw+15, y+tbhSimbolo);
        display.print(symbol);        
    }

    void printExternalTemperature(int x, int y, float temp) {
        printExternalData(x, y, temp, "o");
    }       

    void printExternalHumidity(int x, int y, float hum) {
        printExternalData(x, y, hum, "%");
    }       

    void printInternalTemperature(int x, int y, float temp) {
        int roundedTemp = round(temp);        
        int16_t tbx, tby; uint16_t tbw, tbh;
        display.setFont(&FreeSans24pt7b);
        display.getTextBounds(String(roundedTemp).c_str(), x, y, &tbx, &tby, &tbw, &tbh);  
        display.setCursor(x,y);    
        display.print(String(roundedTemp).c_str());

        int16_t tbxO, tbyO; uint16_t tbwO, tbhO;       
        display.setFont(&FreeSans9pt7b); 
        display.getTextBounds(String("o").c_str(), x, y, &tbxO, &tbyO, &tbwO, &tbhO);      
        display.setCursor(x+tbw+5, y+tbhO-tbh);
        display.print("o");
    }

    void printInternalHumidity(int x, int y, float hum) {
        int roundedHum = round(hum);
        int16_t tbx, tby; uint16_t tbw, tbh;
        display.setFont(&FreeSans24pt7b);
        display.getTextBounds(String(roundedHum).c_str(), x, y, &tbx, &tby, &tbw, &tbh);  
        display.setCursor(x,y);    
        display.print(String(roundedHum).c_str());

        int16_t tbxO, tbyO; uint16_t tbwO, tbhO;       
        display.setTextSize(1);        
        display.setFont(&FreeSans18pt7b); 
        display.getTextBounds(String("%").c_str(), x, y, &tbxO, &tbyO, &tbwO, &tbhO);      
        display.setCursor(x+tbw+3, y+tbhO-tbh);
        display.print("%");
    }   

    void updateFullScreen(float tempExt, float humExt, float tempInt, float humInt, int batInt, int wifiInt, int batExt, int wifiExt, String avg_wind_direction, int avg_wind_speed, int gust_wind_speed, const char* timestamp,
                            float battery_voltage, float battery_shunt_voltage, float battery_current, float battery_power, int maxTempExt, int minTempExt, int maxHumExt, int minHumExt, float rain_last_hour, float rain_today, int sun_current) {
        display.setFullWindow();
        display.firstPage();  
        do
        {   display.setTextColor(GxEPD_WHITE);
            display.setTextSize(1);
            drawInnerDataBackground();
            drawInternalTemp(20,351, tempInt);
            drawInternalHum(130, 351, humInt);
            drawBattery(225, 365, batInt, true);
            drawWifiStrengthAt(261, 372, wifiInt, GxEPD_WHITE);

            drawBattery(225, 315, batExt, false);
            drawWifiStrengthAt(261, 322, wifiExt, GxEPD_BLACK);       
            drawLastUpdate(timestamp);     
            drawExternalTemp(tempExt, maxTempExt, minTempExt);
            drawExternalHum(humExt, maxHumExt, minHumExt);
            drawWind(avg_wind_direction, avg_wind_speed, gust_wind_speed);
            drawRain(rain_last_hour, rain_today);
            drawSunPower(sun_current);
            // drawDebugData(0, 200, "V: ", battery_voltage);
            // drawDebugData(0, 230, "VShunt: ", battery_shunt_voltage);
            // drawDebugData(0, 260, "Int: ", battery_current);
            // drawDebugData(0, 290, "Pot: ", battery_power);            
        }
        while (display.nextPage()); 
    }

    void drawSunPower(int sun_current) {
        int x = 15;
        int y = 240;
        int height = 40;
        int width = 40;
        int x_text = x + width + 5;
        int y_text = y + 26;
        String texto = String(sun_current); 
        display.drawBitmap(x, y, epd_bitmap_sun_power, width, height, GxEPD_BLACK);
        display.setCursor(x_text, y_text);
        display.setFont(&FreeSans12pt7b);
        display.print(texto);
        printMagnitud(texto, x_text, y_text, "mA");       
    }

    void drawRain(float rain_last_hour, float rain_today) {
        int x = 20;
        int y = 175;
        int height = 50;
        int width = 30;
        const uint8_t* bitmap;

        if (rain_today > 10) {
            bitmap = epd_bitmap_rain_3;
        } else if (rain_today > 5) {
            bitmap = epd_bitmap_rain_2;
        } else if (rain_today > 0) {
            bitmap = epd_bitmap_rain_1;
        } else {
            bitmap = epd_bitmap_rain_0;
        }
    
        display.drawBitmap(x, y, bitmap, width, height, GxEPD_BLACK);
        int x_text = x + width + 5;
        int y_text = y + 22;
        display.setCursor(x_text, y_text);
        display.setFont(&FreeSans12pt7b);
        String texto = "";
        if (rain_today == (int)rain_today) {
            texto = String((int)rain_today);                      
        } else {
            texto = String(rain_today, 1);                  
        }      
        display.print(texto);  
        printMagnitud(texto, x_text, y_text, "L/24h");       


        display.setCursor(x_text, y_text);
        display.setFont(&FreeSans12pt7b);
        x_text = x + width + 5;
        y_text = y + 44;
        display.setCursor(x_text, y_text);
        if (rain_last_hour == (int)rain_last_hour) {
            texto = String((int)rain_last_hour);
        } else {
            texto = String(rain_last_hour, 1);
        }
        display.print(texto);  
        printMagnitud(texto, x_text, y_text, "L/1h");
    }

    void printMagnitud(String valor, int x_text, int y_text, String magnitud) {
        int16_t tbx, tby; uint16_t tbw, tbh;        
        display.getTextBounds(valor, x_text, y_text, &tbx, &tby, &tbw, &tbh);              
        int separator = 5;
        display.setFont(&FreeSans9pt7b);
        display.setCursor(x_text + tbw + separator, y_text);
        display.print(magnitud);
    }

    void drawWind(String avg_wind_direction, int avg_wind_speed, int gust_wind_speed) {
        display.fillCircle(210, 240, 75, GxEPD_BLACK);
        display.fillCircle(210, 240, 65, GxEPD_WHITE);

        if(avg_wind_direction.equals("north-east") || avg_wind_direction.equals("north") || avg_wind_direction.equals("north-west")) {
            windLowerText(avg_wind_direction, avg_wind_speed, gust_wind_speed, 210, 240);
        } else {
            windUpperText(avg_wind_direction, avg_wind_speed, gust_wind_speed, 210, 240);
        }
    }

    void windUpperText(String avg_wind_direction, int avg_wind_speed, int gust_wind_speed, int center_x, int center_y) {
        display.setFont(&FreeSans10pt7b);
        int16_t tbx, tby; uint16_t tbw, tbh;
        display.getTextBounds(String(gust_wind_speed).c_str(), center_x, center_y, &tbx, &tby, &tbw, &tbh);
        display.setCursor(center_x-tbw*0.5, center_y-10);
        display.print(gust_wind_speed);
        display.setFont(&FreeSansBold16pt7b);
        int16_t tbx2, tby2; uint16_t tbw2, tbh2;
        display.getTextBounds(String(avg_wind_speed).c_str(), center_x, center_y, &tbx2, &tby2, &tbw2, &tbh2);
        display.setCursor(center_x-tbw2*0.5, center_y-30);
        display.print(avg_wind_speed);    

        if(avg_wind_direction.equals("south"))
            display.drawBitmap(145, 175, epd_bitmap_south, 130, 130, GxEPD_BLACK);
        else if(avg_wind_direction.equals("south-east"))            
            display.drawBitmap(145, 175, epd_bitmap_south_east, 130, 130, GxEPD_BLACK);        
        else if(avg_wind_direction.equals("east"))            
            display.drawBitmap(145, 175, epd_bitmap_east, 130, 130, GxEPD_BLACK);
        else if(avg_wind_direction.equals("west"))            
            display.drawBitmap(145, 175, epd_bitmap_west, 130, 130, GxEPD_BLACK);
        else if(avg_wind_direction.equals("south-west"))            
            display.drawBitmap(145, 175, epd_bitmap_south_west, 130, 130, GxEPD_BLACK);            
    }

    void windLowerText(String avg_wind_direction, int avg_wind_speed, int gust_wind_speed, int center_x, int center_y) {
        display.setFont(&FreeSans10pt7b);
        int16_t tbx, tby; uint16_t tbw, tbh;
        display.getTextBounds(String(gust_wind_speed).c_str(), center_x, center_y, &tbx, &tby, &tbw, &tbh);
        display.setCursor(center_x-tbw*0.5, center_y+20);
        display.print(gust_wind_speed);
        display.setFont(&FreeSansBold16pt7b);
        int16_t tbx2, tby2; uint16_t tbw2, tbh2;
        display.getTextBounds(String(avg_wind_speed).c_str(), center_x, center_y, &tbx2, &tby2, &tbw2, &tbh2);
        display.setCursor(center_x-tbw2*0.5, center_y+50);
        display.print(avg_wind_speed);       

        if(avg_wind_direction.equals("north-east"))            
            display.drawBitmap(145, 175, epd_bitmap_north_east, 130, 130, GxEPD_BLACK);
        else if(avg_wind_direction.equals("north"))            
            display.drawBitmap(145, 175, epd_bitmap_north, 130, 130, GxEPD_BLACK);
        else if(avg_wind_direction.equals("north-west"))            
            display.drawBitmap(145, 175, epd_bitmap_north_west, 130, 130, GxEPD_BLACK);     
    }    

    void drawExternalTemp(int temp, int max, int min) {
        // display.setPartialWindow(0,0, 150, 100);
        display.drawRoundRect(-15, 10, 85, 70, 15, GxEPD_BLACK);        
        display.fillRect(0, 10, 10, 70, GxEPD_BLACK);
        display.drawBitmap(25,20,epd_bitmap_temp_30_50, 30, 50, GxEPD_BLACK);
        printExternalTemperature(80, 20, temp);        
        drawExternalTempMaxMin(max, min);
    }

    void drawExternalTempMaxMin(int max, int min) {
        display.setTextColor(GxEPD_BLACK);        
        display.setFont(&FreeSans9pt7b);        
        display.setCursor(220, 43);
        display.print("MAX");        
        display.setCursor(220, 65);
        display.print("MIN");                        
        display.setFont(&FreeSans12pt7b);   
        display.setCursor(270, 43);
        display.print(String(max).c_str());
        display.setCursor(270, 65);
        display.print(String(min).c_str());
    }

    void drawInternalTemp(int x, int y, float temp) {
        display.setCursor(x, y);
        display.drawBitmap(x, y, epd_bitmap_temp_30_50, 30, 50, GxEPD_WHITE);
        printInternalTemperature(x+30, y+(50*0.8), temp);
    }

    void drawInternalHum(int x, int y, float hum) {
        display.setCursor(x, y);
        display.drawBitmap(x, y, epd_bitmap_hum_30_50, 30, 50, GxEPD_WHITE);
        printInternalHumidity(x+30, y+(50*0.8), hum);
    }   

    void drawExternalHum(float hum, int max, int min) {
        display.drawRoundRect(-15, 90, 85, 70, 15, GxEPD_BLACK);        
        display.fillRect(0, 90, 10, 70, GxEPD_BLACK);
        display.drawBitmap(25,100, epd_bitmap_hum_30_50, 30, 50, GxEPD_BLACK);
        printExternalHumidity(80, 95, hum);
        drawExternalHumMaxMin(max, min);
    }  
    
    void drawExternalHumMaxMin(int max, int min) {
        int maxValue = max;
        if(maxValue >= 100)
            maxValue = 99;
        display.setTextColor(GxEPD_BLACK);        
        display.setFont(&FreeSans9pt7b);        
        display.setCursor(220, 115);
        display.print("MAX");        
        display.setCursor(220, 138);
        display.print("MIN");                        
        display.setFont(&FreeSans12pt7b);   
        display.setCursor(270, 116);
        display.print(String(maxValue).c_str());
        display.setCursor(270, 138);
        display.print(String(min).c_str());
    }    

    void drawBattery(int x, int y, int value, boolean isInterior) {
        int colorBorde = GxEPD_BLACK;
        int colorRelleno = GxEPD_WHITE;
        int _x = x;
        int _y = y;        
        int ancho = 30;
        int alto = 20;
        int grosor = 2;
        if(isInterior)
        {
            _x = display.width() - (ancho + grosor*2) - 5;
            _y = display.height() - (alto + grosor*2);
            colorBorde = GxEPD_WHITE;
            colorRelleno = GxEPD_BLACK;
        } else {
            _x = display.width() - (ancho + grosor*2) - 5;
            _y = display.height() - (alto + grosor*2) - 50;
        }
        display.setTextColor(colorBorde);
        display.setCursor(_x, _y);
        display.fillRect(_x,_y, ancho, alto, colorBorde);
        display.fillRect(_x+grosor, _y+grosor, ancho-(grosor*2), alto-(grosor*2), colorRelleno);
        display.fillRect(_x+ancho, _y+(alto*0.25), 4,alto*0.5, colorBorde);
        display.setFont(&Org_01);
        display.setTextSize(2);
        int16_t tbx, tby; uint16_t tbw, tbh;
        display.getTextBounds(String(value).c_str(), _x+grosor, _y+grosor, &tbx, &tby, &tbw, &tbh);
        uint16_t x2 = ((_x - tbw) / 2) - tbx;
        uint16_t y2 = ((_y - tbh) / 2) - tby;
        display.setCursor(_x+grosor+2, _y+tbh+3);
        display.print(String(value).c_str());
        display.setTextSize(1);        
    }

    void checkboardWifi(int x, int y, int width, int height, uint16_t color) {        
        for(int j=0;j<width;j++)
        {
            for(int i=0;i<height;i++)
            {
                if((i%2!=0 && j%2==0) || (i%2==0 && j%2!=0))
                    display.drawPixel(x+j, y-i, color);
            }
        }
    }

    void drawLastUpdate(const char* timestamp) {
        String horaMinutos = extractTime(timestamp);
        display.setFont(&FreeSans9pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(100, 345);
        display.print("Actualizado: ");
        display.setFont(&FreeSans10pt7b);
        display.setCursor(200, 345);
        display.print(horaMinutos);
    }

    void drawDebugData(int x, int y, String texto, float valor) {
        display.setFont(&FreeSans9pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(x, y);
        display.print(texto+valor);
    }    

    void drawWifiStrengthAt(int x, int y, int value, uint16_t color) {
        int width = 5;
        int separator = 1;
        int step = 4;
        for(int i=0;i<5;i++)
            display.fillRect(x+(width+separator)*i,y,width,-2-step*i,color);

        uint16_t color_cb = GxEPD_BLACK;
        if(color == GxEPD_BLACK)
            color_cb = GxEPD_WHITE;

        int rssiValue = -80;
        for(int i=0;i<5;i++) {
            if(value <= rssiValue)
                checkboardWifi(x+(width+separator)*i,y,width,2+step*i+2, color_cb);                    
            rssiValue = rssiValue + 2;
        }
    }    

    void drawInnerDataBackground() {
        display.setCursor(0,0);
        display.fillRect(0, display.height()-50, display.width(),50, GxEPD_BLACK);
    }

    // Función para extraer horas y minutos de un timestamp
    String extractTime(const String &timestamp) {
        // Buscar la posición de la 'T' (que separa la fecha de la hora)
        int tIndex = timestamp.indexOf(' ');
        if (tIndex == -1) return ""; // Si no hay 'T', retornar cadena vacía

        // Extraer la parte de la hora
        String timePart = timestamp.substring(tIndex + 1); // "19:20:04.149223+01:00"

        // Buscar los dos puntos para identificar horas y minutos
        int colonIndex = timePart.indexOf(':');
        if (colonIndex == -1) return ""; // Si no hay ':', retornar cadena vacía

        // Extraer horas y minutos
        return timePart.substring(0, colonIndex + 3); // "19:20"
    }
};

#endif // SCREEN_MANAGER_H
