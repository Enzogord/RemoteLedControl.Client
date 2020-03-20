#include "SntpPackage.h"

SntpPackage::SntpPackage()
{
	for (byte i = 0; i < SNTP_PACKET_SIZE; i++)
	{
		packetBuffer[i] = 0;
	}

	// Initialize values needed to form NTP request
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
							 // 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;
}

SntpPackage::~SntpPackage()
{
	delete(packetBuffer);
}

Time SntpPackage::GetSendingTime()
{
	return GetTimeFromArray(packetBuffer, SENDING_INDEX);
}

void SntpPackage::SetSendingTime(Time sendingTime)
{
	SetTimeToArray(sendingTime, packetBuffer, SENDING_INDEX);
}

Time SntpPackage::GetServerReceiveTime()
{
	return GetTimeFromArray(packetBuffer, SERVER_RECEIVE_INDEX);
}

void SntpPackage::SetServerReceiveTime(uint8_t* bytes)
{
	for(int i = SERVER_RECEIVE_INDEX; i < (SERVER_RECEIVE_INDEX + 8); i++) {
		packetBuffer[i] = bytes[i];
	}
}

Time SntpPackage::GetServerSendingTime()
{
	return GetTimeFromArray(packetBuffer, SERVER_SEND_INDEX);
}

void SntpPackage::SetServerSendingTime(uint8_t* bytes)
{
	for(int i = SERVER_RECEIVE_INDEX; i < (SERVER_SEND_INDEX + 8); i++) {
		packetBuffer[i] = bytes[i];
	}
}

Time SntpPackage::GetReceiveTime()
{
	return SntpPackage::receiveTime;
}

void SntpPackage::SetReceiveTime(Time receiveTime)
{
	SntpPackage::receiveTime = receiveTime;
}

const uint8_t* SntpPackage::GetBytes()
{
	return packetBuffer;
}
