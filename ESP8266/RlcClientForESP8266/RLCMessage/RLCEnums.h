#pragma once

enum class ClientStateEnum
{
	NotSet = 0,

	Playing = 1,
	Stoped = 2,
	Paused = 3
};

inline const char* ToString(ClientStateEnum clientState)
{
	switch(clientState)
	{
	case ClientStateEnum::NotSet:   return "NotSet";
	case ClientStateEnum::Playing:   return "Playing";
	case ClientStateEnum::Stoped:   return "Stoped";
	case ClientStateEnum::Paused:   return "Paused";
	default:      return "NotSet";
	}
}

enum class MessageTypeEnum
{
	NotSet = 0,

	//common
	State = 1,

	//to client
	SendServerIP = 5,
	RequestClientInfo = 6,

	//to server
	RequestServerIp = 101
};

inline const char* ToString(MessageTypeEnum clientState)
{
	switch(clientState)
	{
	case MessageTypeEnum::NotSet:   return "NotSet";
	case MessageTypeEnum::State:   return "State";
	case MessageTypeEnum::SendServerIP:   return "SendServerIP";
	case MessageTypeEnum::RequestClientInfo:   return "RequestClientInfo";
	case MessageTypeEnum::RequestServerIp:   return "RequestServerIp";
	default:      return "NotSet";
	}
}

enum class SourceTypeEnum
{
	NotSet = 0,
	Server = 1,
	Client = 2
};

inline const char* ToString(SourceTypeEnum clientState)
{
	switch(clientState)
	{
	case SourceTypeEnum::NotSet:   return "NotSet";
	case SourceTypeEnum::Server:   return "Server";
	case SourceTypeEnum::Client:   return "Client";
	default:      return "NotSet";
	}
}