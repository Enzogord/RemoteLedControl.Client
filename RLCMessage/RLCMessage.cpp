#include "RLCMessage.h"

RLCMessage::RLCMessage()
{
	SourceType = SourceTypeEnum::NotSet;
	Key = 0;
	MessageType = MessageTypeEnum::NotSet;
	ClientNumber = 0;
	ClientState = ClientStateEnum::NotSet;
	IP = IPAddress(0, 0, 0, 0);
	LaunchTime = Time();
	SendTime = Time();
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

	LaunchTime.SetSecondsTo(messageBuffer, byteIndex);
	byteIndex += 4;
	
	LaunchTime.SetSecondFractionsTo(messageBuffer, byteIndex);
	byteIndex += 4;

	SendTime.SetSecondsTo(messageBuffer, byteIndex);
	byteIndex += 4;

	SendTime.SetSecondFractionsTo(messageBuffer, byteIndex);
	byteIndex += 4;

	return messageBuffer;
}
