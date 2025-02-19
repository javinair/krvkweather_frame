#ifndef STATUS_LED_HPP
#define STATUS_LED_HPP

#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 1  // Number of leds in the strip
#define LED_INTENSITY 50

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
        return strip.Color(255, 255, 255);
    }

    uint32_t getLEDOnChargingColor() {
        return strip.Color(250, 140, 0);
    }

    uint32_t getLEDChargingColor() {
        return strip.Color(255, 0, 0);
    }

    uint32_t getLEDChargedColor() {
        return strip.Color(0, 255, 0);
    }

    uint32_t getLEDOnChargedColor() {
        return strip.Color(26, 149, 49);
    }
};

#endif // STATUS_LED_HPP