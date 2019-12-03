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

	//to client
	Play = 1,
	Stop = 2,
	Pause = 3,
	PlayFrom = 4,
	SendServerIP = 5,
	RequestClientInfo = 6,
	Rewind = 7,

	//to server
	ClientInfo = 100,
	RequestServerIp = 101,
	BatteryCharge = 102
};

inline const char* ToString(MessageTypeEnum clientState)
{
	switch(clientState)
	{
	case MessageTypeEnum::NotSet:   return "NotSet";
	case MessageTypeEnum::Play:   return "Play";
	case MessageTypeEnum::Stop:   return "Stop";
	case MessageTypeEnum::Pause:   return "Pause";
	case MessageTypeEnum::PlayFrom:   return "PlayFrom";
	case MessageTypeEnum::SendServerIP:   return "SendServerIP";
	case MessageTypeEnum::RequestClientInfo:   return "RequestClientInfo";
	case MessageTypeEnum::Rewind:   return "Rewind";
	case MessageTypeEnum::ClientInfo:   return "ClientInfo";
	case MessageTypeEnum::RequestServerIp:   return "RequestServerIp";
	case MessageTypeEnum::BatteryCharge:   return "BatteryCharge";
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