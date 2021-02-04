#include "ThermostatDisplay.h"

ThermostatDisplay::ThermostatDisplay(const uint8_t pin_sda, const uint8_t pin_scl)
{
  _pin_sda = pin_sda;
  _pin_scl = pin_scl;
  display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
}

void ThermostatDisplay::setTemperature(float temp) { _temperature = temp; }
void ThermostatDisplay::setHumidity(float humidity) { _humidity = humidity; }
void ThermostatDisplay::setPoint(int point) { _point = point; }
void ThermostatDisplay::setWifi(String wifi) { _wifi = wifi; }
void ThermostatDisplay::setThermState(ThermostatState st) { _state = st; }

void ThermostatDisplay::setEnable(bool enable)
{
  _enable = enable;
  if (!_enable)
  {
    display->clearDisplay();
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);
    display->display();
  }
}

void ThermostatDisplay::begin()
{
  Wire.begin(_pin_sda, _pin_scl);
  if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display->clearDisplay();
}

void ThermostatDisplay::showLoaderScreen()
{
  display->clearDisplay();

  display->setCursor(0, 0);
  display->setTextColor(SSD1306_WHITE);
  display->setTextSize(1.25);
  display->println("Conectado a");
  display->println(_wifi);

  display->display();
}

void ThermostatDisplay::showApModeScreen()
{
  if (!_enable)
    return;
  display->clearDisplay();

  display->setCursor(0, 0);
  display->setTextColor(SSD1306_WHITE);
  display->setTextSize(0.75);
  display->println("Conectese a Wi-Fi:");
  display->println(_wifi);
  display->println("para configurar este dispositivo!");

  display->display();
}

void ThermostatDisplay::loop()
{
  if (!_enable)
    return;
  display->clearDisplay();
  display->setCursor(0, 0);
  display->setTextColor(SSD1306_WHITE);

  display->setTextSize(0.5);
  display->print("WiFi:");
  display->println(_wifi);

  display->setCursor(0, 12);
  display->setTextSize(3);
  String ty = "";
  switch (_state)
  {
  case COOL:
    ty = " CL ";
  case HEAT:
    display->print(String(_point, 0));
    display->setTextSize(1.25);
    display->print((char)247);
    display->println("C");
    display->setCursor(38, 24);
    display->setTextSize(0.5);
    display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display->println(ty.isEmpty() ? " HT " : ty);
    break;
  case FAN:
    display->print("FAN");
    break;
  default:
    break;
  }

  display->setTextColor(SSD1306_WHITE);
  int x = 70;
  display->setTextSize(1);
  display->setCursor(x, 13);
  display->print(String(_temperature, 1));
  display->print((char)247);
  display->println("C");

  display->setCursor(x, 24);
  display->print(String(_humidity, 0));
  display->print("%");

  display->display();
}
