#ifndef ThermostatIRCtrls_H
#define ThermostatIRCtrls_H

#include <Arduino.h>
#include <assert.h>
#include "IRrecv.h"
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>

class ThermostatIRCtrls {
  public:
    enum TCMode: char { AUTO='4', COOL='2', DEHUMIFY='3', FAN='5', HEAT='1' };
    enum TCSpeed { AUTO1, SPEED1, SPEED2, SPEED3 };
    enum TCTab: long { OFF=0xC0, SWING=0x1A, TAB1=0x16, TAB2=0x12, TAB3=0x0E, TAB4=0x0A, TAB5=0x06 };
    enum TCTemp: long { 
      TEMP16=0x6C, TEMP17=0x6D, TEMP18=0x6E, TEMP19=0x6F, TEMP20=0x70, 
      TEMP21=0x71, TEMP22=0x72, TEMP23=0x73, TEMP24=0x74, TEMP25=0x75, 
      TEMP26=0x76, TEMP27=0x77, TEMP28=0x78, TEMP29=0x79, TEMP30=0x7A, TEMP31=0x7B, TEMP32=0x7C 
    } temp;
    ThermostatIRCtrls(const uint8_t _kRecvPin=0);
    TCMode getMode();
    TCSpeed getSpeed();
    TCTab getTab();
    int getTemp();
    void begin();
    void begin(TCMode mode, TCSpeed speed, TCTab tab, int temp);
    void loop();
    void setOnTemperatureChange(std::function<int(int, int)> func);
    void setOnModeChange(std::function<TCMode(TCMode, TCMode)> func);
    void setOnTabChange(std::function<TCTab(TCTab, TCTab)> func);
    void setOnSpeedChange(std::function<TCSpeed(TCSpeed, TCSpeed)> func);
    void setOnChange(std::function<void(TCSpeed, TCTab, TCMode, int)> func);

  protected:
    String getIRProtocol(const decode_results* const results);
    TCMode getMode(const decode_results* const results);
    TCSpeed getSpeed(const decode_results* const results);
    TCTab getTab(const decode_results* const results);
    int getTemp(const decode_results* const results);
    String stateToString(const decode_results* const results);
    
  private:
    const uint8_t _kTimeout = 50, _kTolerancePercentage = kTolerance;
    const uint16_t _kCaptureBufferSize = 1024, _kMinUnknownSize = 12;
    IRrecv* _irrecv;
    decode_results _results;
    TCMode _mode;
    TCSpeed _speed;
    TCTab _tab;
    int _temp;
    std::function<int(int, int)> _onTemperatureChange;
    std::function<TCMode(TCMode, TCMode)> _onModeChange;
    std::function<TCTab(TCTab, TCTab)> _onTabChange;
    std::function<TCSpeed(TCSpeed, TCSpeed)> _onSpeedChange;
    std::function<void(TCSpeed, TCTab, TCMode, int)> _onChange;
};

inline const String toString(ThermostatIRCtrls::TCMode v) {
  switch (v) {
    case ThermostatIRCtrls::AUTO: return "AUTO";
    case ThermostatIRCtrls::COOL: return "COOL";
    case ThermostatIRCtrls::DEHUMIFY: return "DEHUMIFY";
    case ThermostatIRCtrls::FAN: return "FAN";
    case ThermostatIRCtrls::HEAT: return "HEAT";
    default: return "[Unknown]";
  }
};

inline const String toString(ThermostatIRCtrls::TCSpeed v) {
  switch (v) {
    case ThermostatIRCtrls::AUTO1: return "AUTO";
    case ThermostatIRCtrls::SPEED1: return "SPEED1";
    case ThermostatIRCtrls::SPEED2: return "SPEED2";
    case ThermostatIRCtrls::SPEED3: return "SPEED3";
    default: return "[Unknown]";
  }
};

inline const String toString(ThermostatIRCtrls::TCTab v) {
  switch (v) {
    case ThermostatIRCtrls::OFF: return "OFF";
    case ThermostatIRCtrls::SWING: return "SWING";
    case ThermostatIRCtrls::TAB1: return "TAB1";
    case ThermostatIRCtrls::TAB2: return "TAB2";
    case ThermostatIRCtrls::TAB3: return "TAB3";
    case ThermostatIRCtrls::TAB4: return "TAB4";
    case ThermostatIRCtrls::TAB5: return "TAB5";
    default: return "[Unknown]";
  }
};

#endif
