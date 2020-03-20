#pragma once

#include <ILogger.h>
#include <Arduino.h>

class SerialLogger : public ILogger
{
private:
	bool isEnabled;
	unsigned long baudRate;

public:
	SerialLogger(unsigned long baudRate);
	void SetBaudRate(unsigned long baudRate);

	virtual bool IsEnabled();
	virtual void Enable();
	virtual void Disable();
	virtual void PrintNewLine();
	virtual void Print(const char message[], bool newLine);
	virtual void Print(const char message[], const char content[], bool newLine);
	virtual void Print(const char message[], char content, bool newLine);
	virtual void Print(const char message[], unsigned char content, bool newLine);
	virtual void Print(const char message[], int content, bool newLine);
	virtual void Print(const char message[], unsigned int content, bool newLine);
	virtual void Print(const char message[], long content, bool newLine);
	virtual void Print(const char message[], unsigned long content, bool newLine);
	virtual void Print(const char message[], long long content, bool newLine);
	virtual void Print(const char message[], unsigned long long content, bool newLine);
	virtual void Print(const char message[], double content, bool newLine);
};

