#pragma once
#include "RLCSetting.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

class RLCMessageParser
{
private:
	RLCSetting * Setting_ptr;
	uint32* CurrenctTime_ptr;
	IPAddress* ServerIP_ptr;
	WiFiUDP* Udp_ptr;
public:
	RLCMessageParser();
	RLCMessageParser(RLCSetting* settings, uint32* currenctTime_ptr, IPAddress* serverIP_ptr, WiFiUDP* udp_ptr);
	~RLCMessageParser();
	byte Parse();
};

