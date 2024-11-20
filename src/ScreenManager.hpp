#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

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

    // void printTextOnScreen(String text) {
    //     int16_t tbx, tby; uint16_t tbw, tbh;
    //     display.getTextBounds(text.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
    //     // center the bounding box by transposition of the origin:
    //     uint16_t x = ((display.width() - tbw) / 2) - tbx;
    //     uint16_t y = ((display.height() - tbh) / 2) - tby;
    //     display.setFullWindow();
    //     display.firstPage();  
    //     do
    //     {    
    //         display.fillScreen(GxEPD_WHITE);
    //         display.setCursor(x, y-tbh);
    //         display.print(text);  
    //         // drawBatteryAt(27, 0, 0);   
    //         drawWifiStrengthAt(45,0,0); 
    //     }
    //     while (display.nextPage());    
    // }

    // // Método para actualizar la pantalla
    // void updateScreen() {
    //     display.display(); // Refresca el contenido de la pantalla
    // }

    // // Limpia la pantalla
    // void clearScreen() {
    //     display.fillScreen(GxEPD_WHITE);
    // }

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
        // String humValue = String(roundedHum) + "%";        
        int16_t tbx, tby; uint16_t tbw, tbh;
        display.setFont(&FreeSans24pt7b);
        display.getTextBounds(String(roundedHum).c_str(), x, y, &tbx, &tby, &tbw, &tbh);  
        display.setCursor(x,y);    
        display.print(String(roundedHum).c_str());

        int16_t tbxO, tbyO; uint16_t tbwO, tbhO;       
        display.setFont(&FreeSans18pt7b); 
        display.getTextBounds(String("%").c_str(), x, y, &tbxO, &tbyO, &tbwO, &tbhO);      
        display.setCursor(x+tbw+3, y+tbhO-tbh);
        display.print("%");
    }    

    void updateFullScreen(float tempInt, float humInt, int batInt, int wifiInt, int batExt, int wifiExt) {
        display.setFullWindow();
        display.firstPage();  
        do
        {   display.setTextColor(GxEPD_WHITE);
            drawSeparator();
            drawInternalTemp(0,351, tempInt);
            drawInternalHum(100, 351, humInt);
            drawBattery(225, 365, batInt, true);
            drawWifiStrengthAt(261, 372, wifiInt, GxEPD_WHITE);
            drawBattery(225, 315, batExt, false);
            drawWifiStrengthAt(261, 322, wifiExt, GxEPD_BLACK);            
        }
        while (display.nextPage()); 
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
        // // center the bounding box by transposition of the origin:
        uint16_t x2 = ((_x - tbw) / 2) - tbx;
        uint16_t y2 = ((_y - tbh) / 2) - tby;
        display.setCursor(_x+grosor+2, _y+tbh+3);
        display.print(String(value).c_str());
        display.setTextSize(1);        
    }

    void drawBatteryAt(int percentage, int x, int y) {
        display.setCursor(x,y);
        int initialOffset = x+2;
        if(percentage < 100)
            initialOffset = x+4;
        display.drawRect(x,y,14,9, GxEPD_BLACK);
        display.drawRect(x+14, y+2, 2,5, GxEPD_BLACK);
        display.setCursor(initialOffset, y+7);
        display.setFont(&TomThumb);
        display.print(String(percentage).c_str());
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
            rssiValue = rssiValue + 10;
        }
            
        // if(value <= -80)
        //     checkboardWifi(x+(width+separator)*0,y,width,2+step*0+2, color_cb);
        // if(value <= -70)
        //     checkboardWifi(x+(width+separator)*1,y,width,2+step*1+2, color_cb);
        // if(value <= -60)
        //     checkboardWifi(x+(width+separator)*2,y,width,2+step*2+2, color_cb);
        // if(value <= -50)
        //     checkboardWifi(x+(width+separator)*3,y,width,2+step*3+2, color_cb);
        // if(value <= -40)
        //     checkboardWifi(x+(width+separator)*4,y,width,2+step*4+2, color_cb);
    }    

    void drawSeparator() {
        display.setCursor(0,0);
        // display.drawFastHLine(0,350,30, GxEPD_BLACK);
        // display.drawFastVLine(30,350,-25, GxEPD_BLACK);
        // display.drawLine(30, 325, 40, 300, GxEPD_BLACK);
        // display.drawLine(40,300,60,325, GxEPD_BLACK);
        // display.drawFastVLine(60,325,25, GxEPD_BLACK);
        // display.drawFastHLine(60,350,display.epd2.WIDTH-100, GxEPD_BLACK);
        display.fillTriangle(30,325,44,310,59,325,GxEPD_BLACK);
        display.fillRect(30,325,30,30,GxEPD_BLACK);
        display.fillRect(0, display.height()-50, display.width(),50, GxEPD_BLACK);
    }
};

#endif // SCREEN_MANAGER_H
