#ifndef Thermostat_H
#define Thermostat_H

#include "Arduino.h"


enum ThermostatState { OFF, HEAT, COOL };

class Thermostat {
  public:    
    Thermostat(uint8_t pinFan=0,uint8_t pinCool=1,uint8_t pinHeat=2);
    void begin();
    
    void setState(String state);
    void setState(ThermostatState state);
    void setPoint(float point);
    void setTemperature(float temperature);
    void setOnStateChange(std::function<ThermostatState(ThermostatState, ThermostatState)> func);
    void setOnPointChange(std::function<float(float, float)> func);
    void setOnTemperatureChange(std::function<float(float, float)> func);
    
    ThermostatState getState();
    bool isHeat();
    bool isCool();
    bool isOff();
    bool isStandby();
    
    void runner(ThermostatState state, float point, float temperature);
    static ThermostatState strToState(String state);
    static String stateToStr(ThermostatState state);
    
  private:
    uint8_t _pinFan, _pinCool, _pinHeat, _ledOff, _ledStby;
    bool _isStandby = false;
    ThermostatState _state;
    float _point, _temperature;
    std::function<ThermostatState(ThermostatState, ThermostatState)> _onStateChange;
    std::function<float(float, float)> _onPointChange;
    std::function<float(float, float)> _onTemperatureChange;
};

#endif
