#include "RLCMessageFactory.h"


RLCMessageFactory::RLCMessageFactory()
{
}

RLCMessageFactory::RLCMessageFactory(uint32_t projectKey, uint16_t clientNumber)
{
	key = projectKey;
	RLCMessageFactory::clientNumber = clientNumber;
}


RLCMessageFactory::~RLCMessageFactory()
{
}

RLCMessage RLCMessageFactory::SendState(ClientStateEnum clientState)
{
	RLCMessage message = RLCMessage();
	message.SourceType = SourceTypeEnum::Client;
	message.MessageType = MessageTypeEnum::State;
	message.Key = key;
	message.ClientNumber = clientNumber;
	message.ClientState = clientState;
	return message;
}

RLCMessage RLCMessageFactory::SendState(ClientStateEnum clientState, uint16_t batteryCharge)
{
	RLCMessage message = RLCMessage();
	message.SourceType = SourceTypeEnum::Client;
	message.MessageType = MessageTypeEnum::State;
	message.Key = key;
	message.ClientNumber = clientNumber;
	message.ClientState = clientState;
	message.BatteryCharge = batteryCharge;
	return message;
}

RLCMessage RLCMessageFactory::RequestServerIP(ClientStateEnum clientState)
{
	RLCMessage message = RLCMessage();
	message.SourceType = SourceTypeEnum::Client;
	message.MessageType = MessageTypeEnum::RequestServerIp;
	message.Key = key;
	message.ClientNumber = clientNumber;
	message.ClientState = clientState;
	return message;
}
