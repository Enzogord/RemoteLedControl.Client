#pragma once

#include "RLCMessage.h"
#include "RLCMessageParser.h"

class RLCMessageFactory
{
public:
	RLCMessageFactory();
	RLCMessageFactory(uint32_t key, uint16_t clientNumber);
	~RLCMessageFactory();
	RLCMessage SendClientInfo(ClientStateEnum clientState);
	RLCMessage RequestServerIP(ClientStateEnum clientState);
	RLCMessage BatteryCharge(ClientStateEnum clientState, uint16_t batteryCharge);
private:
	uint32_t key;
	uint16_t clientNumber;
};

