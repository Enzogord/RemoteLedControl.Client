#pragma once

#include "RLCMessage.h"

class RLCMessageParser
{
public:
	RLCMessageParser();
	~RLCMessageParser();

	RLCMessage Parse(uint8_t messageBuffer[]);
	bool TryParseSourceType(SourceTypeEnum &sourceType, uint8_t value);
	bool TryParseMessageType(MessageTypeEnum &messageType, uint8_t value);
};

