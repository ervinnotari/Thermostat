#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include "DHT.h"
#include <EEPROM.h>
#include "Thermostat.h"
#include "ThermostatIRCtrls.h"

#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <StreamString.h>

#define HEARTBEAT_INTERVAL 300000 // 5 Minutes 
#define CLOUD_UPDATE 60000 // 1 Minutes 
#define DEFAULT_SCALE "CELSIUS"
#define PIN_FAN  D0
#define PIN_HEAF D1
#define PIN_COOL D2
#define LED_ON   D3
#define DHTPIN   D4
#define PIN_IR   D5
#define PIN_BTN  D6
#define PIN_RST  D7
#define DHTTYPE DHT11

uint64_t heartbeatTimestamp = 0, cloudLastUpdateTimestamp = 0, now, stum = 0, isWifiReseted = false;
bool isPersist = false, isConnected = false;
uint eeAddr = 500, eeAddr2 = 500;
struct { char deviceId[30], apiKey[50]; } sinric;
int btnWifiReset = 0;

struct {
  float pointTemp = 20;
  ThermostatState state;
} data;

DHT dht(DHTPIN, DHTTYPE);
WiFiManager wifiManager;
WiFiManagerParameter sinricApiKey("sinric_apiKey", "Sinric Api Key", "", 50), 
                     sinricDeviceId("sinric_devId", "Sinric Device ID", "", 30);

Thermostat termostato(PIN_FAN, PIN_COOL, PIN_HEAF);
ThermostatIRCtrls control(PIN_IR);

WebSocketsClient webSocket;

void setPowerStateOnServer(String deviceId, String value);
void setSetTemperatureSettingOnServer(String deviceId, float setPoint, String scale, float ambientTemperature, float ambientHumidity);
void setThermostatModeOnServer(String deviceId, String thermostatMode);

void saveConfigCallback() {
  strcpy(sinric.apiKey, sinricApiKey.getValue());
  strcpy(sinric.deviceId, sinricDeviceId.getValue());
  EEPROM.put(eeAddr2, sinric);
  isPersist = true;
}

ThermostatState onChangeStatus(ThermostatState oldST, ThermostatState newST){
  String st = Thermostat::stateToStr(newST).c_str();
  if(isConnected) setThermostatModeOnServer(sinric.deviceId, st);
  Serial.printf("->State change: %s -> %s\n", Thermostat::stateToStr(oldST).c_str(), st.c_str());
  isPersist = true;
  data.state = newST;
  return newST;
}

float onChangePoint(float oldP, float newP){
  if(isConnected) setSetTemperatureSettingOnServer(sinric.deviceId, newP, DEFAULT_SCALE, dht.readTemperature(), dht.readHumidity());
  Serial.printf("->Point change: %f -> %f\n", oldP, newP);
  isPersist = true;
  data.pointTemp = newP;
  return newP;
}

float onChangeTemp(float oldTmp, float newTmp){
  if( ((int)oldTmp) == ((int)newTmp)) return oldTmp;
  if(isConnected && (now - cloudLastUpdateTimestamp) > CLOUD_UPDATE){
    setSetTemperatureSettingOnServer(sinric.deviceId, data.pointTemp, DEFAULT_SCALE, newTmp, dht.readHumidity());
    cloudLastUpdateTimestamp = now;
  }
  Serial.printf("-> Temperature change: %f -> %f\n", oldTmp, newTmp);
  return newTmp;
}

void onChange(ThermostatIRCtrls::TCSpeed speed, ThermostatIRCtrls::TCTab tab, ThermostatIRCtrls::TCMode mode, int temp){
  switch(mode) {
    case ThermostatIRCtrls::COOL: 
      data.state = ThermostatState::COOL;
      break;
    case ThermostatIRCtrls::HEAT: 
      data.state = ThermostatState::HEAT;
      break;
  }
  switch(tab) {
    case ThermostatIRCtrls::OFF: 
      data.state = ThermostatState::OFF;
      break;
  }
  data.pointTemp = temp;
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      isConnected = false;    
      Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
      break;
    case WStype_CONNECTED:
      isConnected = true;
      Serial.printf("[WSc] Service connected to sinric.com at url: %s\n", payload);
      Serial.printf("Waiting for commands from sinric.com ...\n");        
      setThermostatModeOnServer(sinric.deviceId, Thermostat::stateToStr(data.state));
      break;
    case WStype_TEXT: {
      Serial.printf("[WSc] get text: %s\n", payload);
#if ARDUINOJSON_VERSION_MAJOR == 5
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject((char*)payload);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
      DynamicJsonDocument json(1024);
      deserializeJson(json, (char*) payload);      
#endif
      String deviceId = json ["deviceId"];     
      String action = json ["action"];
      String baseValue = json ["value"];
      Serial.println("[WSc] baseValue: " + baseValue);
      
      if (deviceId == String(sinric.deviceId)) {
        if(action == "action.devices.commands.ThermostatTemperatureSetpoint") { 
          String value = json ["value"]["thermostatTemperatureSetpoint"];
          data.pointTemp = value.toFloat();
        }
        else if(action == "action.devices.commands.ThermostatSetMode") { 
          String value = json["value"]["thermostatMode"];
          data.state = Thermostat::strToState(value);
        }
        else if (action == "test") {
          Serial.println("[WSc] received test command from sinric.com");
        }
      }
    }
    break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
    case WStype_ERROR:
      Serial.printf("[WSc] ERROR \n");
    default: break;  
  }
}

