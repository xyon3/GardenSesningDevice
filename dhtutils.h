#ifndef DHTUTILS_H

#define DHTUTILS_H
#define EEPROM_SIZE

#include <Arduino.h>


String getDataROM();

void writeDataROM(String ssid, String pass, String apiKey = "");

#endif
