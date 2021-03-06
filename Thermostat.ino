#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include "DHT.h"
#include <EEPROM.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <StreamString.h>
#include "Thermostat.h"
#include "ThermostatIRCtrls.h"
#include "ThermostatDisplay.h"
#include "ButtonEvent.h"

#define DEBUG true

#define HEARTBEAT_INTERVAL 300000 // 5 Minutes
#define CLOUD_UPDATE 60000        // 1 Minutes
#define FLASH_STUM 3000           // 3 Secunds

#define DEFAULT_SCALE "CELSIUS"
#define DHTTYPE DHT11
#define WIFI_SSID "TermostatoAP"
#define WIFI_PASS "123456789"

#define PIN_FAN D0
#define PIN_SDA D1
#define PIN_SCL D2
#define PIN_BTN D3
#define PIN_LED D4
#define PIN_COOL D5
#define PIN_HEAT D6
#define PIN_IR D7
#define PIN_DHT D9

uint64_t heartbeatTimestamp = 0, cloudLastUpdateST = 0, stum = 0, now;
bool isPersist = false, isConnected = false, isWifiReseted = false;



uint eeAddr = 500, eeAddr2 = 500;
int btnWifiReset = 0, debugRead = 99999;

struct
{
  char deviceId[30], apiKey[50];
} sinric;
struct
{
  float pointTemp = 20;
  ThermostatState state;
} data;

DHT dht(PIN_DHT, DHTTYPE);
WiFiManager wifiManager;
WebSocketsClient webSocket;
WiFiManagerParameter
    sinricApiKey("sinric_apiKey", "Sinric Api Key", "", 50),
    sinricDeviceId("sinric_devId", "Sinric Device ID", "", 30);
Thermostat termostato(PIN_FAN, PIN_COOL, PIN_HEAT);
ThermostatIRCtrls control(PIN_IR);
ThermostatDisplay display(PIN_SDA, PIN_SCL);

void setPowerStateOnServer(String deviceId, String value);
void setSetTemperatureSettingOnServer(String deviceId, float setPoint, String scale, float ambientTemperature, float ambientHumidity);
void setThermostatModeOnServer(String deviceId, String thermostatMode);
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);
void onChange(ThermostatIRCtrls::TCSpeed speed, ThermostatIRCtrls::TCTab tab, ThermostatIRCtrls::TCMode mode, int temp);
float onChangeTemp(float oldTmp, float newTmp);
float onChangePoint(float oldP, float newP);
void onStum(ButtonInformation *sender);
ThermostatState onChangeStatus(ThermostatState oldST, ThermostatState newST);
void saveConfigCallback();

void setup()
{
  ButtonEvent.addButton(PIN_BTN);
  ButtonEvent.setStumEvent(PIN_BTN, onStum);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);

#if DEBUG
  Serial.begin(115200);
#endif
  display.begin();
  EEPROM.begin(4096);
  EEPROM.get(eeAddr, data);
  eeAddr2 = eeAddr + sizeof(data);
  EEPROM.get(eeAddr2, sinric);

  sinricApiKey.setValue(sinric.apiKey, 50);
  sinricDeviceId.setValue(sinric.deviceId, 30);
  wifiManager.addParameter(&sinricApiKey);
  wifiManager.addParameter(&sinricDeviceId);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  if (WiFi.status() == WL_CONNECT_FAILED || WiFi.SSID().isEmpty())
  {
    display.setWifi(WIFI_SSID);
    display.showApModeScreen();
  }
  else
  {
    display.setWifi(WiFi.SSID());
    display.showLoaderScreen();
  }

  wifiManager.autoConnect(WIFI_SSID, WIFI_PASS);
  wifiManager.setDebugOutput(DEBUG);

  display.setWifi(WiFi.SSID());
  display.showLoaderScreen();

  webSocket.begin("iot.sinric.com", 80, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", sinric.apiKey);
  webSocket.setReconnectInterval(5000);

  dht.begin();
  termostato.begin();
  termostato.setOnStateChange(onChangeStatus);
  termostato.setOnPointChange(onChangePoint);
  termostato.setOnTemperatureChange(onChangeTemp);
  control.setOnChange(onChange);
  control.begin();

#if DEBUG
  Serial.printf("Store -> \n\tPoint: %f\n", data.pointTemp);
  Serial.printf("\tState: %s\n", Thermostat::stateToStr(data.state).c_str());
  Serial.printf("\tSinric api key: %s\n", sinric.apiKey);
  Serial.printf("\tSinric device Id: %s\n", sinric.deviceId);
#endif

  if (isnan(data.pointTemp))
  {
    data.pointTemp = 20;
    EEPROM.put(eeAddr, data);
  }
  digitalWrite(PIN_LED, LOW);
}

