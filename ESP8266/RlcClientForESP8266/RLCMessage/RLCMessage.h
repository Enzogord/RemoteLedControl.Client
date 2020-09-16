#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "RLCEnums.h"
#include <Time.h>
#include <SNTP/SntpFunctions.h>

#define RLC_MESSAGE_LENGTH 200

class RLCMessage
{
public:
	RLCMessage();
	~RLCMessage();

	bool IsInitialized;

	SourceTypeEnum SourceType;
	uint32_t Key;
	int32_t MessageId;
	MessageTypeEnum MessageType;;
	uint16_t ClientNumber;
	ClientStateEnum ClientState;
	IPAddress IP;
	Time PlayFromTime;
	Time SendTime;
	uint16_t BatteryCharge;

	uint8_t* GetBytes();

private:

};

