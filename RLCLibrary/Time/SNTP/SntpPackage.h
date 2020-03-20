#pragma once

#include "../../RLCLibrary/Infrastructure/GlobalTypes.h"
#include "../Time.h"
#include "SntpFunctions.h"

const int SNTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
const int SENDING_INDEX = 24;
const int SERVER_RECEIVE_INDEX = 32;
const int SERVER_SEND_INDEX = 40;

class SntpPackage
{
private:
	uint8_t* packetBuffer = new uint8_t[SNTP_PACKET_SIZE];
	Time receiveTime;

public:
	Time GetSendingTime();
	void SetSendingTime(Time sendingTime);

	Time GetServerReceiveTime();
	void SetServerReceiveTime(uint8_t* bytes);

	Time GetServerSendingTime();
	void SetServerSendingTime(uint8_t* bytes);

	Time GetReceiveTime();
	void SetReceiveTime(Time receiveTime);

	const uint8_t* GetBytes();

	SntpPackage();
	~SntpPackage();
};