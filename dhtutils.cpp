#include <EEPROM.h>
#include <Arduino.h>

#define EEPROM_SIZE 256

String getDataROM() {
  char buffer[EEPROM_SIZE];

  for (int i = 0; i < 64; i++) {
    buffer[i] = EEPROM.read(i);
    if (buffer[i] == '\0') break; // Stop at null terminator
  }

  return buffer;
}

void writeDataROM(String ssid, String pass, String apiKey = "") {

  String toWrite = "";

  toWrite += ssid ;// wifi ssid
  toWrite += "$padding$";
  toWrite += pass ;// wifi password
  toWrite += "$padding$";
  toWrite += apiKey ;// sensor api key

  const char* cToWrite = toWrite.c_str();

  // Write each character to EEPROM
  for (int i = 0; i < strlen(cToWrite); i++) {
    EEPROM.write(i, toWrite[i]);
  }

  // Write null terminator to mark end of string
  EEPROM.write(strlen(cToWrite), '\0');

  EEPROM.commit();  // Save to flash
  Serial.println("String written to EEPROM!");
}
