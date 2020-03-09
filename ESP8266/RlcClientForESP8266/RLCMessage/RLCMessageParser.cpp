#include "RLCMessageParser.h"

RLCMessageParser::RLCMessageParser()
{
}


RLCMessageParser::~RLCMessageParser()
{
}

RLCMessage RLCMessageParser::Parse(uint8_t messageBuffer[])
{
	RLCMessage message = RLCMessage();
	int index = 0;

	//Source type
	uint8_t sourceType = messageBuffer[index++];
	if(!TryParseSourceType(message.SourceType, sourceType))
	{		
		message.IsInitialized = false;
		return message;
	}

	//Key
	message.Key = (messageBuffer[index++] << 24) + (messageBuffer[index++] << 16) + (messageBuffer[index++] << 8) + (messageBuffer[index++]);
	
	//MessageType
	if (!TryParseMessageType(message.MessageType, messageBuffer[index++]))
	{
		message.IsInitialized = false;
		return message;
	}

	//ClientNumber
	//Not implemented
	//Нет необходимоти в номере клиента для входящего сообщения
	index += 2; //занимает 2 байта

	//ClientState
	if(!TryParseClientState(message.ClientState, messageBuffer[index++]))
	{
	}

	//IPAddress
	message.IP = IPAddress(messageBuffer[index++], messageBuffer[index++], messageBuffer[index++], messageBuffer[index++]);
	
	//Playfrom time
	message.PlayFromTime = Time(messageBuffer, index);
	index += 8;

	//Send time
	message.SendTime = Time(messageBuffer, index);
	index += 8;

	//BatteryCharge
	//Not implemented
	//Нет необходимоти в уровне заряда клиента для входящего сообщения
	index += 2; //занимает 2 байта

	return message;
}

bool RLCMessageParser::TryParseSourceType(SourceTypeEnum & sourceType, uint8_t value)
{
	switch (value)
	{
		case(1):
			sourceType = SourceTypeEnum::Server;
			return true;
		case(2):
			sourceType = SourceTypeEnum::Client;
			return true;
		default:
			sourceType = SourceTypeEnum::NotSet;
			return false;
	}
}

bool RLCMessageParser::TryParseMessageType(MessageTypeEnum & messageType, uint8_t value)
{
	switch (value)
	{
			//to client
		case(1):
			messageType = MessageTypeEnum::Play;
			return true;
		case(2):
			messageType = MessageTypeEnum::Stop;
			return true;
		case(3):
			messageType = MessageTypeEnum::Pause;
			return true;
		case(4):
			messageType = MessageTypeEnum::PlayFrom;
			return true;
		case(5):
			messageType = MessageTypeEnum::SendServerIP;
			return true;
		case(6):
			messageType = MessageTypeEnum::RequestClientInfo;
			return true;
		case(7):
			messageType = MessageTypeEnum::Rewind;
			return true;

			//to server
		case(100):
			messageType = MessageTypeEnum::ClientInfo;
			return true;
		case(101):
			messageType = MessageTypeEnum::RequestServerIp;
			return true;
		case(102):
			messageType = MessageTypeEnum::BatteryCharge;
			return true;
		default:
			messageType = MessageTypeEnum::NotSet;
			return false;
	}
}
bool RLCMessageParser::TryParseClientState(ClientStateEnum & clientState, uint8_t value)
{
	switch(value)
	{
	case(1):
		clientState = ClientStateEnum::Playing;
		return true;
	case(2):
		clientState = ClientStateEnum::Stoped;
		return true;
	case(3):
		clientState = ClientStateEnum::Paused;
		return true;
	default:
		clientState = ClientStateEnum::NotSet;
		return false;
	}
}
