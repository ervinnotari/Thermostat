/*
  ButtonEvent.h - Event-Based Library for Arduino.
  Copyright (c) 2011, Renato A. Ferreira
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of the author nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef ButtonEvent_h
#define ButtonEvent_h

#include <stdlib.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define NOT_ANALOG -99

struct ButtonInformation
{
  short pin;
  short analogValue;
  byte deviation;
  bool pressed;
  bool hold;
  bool stuned;
  unsigned long startMillis;
  unsigned long holdMillis;
  unsigned long holdMillisWait;
  unsigned long doubleMillis;
  unsigned long doubleMillisWait;
  unsigned long stumMillis;
  unsigned long stumMillisWait;
  void (*onDown)(ButtonInformation *Sender);
  void (*onUp)(ButtonInformation *Sender);
  void (*onHold)(ButtonInformation *Sender);
  void (*onDouble)(ButtonInformation *Sender);
  void (*onStum)(ButtonInformation *Sender);
};

class ButtonEventClass
{
public:
  typedef void (*ButtonEvent)(ButtonInformation *sender);

  ButtonEventClass();
  short initialCapacity;
  void addButton(short pin);
  void addButton(short pin, ButtonEvent onDown, ButtonEvent onUp, ButtonEvent onHold, unsigned long holdMillisWait, ButtonEvent onDouble, unsigned long doubleMillisWait);
  void addButton(short pin, short analogValue, byte deviation, ButtonEvent onDown, ButtonEvent onUp, ButtonEvent onHold, unsigned long holdMillisWait, ButtonEvent onDouble, unsigned long doubleMillisWait);
  void loop();
  void setHoldEvent(short position, ButtonEvent event, unsigned long holdMillisWait = 1000);
  void setDownEvent(short position, ButtonEvent event);
  void setUpEvent(short position, ButtonEvent event);
  void setDoubleEvent(short position, ButtonEvent event, unsigned long doubleMillisWait = 500);
  void setStumEvent(short position, ButtonEvent event, unsigned long stumMillisWait = 3000);

private:
  bool nextPressed;
  short nextAnalogRead;
  short count;
  short mallocSize;
  short index;
  unsigned long lastMillis;
  ButtonInformation *buttons;
  ButtonInformation *currentButton;
  void setPosition(short Position);
  void buttonAgragate();
};

//global instance
extern ButtonEventClass ButtonEvent;

#endif