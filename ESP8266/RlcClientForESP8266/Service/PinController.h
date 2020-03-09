

#ifndef PIN_CONTROLLER_H
#define PIN_CONTROLLER_H

#pragma once
#include <Arduino.h>

extern bool IsDigitalOutput;
extern bool InvertedOutput;

void PinWrite(uint8_t pin, uint8_t value);

#endif


