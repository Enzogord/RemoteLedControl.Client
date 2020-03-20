#include "SerialLogger.h"

SerialLogger::SerialLogger(unsigned long baudRate)
{
	SerialLogger::baudRate = baudRate;
}

void SerialLogger::SetBaudRate(unsigned long baudRate)
{
	SerialLogger::baudRate = baudRate;
	if(isEnabled)
	{
		Disable();
		Enable();
	}
}

bool SerialLogger::IsEnabled()
{
	return isEnabled;
}

void SerialLogger::Enable()
{
	Serial.begin(baudRate);
	isEnabled = true;
}

void SerialLogger::Disable()
{
	Serial.end();
	isEnabled = false;
}

void SerialLogger::PrintNewLine()
{
	if(!isEnabled)
	{
		return;
	}
	Serial.println();
}

void SerialLogger::Print(const char message[], bool newLine = true)
{
	if (!isEnabled)
	{
		return;
	}
	Serial.print(message);
	if (newLine)
	{
		Serial.println();
	}
}

void SerialLogger::Print(const char message[], const char content[], bool newLine = true)
{
	if (!isEnabled)
	{
		return;
	}
	Serial.print(message);
	Serial.print(content);
	if (newLine)
	{
		Serial.println();
	}
}

void SerialLogger::Print(const char message[], char content, bool newLine = true)
{
	if (!isEnabled)
	{
		return;
	}
	Serial.print(message);
	Serial.print(content);
	if (newLine)
	{
		Serial.println();
	}
}

void SerialLogger::Print(const char message[], unsigned char content, bool newLine = true)
{
	if (!isEnabled)
	{
		return;
	}
	Serial.print(message);
	Serial.print(content);
	if (newLine)
	{
		Serial.println();
	}
}

void SerialLogger::Print(const char message[], int content, bool newLine = true)
{
	if (!isEnabled)
	{
		return;
	}
	Serial.print(message);
	Serial.print(content);
	if (newLine)
	{
		Serial.println();
	}
}

void SerialLogger::Print(const char message[], unsigned int content, bool newLine = true)
{
	if (!isEnabled)
	{
		return;
	}
	Serial.print(message);
	Serial.print(content);
	if (newLine)
	{
		Serial.println();
	}
}

void SerialLogger::Print(const char message[], long content, bool newLine = true)
{
	if (!isEnabled)
	{
		return;
	}
	Serial.print(message);
	Serial.print(content);
	if (newLine)
	{
		Serial.println();
	}
}

void SerialLogger::Print(const char message[], unsigned long content, bool newLine = true)
{
	if (!isEnabled)
	{
		return;
	}
	Serial.print(message);
	Serial.print(content);
	if (newLine)
	{
		Serial.println();
	}
}

void SerialLogger::Print(const char message[], long long content, bool newLine)
{
	Serial.print(message);
	if(content < 0) {
		Serial.print("-");
		content *= -1;
	}
	Print("", (unsigned long long)content, newLine);
}

void SerialLogger::Print(const char message[], unsigned long long content, bool newLine)
{
	Serial.print(message);
	char rev[128];
	char* p = rev + 1;

	while(content > 0) {
		*p++ = '0' + (content % 10);
		content /= 10;
	}
	p--;
	/*Print the number which is now in reverse*/
	while(p > rev) {
		Serial.print(*p--);
	}
	if(newLine) {
		Serial.println();
	}
}

void SerialLogger::Print(const char message[], double content, bool newLine = true)
{
	if (!isEnabled)
	{
		return;
	}
	Serial.print(message);
	Serial.print(content);
	if (newLine)
	{
		Serial.println();
	}
}
