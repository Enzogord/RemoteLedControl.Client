#include "PinController.h"

bool IsDigitalOutput;
bool InvertedOutput;

void PinWrite(uint8_t pin, uint8_t value)
{
	if(IsDigitalOutput)
	{
		if(InvertedOutput && value > 0)
		{
			digitalWrite(pin, LOW);
		}
		else if(InvertedOutput && value == 0)
		{
			digitalWrite(pin, HIGH);
		}
		else if(value > 0)
		{
			digitalWrite(pin, HIGH);
		}
		else
		{
			digitalWrite(pin, LOW);
		}
	}
	else
	{
		if(InvertedOutput)
		{
			analogWrite(pin, (uint8_t)~value);
		}
		else
		{
			analogWrite(pin, value);
		}
	}
}
