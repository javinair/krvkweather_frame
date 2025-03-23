#ifndef STATUS_LED_HPP
#define STATUS_LED_HPP

#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 1  // Number of leds in the strip
#define LED_INTENSITY 50
#define LED_RED strip.Color(255, 0, 0)
#define LED_WHITE strip.Color(255, 255, 255)
#define LED_GREEN strip.Color(0, 255, 0)
#define LED_BLUE strip.Color(0, 0, 255)
#define LED_ORANGE strip.Color(250, 140, 0)

class StatusLED {
private:
    Adafruit_NeoPixel strip;
    int ledPin;
    int numLeds;
    int ledIntensity;

public:
    StatusLED(int pin)
        : strip(NUM_LEDS, pin, NEO_GRB + NEO_KHZ800), ledPin(pin), numLeds(NUM_LEDS), ledIntensity(LED_INTENSITY) {
        strip.begin();
        strip.setBrightness(ledIntensity);
    }

    void setLEDColor(uint32_t color) {
        strip.setPixelColor(0, color);
        strip.show();
    }

    void setLEDOff() {
        strip.setPixelColor(0, strip.Color(0, 0, 0));
        strip.show();
    }

    void clearLED() {
        strip.clear();
        strip.show();
    }

    uint32_t getLEDOnColor() {
        return LED_WHITE;
    }

    uint32_t getLEDOnChargingColor() {
        return LED_ORANGE;
    }

    uint32_t getLEDChargingColor() {
        return LED_ORANGE;
    }

    uint32_t getLEDChargedColor() {
        return LED_GREEN;
    }

    uint32_t getLEDOnChargedColor() {
        return LED_BLUE;
    }

    uint32_t getLEDLowBatteryColor() {
        return LED_RED;
    }
};

#endif // STATUS_LED_HPP