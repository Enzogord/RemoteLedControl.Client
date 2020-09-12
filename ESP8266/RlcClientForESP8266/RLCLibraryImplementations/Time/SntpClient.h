#pragma once

#include <SNTP/ISntpClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

class SntpClient : public ISntpClient
{
private:
	IPAddress& ipAddress;
	uint16_t port;

public:
	SntpClient(IPAddress& ipAddressRef, uint16_t port);

	// Inherited via ISntpClient
	virtual bool SendSntpRequest(SntpPackage& sntpPackage, int responseDelayMs = 1000);

};
