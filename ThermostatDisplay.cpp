#include "ThermostatDisplay.h"

ThermostatDisplay::ThermostatDisplay(const uint8_t pin_sda, const uint8_t pin_scl) {
  _pin_sda = pin_sda;
  _pin_scl = pin_scl;
  display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
}

void ThermostatDisplay::setTemperature(float temp) { _temperature = temp; }
void ThermostatDisplay::setHumidity(float humidity) { _humidity = humidity; }
void ThermostatDisplay::setPoint(int point) { _point = point; }
void ThermostatDisplay::setWifi(String wifi) { _wifi = wifi; }
void ThermostatDisplay::setThermState(char st) { _state = st; }

void ThermostatDisplay::begin() {
  Wire.begin(_pin_sda, _pin_scl);
  if(!display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display->clearDisplay();
}

void ThermostatDisplay::loop() {
  display->clearDisplay();
  display->setCursor(0,0);
  display->setTextColor(SSD1306_WHITE);
  
  display->setTextSize(1);
  display->print("WiFi:");
  display->println(_wifi);
  
  display->setTextSize(3);
  display->setCursor(0,12);
  display->print(String(_point, 0));
  display->setTextSize(2);
  display->print((char)247);
  display->println("C");

  int x = 70;
  display->setTextSize(1);
  display->setCursor(x,13);
  display->print(String(_temperature, 1));
  display->setTextSize(1);
  display->print((char)247);
  display->println("C");
  
  display->setTextSize(1);
  display->setCursor(x,24);
  display->print(String(_humidity, 0));
  display->setTextSize(1);
  display->print("% ");
  
  display->display();
}
