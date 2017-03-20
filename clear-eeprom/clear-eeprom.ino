#include <EEPROM.h>

void setup() {
  unsigned long value = 0;
  for (unsigned long addr = 0x0; addr < EEPROM.length(); addr += 4) {
    EEPROM.put(addr, value);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}
