#ifndef ThermostatDisplay_H
#define ThermostatDisplay_H

#include "Arduino.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Thermostat.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)

class ThermostatDisplay
{
public:
  ThermostatDisplay(const uint8_t pin_sda = 0, const uint8_t pin_scl = 0);
  void begin();
  void loop();
  void setTemperature(float temp);
  void setHumidity(float humidity);
  void setPoint(int point);
  void setWifi(String wifi);
  void setThermState(ThermostatState st);
  void showApModeScreen();
  void showLoaderScreen();
  void setEnable(bool enable);
  Adafruit_SSD1306 *display;

private:
  uint8_t _pin_sda, _pin_scl;
  float _temperature, _humidity, _point;
  String _wifi;
  ThermostatState _state;
  bool _enable = true;
};

static const unsigned char PROGMEM IMG_FIRE[] = {0x08, 0x00, 0x08, 0x00, 0x14, 0x00, 0x24, 0x00, 0x40, 0x00, 0x46, 0x00, 0x49, 0x00, 0x41, 0x00, 0x22, 0x00, 0x1c, 0x00};

#endif
