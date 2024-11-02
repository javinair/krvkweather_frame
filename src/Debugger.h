#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <WiFiUdp.h>
#include <Arduino.h>

class Debugger {
public:
    Debugger(IPAddress remoteIP, uint16_t remotePort);
    void init();
    void log(const char *message);

private:
    WiFiUDP udp;
    IPAddress remoteIP;
    uint16_t remotePort;
};

#endif // DEBUGGER_H
