#include "Thermostat.h"

Thermostat::Thermostat(uint8_t pinFan, uint8_t pinCool, uint8_t pinHeat) {
  _pinFan = pinFan;
  _pinCool = pinCool;
  _pinHeat = pinHeat;
  _state = ThermostatState::OFF;
}

void Thermostat::begin(){
  pinMode(_pinHeat, OUTPUT);
  digitalWrite(_pinHeat, HIGH);
  pinMode(_pinCool, OUTPUT);
  digitalWrite(_pinCool, HIGH);
  pinMode(_pinFan, OUTPUT);
  digitalWrite(_pinFan, HIGH);
}

void Thermostat::setState(String state){ setState(strToState(state)); }
void Thermostat::setOnStateChange(std::function<ThermostatState(ThermostatState, ThermostatState)> func){ _onStateChange = func; }
void Thermostat::setOnPointChange(std::function<float(float, float)> func){ _onPointChange = func; }
void Thermostat::setOnTemperatureChange(std::function<float(float, float)> func){ _onTemperatureChange = func; }

ThermostatState Thermostat::getState(){ return _state; }
bool Thermostat::isHeat(){ return _state == ThermostatState::HEAT; }
bool Thermostat::isCool(){ return _state == ThermostatState::COOL; }
bool Thermostat::isOff(){ return _state == ThermostatState::OFF; }
bool Thermostat::isStandby(){ return (isHeat() && _point <= _temperature) || (isCool() && _point >= _temperature); }

ThermostatState Thermostat::strToState(String state){
  if(state == "heat"){ return HEAT; }
  else if(state == "cool"){ return COOL; }
  else return OFF;
}

String Thermostat::stateToStr(ThermostatState state){
  switch(state){
    case HEAT: return "heat";
    case COOL: return "cool";
    default: return "off";
  }
}

void Thermostat::setState(ThermostatState state){
  if(_state == state) return;
  ThermostatState nextState = state;
  if ( _onStateChange != NULL) nextState = _onStateChange(_state, state); 
  _state = nextState;
}

void Thermostat::setPoint(float point){
  if(_point == point) return;
  float nextPoint = point;
  if ( _onPointChange != NULL)  nextPoint = _onPointChange(_point, point); 
  _point = nextPoint;
}

void Thermostat::setTemperature(float temperature){
  if(_temperature == temperature) return;
  float nextTemperature = temperature;
  if ( _onTemperatureChange != NULL) nextTemperature = _onTemperatureChange(_temperature, temperature); 
  _temperature = nextTemperature;
}

void Thermostat::runner(ThermostatState state, float point, float temperature){

  setPoint(point);
  setTemperature(temperature);
  setState(state);
    
  if (isOff() || isStandby()) {
    digitalWrite(_pinHeat, HIGH);
    digitalWrite(_pinCool, HIGH);
    digitalWrite(_pinFan, HIGH);
  } else {
    digitalWrite(_pinFan, LOW);
    if(isCool()){
      digitalWrite(_pinHeat, HIGH);
      digitalWrite(_pinCool, LOW);
    } else if (isHeat()) {
      digitalWrite(_pinHeat, LOW);
      digitalWrite(_pinCool, HIGH);
    }
  }
}
