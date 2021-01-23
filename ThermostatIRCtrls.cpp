#include "ThermostatIRCtrls.h"

ThermostatIRCtrls::ThermostatIRCtrls(const uint8_t recvpin){
  _irrecv = new IRrecv(recvpin, _kCaptureBufferSize, _kTimeout, true);
}

ThermostatIRCtrls::TCMode ThermostatIRCtrls::getMode(){ return _mode; }
ThermostatIRCtrls::TCSpeed ThermostatIRCtrls::getSpeed(){ return _speed; }
ThermostatIRCtrls::TCTab ThermostatIRCtrls::getTab(){ return _tab; }
int ThermostatIRCtrls::getTemp(){ return _temp; }
void ThermostatIRCtrls::setOnTemperatureChange(std::function<int(int, int)> func) { _onTemperatureChange = func; }
void ThermostatIRCtrls::setOnModeChange(std::function<TCMode(TCMode, TCMode)> func) { _onModeChange = func; }
void ThermostatIRCtrls::setOnTabChange(std::function<TCTab(TCTab, TCTab)> func) { _onTabChange = func; }
void ThermostatIRCtrls::setOnSpeedChange(std::function<TCSpeed(TCSpeed, TCSpeed)> func) { _onSpeedChange = func; }
void ThermostatIRCtrls::setOnChange(std::function<void(TCSpeed, TCTab, TCMode, int)> func) { _onChange = func; }

void ThermostatIRCtrls::begin(ThermostatIRCtrls::TCMode mode, ThermostatIRCtrls::TCSpeed speed, ThermostatIRCtrls::TCTab tab, int temp) {
  _mode=mode;
  _speed=speed;
  _tab=tab;
  _temp=temp;
  ThermostatIRCtrls::begin();
}

void ThermostatIRCtrls::begin(){
  assert(irutils::lowLevelSanityCheck() == 0);
  _irrecv->setUnknownThreshold(_kMinUnknownSize);
  _irrecv->setTolerance(_kTolerancePercentage); 
  _irrecv->enableIRIn();
}

String ThermostatIRCtrls::stateToString(const decode_results* const results) {
  String output = "";
  if (results->decode_type != UNKNOWN) {
    if (hasACState(results->decode_type)) {
      uint16_t nbytes = results->bits / 8;
      output += F("uint8_t state[");
      output += uint64ToString(nbytes);
      output += F("] = {");
      for (uint16_t i = 0; i < nbytes; i++) {
        output += F("0x");
        if (results->state[i] < 0x10) output += '0';
        output += uint64ToString(results->state[i], 16);
        if (i < nbytes - 1) output += kCommaSpaceStr;
      }
      output += F("};");
    } else {
      if (results->address > 0 || results->command > 0) {
        output += F("uint32_t address = 0x");
        output += uint64ToString(results->address, 16);
        output += F(";\n");
        output += F("uint32_t command = 0x");
        output += uint64ToString(results->command, 16);
        output += F(";\n");
      }
      output += F("uint64_t data = 0x");
      output += uint64ToString(results->value, 16);
    }
  }
  output += F("  // ");
  output += typeToString(results->decode_type, results->repeat);
  return output;
}

ThermostatIRCtrls::TCMode ThermostatIRCtrls::getMode(const decode_results* const results) {
  String state = uint64ToString(results->state[4], 16);
  return (ThermostatIRCtrls::TCMode) state.c_str()[0];
}

ThermostatIRCtrls::TCSpeed ThermostatIRCtrls::getSpeed(const decode_results* const results) {
  String state = uint64ToString(results->state[4], 16);
  switch(state.c_str()[1]){
    case '0':
    case '4': return AUTO1;
    
    case '6':
    case '2': return SPEED1;
    
    case '7':
    case '3': return SPEED2;
    
    case '5':
    case '1': return SPEED3;
  }  
}

ThermostatIRCtrls::TCTab ThermostatIRCtrls::getTab(const decode_results* const results) {
  return (ThermostatIRCtrls::TCTab) results->state[5];
}

int ThermostatIRCtrls::getTemp(const decode_results* const results) {
  switch(results->state[1]) {
    case 0x6C: return 16;
    case 0x6D: return 17;
    case 0x6E: return 18;
    case 0x6F: return 19;
    case 0x70: return 20;
    case 0x71: return 21;
    case 0x72: return 22;
    case 0x73: return 23;
    case 0x74: return 24;
    case 0x75: return 25;
    case 0x76: return 26;
    case 0x77: return 27;
    case 0x78: return 28;
    case 0x79: return 29;
    case 0x7A: return 30;
    case 0x7B: return 31;
    case 0x7C: return 32;
  }
  return 0;
}

String ThermostatIRCtrls::getIRProtocol(const decode_results* const results){
  return typeToString(results->decode_type, results->repeat);
}

void ThermostatIRCtrls::loop(){
  if (_irrecv->decode(&_results)) {
    if(getIRProtocol(&_results) == "MIRAGE") {
      TCSpeed _nextSpeed = getSpeed(&_results);
      int _nextTemp = getTemp(&_results);
      TCMode _nextMode = getMode(&_results);
      TCTab _nextTab = getTab(&_results);
      
      if ( _onChange != NULL) _onChange(_nextSpeed, _nextTab, _nextMode, _nextTemp);   
      if ( _onSpeedChange != NULL) _nextSpeed = _onSpeedChange(_speed, _nextSpeed);      
      if ( _onModeChange != NULL) _nextMode = _onModeChange(_mode, _nextMode);
      if ( _onTabChange != NULL) _nextTab = _onTabChange(_tab, _nextTab); 
      if ( _onTemperatureChange != NULL) _nextTemp = _onTemperatureChange(_temp, _nextTemp); 
      
      _mode = _nextMode;
      _speed = _nextSpeed;
      _tab = _nextTab;
      _temp = _nextTemp;
    }
    yield();
  }
}
