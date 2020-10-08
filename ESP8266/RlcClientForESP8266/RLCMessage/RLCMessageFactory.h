#pragma once

#include "RLCMessage.h"
#include "RLCMessageParser.h"

class RLCMessageFactory
{
public:
	RLCMessageFactory();
	RLCMessageFactory(uint32_t key, uint16_t clientNumber);
	~RLCMessageFactory();
	RLCMessage SendState(ClientStateEnum clientState);
	RLCMessage SendState(ClientStateEnum clientState, uint16_t batteryCharge);
	RLCMessage RequestServerIP(ClientStateEnum clientState);
private:
	uint32_t key;
	uint16_t clientNumber;
};