void setup() {
  isWifiReseted = true;
  pinMode(LED_ON, OUTPUT);
  digitalWrite(LED_ON, HIGH);
  pinMode(PIN_BTN, INPUT);
  digitalWrite(PIN_RST, HIGH); 
  pinMode(PIN_RST, OUTPUT);
  
  EEPROM.begin(4096);
  Serial.begin(115200);

  EEPROM.get(eeAddr, data);
  eeAddr2 = eeAddr + sizeof(data);
  EEPROM.get(eeAddr2, sinric);

  //strcpy(sinric.apiKey, "b6813c5d-4d50-401e-9629-ef60ca89b09b");
  //strcpy(sinric.deviceId, "5fde8f248a66e26a4b8721ee");
  //data.pointTemp = 30.0;
  //data.state = Thermostat::strToState("cool");

  sinricApiKey.setValue(sinric.apiKey, 50);
  sinricDeviceId.setValue(sinric.deviceId, 30);
  wifiManager.addParameter (&sinricApiKey);
  wifiManager.addParameter (&sinricDeviceId);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.autoConnect("TermostatoAP", "12345678");  
  
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

  Serial.printf("Store -> \n\tPoint: %f\n", data.pointTemp);
  Serial.printf("\tState: %s\n", Thermostat::stateToStr(data.state).c_str());
  Serial.printf("\tSinric api key: %s\n", sinric.apiKey);
  Serial.printf("\tSinric device Id: %s\n", sinric.deviceId);

  if(isnan(data.pointTemp)){ 
    data.pointTemp = 20; 
    EEPROM.put(eeAddr, data);
  }

  digitalWrite(LED_ON, LOW);
  isWifiReseted = false;
}

int teste = 99999;
void loop() {
  now = millis();

  btnWifiReset = digitalRead(PIN_BTN);
  if (termostato.isOff() && !isWifiReseted && btnWifiReset == HIGH) {
    if((now - stum) > 3000) {
      Serial.println("-> WIFI RESET");
      isWifiReseted = true;
      wifiManager.resetSettings();
      webSocket.loop();   
      for(int i=0;i<=10;i++) {
        if((now%500) <= 250) digitalWrite(LED_ON, LOW); 
        else  digitalWrite(LED_ON, HIGH);
        delay(500);
      }
      delay(500);
      digitalWrite(PIN_RST, LOW); 
    }
  } else stum = now;

  if(isPersist) {
    EEPROM.put(eeAddr, data);
    EEPROM.commit();
    Serial.println("-> Save data!");
    isPersist = false;
  }
  webSocket.loop();   
  termostato.runner(data.state, data.pointTemp, dht.readTemperature());
  
  if(termostato.isOff()) digitalWrite(LED_ON, LOW);
  else if(termostato.isStandby() && (now%1000) <= 500) digitalWrite(LED_ON, LOW);
  else digitalWrite(LED_ON, HIGH);
  
  if(Serial.available() > 0) {
    teste = Serial.parseInt();
    if(teste == 99) { wifiManager.resetSettings(); } 
    else if(teste == 1){ data.state = ThermostatState::OFF; }
    else if(teste == 2){ data.state = ThermostatState::HEAT; }
    else if(teste == 3){ data.state = ThermostatState::COOL; }
    else if(teste >= 10 && teste <= 40 ) { data.pointTemp = teste; }
    teste = 99999;
  }
  control.loop();
}

void setPowerStateOnServer(String deviceId, String value) {
#if ARDUINOJSON_VERSION_MAJOR == 5
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
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
  
  Serial.println("[Ws] Power state change!");
  webSocket.sendTXT(databuf);
}

void setSetTemperatureSettingOnServer(String deviceId, float setPoint, String scale, float ambientTemperature, float ambientHumidity) {
#if ARDUINOJSON_VERSION_MAJOR == 5
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
  DynamicJsonDocument root(1024);
#endif        
  root["action"] = "SetTemperatureSetting";
  root["deviceId"] = deviceId;

#if ARDUINOJSON_VERSION_MAJOR == 5
  JsonObject& valueObj = root.createNestedObject("value");
  JsonObject& temperatureSetting = valueObj.createNestedObject("temperatureSetting");
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
  Serial.println("[Ws] Temperature and humidity sended!");
  webSocket.sendTXT(databuf);
}

void setThermostatModeOnServer(String deviceId, String thermostatMode) {
#if ARDUINOJSON_VERSION_MAJOR == 5
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
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

  Serial.println("[Ws] Thermostat mode change!");
  webSocket.sendTXT(databuf);
}
