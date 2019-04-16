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

RLCMessage RLCMessageFactory::SendClientInfo(ClientStateEnum clientState)
{
	RLCMessage message = RLCMessage();
	message.SourceType = SourceTypeEnum::Client;
	message.MessageType = MessageTypeEnum::ClientInfo;
	message.Key = key;
	message.ClientNumber = clientNumber;
	message.ClientState = clientState;
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