void loop()
{
  now = millis();
  display.setWifi(WiFi.SSID());

  display.setWifi(WiFi.SSID());
  display.setEnable(!termostato.isOff());

  if (isPersist)
  {
    EEPROM.put(eeAddr, data);
    EEPROM.commit();
    isPersist = false;
#if DEBUG
    Serial.println("-> Save data!");
#endif
  }

  display.setEnable(!termostato.isOff());
  if (termostato.isOff())
    digitalWrite(PIN_LED, LOW);
  else if (termostato.isStandby() && (now % 1000) <= 500)
    digitalWrite(PIN_LED, LOW);
  else
    digitalWrite(PIN_LED, HIGH);

#if DEBUG
  if (Serial.available() > 0)
  {
    debugRead = Serial.parseInt();
    if (debugRead == 99)
    {
      wifiManager.resetSettings();
    }
    else if (debugRead == 1)
    {
      data.state = ThermostatState::OFF;
    }
    else if (debugRead == 2)
    {
      data.state = ThermostatState::HEAT;
    }
    else if (debugRead == 3)
    {
      data.state = ThermostatState::COOL;
    }
    else if (debugRead >= 10 && debugRead <= 40)
    {
      data.pointTemp = debugRead;
    }
    debugRead = 99999;
  }
#endif

  control.loop();
  webSocket.loop();
  termostato.runner(data.state, data.pointTemp, dht.readTemperature());
  display.loop();
  ButtonEvent.loop();
}

void saveConfigCallback()
{
  strcpy(sinric.apiKey, sinricApiKey.getValue());
  strcpy(sinric.deviceId, sinricDeviceId.getValue());
  EEPROM.put(eeAddr2, sinric);
  isPersist = true;
}

void onStum(ButtonInformation *sender)
{
#if DEBUG
  Serial.println("-> Flash Stuned");
#endif
  if (termostato.isOff() && !WiFi.SSID().isEmpty())
  {
    for (size_t i = 0; i < 10; i++)
    {
      digitalWrite(PIN_LED, (i % 2) ? HIGH : LOW);
      delay(500);
    }
    wifiManager.resetSettings();
#if DEBUG
    Serial.println("-> Wifi reset");
#endif
  }
}

ThermostatState onChangeStatus(ThermostatState oldST, ThermostatState newST)
{
  String st = Thermostat::stateToStr(newST).c_str();
  if (isConnected)
    setThermostatModeOnServer(sinric.deviceId, st);
#if DEBUG
  Serial.printf("->State change: %s -> %s\n", Thermostat::stateToStr(oldST).c_str(), st.c_str());
#endif
  isPersist = true;
  data.state = newST;
  display.setThermState(newST);
  return newST;
}

float onChangePoint(float oldP, float newP)
{
  if (isConnected)
    setSetTemperatureSettingOnServer(sinric.deviceId, newP, DEFAULT_SCALE, dht.readTemperature(), dht.readHumidity());
#if DEBUG
  Serial.printf("->Point change: %f -> %f\n", oldP, newP);
#endif
  isPersist = true;
  data.pointTemp = newP;
  display.setPoint(data.pointTemp);
  return newP;
}

float onChangeTemp(float oldTmp, float newTmp)
{
  if (((int)oldTmp) == ((int)newTmp))
    return oldTmp;
  int hum = dht.readHumidity();

  if (isConnected && (now - cloudLastUpdateST) > CLOUD_UPDATE)
  {
    setSetTemperatureSettingOnServer(sinric.deviceId, data.pointTemp, DEFAULT_SCALE, newTmp, hum);
    cloudLastUpdateST = now;
  }
  display.setTemperature(newTmp);
  display.setHumidity(hum);
#if DEBUG
  Serial.printf("-> Temperature change: %f -> %f\n", oldTmp, newTmp);
#endif
  return newTmp;
}

