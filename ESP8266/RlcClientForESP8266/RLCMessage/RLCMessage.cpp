#include "RLCMessage.h"

RLCMessage::RLCMessage()
{
	SourceType = SourceTypeEnum::NotSet;
	Key = 0;
	MessageId = 0;
	MessageType = MessageTypeEnum::NotSet;
	ClientNumber = 0;
	ClientState = ClientStateEnum::NotSet;
	IP = IPAddress(0, 0, 0, 0);
	Frame = 0;
	FrameStartTime = Time();
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

	messageBuffer[byteIndex++] = (uint8_t)(MessageId >> 24);
	messageBuffer[byteIndex++] = (uint8_t)(MessageId >> 16);
	messageBuffer[byteIndex++] = (uint8_t)(MessageId >> 8);
	messageBuffer[byteIndex++] = (uint8_t)(MessageId >> 0);

	messageBuffer[byteIndex++] = (uint8_t)MessageType;

	messageBuffer[byteIndex++] = (uint8_t)(ClientNumber >> 8);
	messageBuffer[byteIndex++] = (uint8_t)(ClientNumber >> 0);

	messageBuffer[byteIndex++] = (uint8_t)ClientState;

	messageBuffer[byteIndex++] = (uint8_t)(IP[0]);
	messageBuffer[byteIndex++] = (uint8_t)(IP[1]);
	messageBuffer[byteIndex++] = (uint8_t)(IP[2]);
	messageBuffer[byteIndex++] = (uint8_t)(IP[3]);
	
	messageBuffer[byteIndex++] = (uint8_t)(Frame >> 24);
	messageBuffer[byteIndex++] = (uint8_t)(Frame >> 16);
	messageBuffer[byteIndex++] = (uint8_t)(Frame >> 8);
	messageBuffer[byteIndex++] = (uint8_t)(Frame >> 0);

	SetTimeToArray(FrameStartTime, messageBuffer, byteIndex);
	byteIndex += 8;

	messageBuffer[byteIndex++] = (uint8_t)(BatteryCharge >> 8);
	messageBuffer[byteIndex++] = (uint8_t)(BatteryCharge >> 0);

	return messageBuffer;
}
