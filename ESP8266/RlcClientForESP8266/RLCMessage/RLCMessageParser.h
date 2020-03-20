#pragma once

#include "RLCMessage.h"
#include <SNTP/SntpFunctions.h>

class RLCMessageParser
{
public:
	RLCMessageParser();
	~RLCMessageParser();

	RLCMessage Parse(uint8_t messageBuffer[]);
	bool TryParseSourceType(SourceTypeEnum &sourceType, uint8_t value);
	bool TryParseMessageType(MessageTypeEnum &messageType, uint8_t value);
	bool TryParseClientState(ClientStateEnum & sourceType, uint8_t value);
};

