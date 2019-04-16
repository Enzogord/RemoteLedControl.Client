#include "RLCMessage.h"



RLCMessage::RLCMessage()
{
	SourceType = SourceTypeEnum::NotSet;
	Key = 0;
	MessageType = MessageTypeEnum::NotSet;
	ClientNumber = 0;
	ClientState = ClientStateEnum::NotSet;
	IP = IPAddress(0, 0, 0, 0);
	TimeFrame = 0;
}

RLCMessage::~RLCMessage()
{
}

uint8_t* RLCMessage::GetBytes()
{
	uint8_t *messageBuffer = new uint8_t[RLC_MESSAGE_LENGTH];
	memset(messageBuffer, 0, RLC_MESSAGE_LENGTH);
	uint8_t byteIndex = 0;

	messageBuffer[byteIndex++] = (uint8_t)SourceType;

	messageBuffer[byteIndex++] = (uint8_t)(Key >> 24);
	messageBuffer[byteIndex++] = (uint8_t)(Key >> 16);
	messageBuffer[byteIndex++] = (uint8_t)(Key >> 8);
	messageBuffer[byteIndex++] = (uint8_t)(Key >> 0);

	messageBuffer[byteIndex++] = (uint8_t)MessageType;

	messageBuffer[byteIndex++] = (uint8_t)(ClientNumber >> 8);
	messageBuffer[byteIndex++] = (uint8_t)(ClientNumber >> 0);

	messageBuffer[byteIndex++] = (uint8_t)ClientState;

	messageBuffer[byteIndex++] = (uint8_t)(IP[0]);
	messageBuffer[byteIndex++] = (uint8_t)(IP[1]);
	messageBuffer[byteIndex++] = (uint8_t)(IP[2]);
	messageBuffer[byteIndex++] = (uint8_t)(IP[3]);

	messageBuffer[byteIndex++] = (uint8_t)(TimeFrame >> 24);
	messageBuffer[byteIndex++] = (uint8_t)(TimeFrame >> 16);
	messageBuffer[byteIndex++] = (uint8_t)(TimeFrame >> 8);
	messageBuffer[byteIndex++] = (uint8_t)(TimeFrame >> 0);

	return messageBuffer;
}
