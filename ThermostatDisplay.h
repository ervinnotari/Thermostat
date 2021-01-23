#ifndef ThermostatDisplay_H
#define ThermostatDisplay_H

#include "Arduino.h"
//#include <Wire.h>
//#include "SSD1306Wire.h"

class ThermostatDisplay {
  public:
    ThermostatDisplay(const uint8_t pin_scl=0, const uint8_t pin_sda=0);
    void begin();
    void loop();
  private:
  //  SSD1306Wire* _display;
};

#endif
