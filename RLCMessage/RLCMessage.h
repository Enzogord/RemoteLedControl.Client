#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "RLCEnums.h"

#define RLC_MESSAGE_LENGTH 200

class RLCMessage
{
public:
	RLCMessage();
	~RLCMessage();

	bool IsInitialized;

	SourceTypeEnum SourceType;
	uint32_t Key;
	MessageTypeEnum MessageType;;
	uint16_t ClientNumber;
	ClientStateEnum ClientState;
	IPAddress IP;
	uint32_t TimeFrame;

	uint8_t* GetBytes();

private:

};

