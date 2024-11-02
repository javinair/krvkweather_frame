#include "Debugger.h"

Debugger::Debugger(IPAddress remoteIP, uint16_t remotePort) 
    : remoteIP(remoteIP), remotePort(remotePort) {}

void Debugger::init() {
}

void Debugger::log(const char *message) {
    udp.beginPacket(remoteIP, remotePort);
    udp.write((const uint8_t*)message, strlen(message));
    udp.endPacket();
}
