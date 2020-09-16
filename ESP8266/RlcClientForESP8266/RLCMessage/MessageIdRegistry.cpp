#include "MessageIdRegistry.h"

MessageIdRegistry::MessageIdRegistry(uint8_t poolSize = 10)
{
	MessageIdRegistry::poolSize = poolSize;
	messageIds = new int32_t[poolSize];
	memset(messageIds, 0, poolSize);
	index = 0;
}

MessageIdRegistry::~MessageIdRegistry()
{
}

bool MessageIdRegistry::Contains(int32_t messageId)
{
	for(uint8_t i = 0; i < poolSize; i++)
	{
		if(messageIds[i] == messageId) {
			return true;
		}
	}
	return false;
}

void MessageIdRegistry::AppendId(int32_t messageId)
{
	messageIds[index] = messageId;
	IncrementIndex();
}

void MessageIdRegistry::IncrementIndex()
{
	if(index == poolSize - 1) {
		index = 0;
	}
	index++;
}
