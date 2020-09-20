#pragma once
#include "../../RLCLibrary/Infrastructure/GlobalTypes.h"
#include <Arduino.h>

class MessageIdRegistry
{
public:
	MessageIdRegistry();
	MessageIdRegistry(uint8_t poolSize);
	~MessageIdRegistry();

	bool Contains(int32_t messageId);
	void AppendId(int32_t messageId);

private:
	uint8_t poolSize;
	int32_t* messageIds;
	uint8_t index;
	void IncrementIndex();
};