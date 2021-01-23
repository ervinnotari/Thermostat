#include "ThermostatDisplay.h"

ThermostatDisplay::ThermostatDisplay(const uint8_t pin_scl, const uint8_t pin_sda) {
//  _display = new SSD1306Wire(0x3c, pin_sda, pin_scl);
}

void ThermostatDisplay::begin() {
/*  _display->init();
  _display->flipScreenVertically();*/
}

void ThermostatDisplay::loop() {
  //Apaga o display
/*  _display->clear();
  _display->setTextAlignment(TEXT_ALIGN_CENTER);
  //Seleciona a fonte
  _display->setFont(ArialMT_Plain_16);
  _display->drawString(63, 10, "NodeMCU");
  _display->drawString(63, 26, "ESP8266");
  _display->drawString(63, 45, "Display Oled");
  _display->display();*/
}