void onChange(ThermostatIRCtrls::TCSpeed speed, ThermostatIRCtrls::TCTab tab, ThermostatIRCtrls::TCMode mode, int temp)
{
  data.pointTemp = temp;
  switch (mode)
  {
  case ThermostatIRCtrls::COOL:
    data.state = ThermostatState::COOL;
    break;
  case ThermostatIRCtrls::HEAT:
    data.state = ThermostatState::HEAT;
    break;
  case ThermostatIRCtrls::AUTO:
    if (dht.readTemperature() > data.pointTemp)
      data.state = ThermostatState::COOL;
    else
      data.state = ThermostatState::HEAT;
    break;
  case ThermostatIRCtrls::FAN:
    data.state = ThermostatState::FAN;
    break;
  default:
    break;
  }
  switch (tab)
  {
  case ThermostatIRCtrls::OFF:
    data.state = ThermostatState::OFF;
    break;
  default:
    break;
  }
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    isConnected = false;
#if DEBUG
    Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
#endif
    break;
  case WStype_CONNECTED:
    isConnected = true;
#if DEBUG
    Serial.printf("[WSc] Service connected to sinric.com at url: %s\n", payload);
    Serial.printf("Waiting for commands from sinric.com ...\n");
#endif
    setThermostatModeOnServer(sinric.deviceId, Thermostat::stateToStr(data.state));
    break;
  case WStype_TEXT:
  {
#if DEBUG
    Serial.printf("[WSc] get text: %s\n", payload);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 5
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.parseObject((char *)payload);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6
    DynamicJsonDocument json(1024);
    deserializeJson(json, (char *)payload);
#endif
    String deviceId = json["deviceId"];
    String action = json["action"];
    String baseValue = json["value"];
#if DEBUG
    Serial.println("[WSc] baseValue: " + baseValue);
#endif
    if (deviceId == String(sinric.deviceId))
    {
      if (action == "action.devices.commands.ThermostatTemperatureSetpoint")
      {
        String value = json["value"]["thermostatTemperatureSetpoint"];
        data.pointTemp = value.toFloat();
      }
      else if (action == "action.devices.commands.ThermostatSetMode")
      {
        String value = json["value"]["thermostatMode"];
        data.state = Thermostat::strToState(value);
      }
#if DEBUG
      else if (action == "test")
      {
        Serial.println("[WSc] received test command from sinric.com");
      }
#endif
    }
  }
  break;
#if DEBUG
  case WStype_BIN:
    Serial.printf("[WSc] get binary length: %u\n", length);
    break;
  case WStype_ERROR:
    Serial.printf("[WSc] ERROR \n");
  default:
    break;
#endif
  }
}

void setPowerStateOnServer(String deviceId, String value)
{
#if ARDUINOJSON_VERSION_MAJOR == 5
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6
  DynamicJsonDocument root(1024);
#endif
  root["deviceId"] = deviceId;
  root["action"] = "setPowerState";
  root["value"] = value;
  StreamString databuf;
#if ARDUINOJSON_VERSION_MAJOR == 5
  root.printTo(databuf);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6
  serializeJson(root, databuf);
#endif
#if DEBUG
  Serial.println("[Ws] Power state change!");
#endif
  webSocket.sendTXT(databuf);
}

void setSetTemperatureSettingOnServer(String deviceId, float setPoint, String scale, float ambientTemperature, float ambientHumidity)
{
#if ARDUINOJSON_VERSION_MAJOR == 5
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6
  DynamicJsonDocument root(1024);
#endif
  root["action"] = "SetTemperatureSetting";
  root["deviceId"] = deviceId;

#if ARDUINOJSON_VERSION_MAJOR == 5
  JsonObject &valueObj = root.createNestedObject("value");
  JsonObject &temperatureSetting = valueObj.createNestedObject("temperatureSetting");
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6
  JsonObject valueObj = root.createNestedObject("value");
  JsonObject temperatureSetting = valueObj.createNestedObject("temperatureSetting");
#endif
  temperatureSetting["setPoint"] = setPoint;
  temperatureSetting["scale"] = scale;
  temperatureSetting["ambientTemperature"] = ambientTemperature;
  temperatureSetting["ambientHumidity"] = ambientHumidity;

  StreamString databuf;
#if ARDUINOJSON_VERSION_MAJOR == 5
  root.printTo(databuf);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6
  serializeJson(root, databuf);
#endif
#if DEBUG
  Serial.println("[Ws] Temperature and humidity sended!");
#endif
  webSocket.sendTXT(databuf);
}

void setThermostatModeOnServer(String deviceId, String thermostatMode)
{
#if ARDUINOJSON_VERSION_MAJOR == 5
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6
  DynamicJsonDocument root(1024);
#endif
  root["deviceId"] = deviceId;
  root["action"] = "SetThermostatMode";
  root["value"] = thermostatMode;
  StreamString databuf;
#if ARDUINOJSON_VERSION_MAJOR == 5
  root.printTo(databuf);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6
  serializeJson(root, databuf);
#endif
#if DEBUG
  Serial.println("[Ws] Thermostat mode change!");
#endif
  webSocket.sendTXT(databuf);
}
