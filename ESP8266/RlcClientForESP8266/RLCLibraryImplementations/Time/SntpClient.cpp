#include "SntpClient.h"
#include <TimeNow.h>

SntpClient::SntpClient(WiFiUDP& udpClientRef, IPAddress& ipAddressRef, uint16_t port)
	: udpClient(udpClientRef), ipAddress(ipAddressRef)
{
	SntpClient::port = port;
}

bool SntpClient::SendSntpRequest(SntpPackage& sntpPackage, int responseDelayMs)
{
	udpClient.flush();
	udpClient.setTimeout(responseDelayMs);
	udpClient.beginPacket(ipAddress, port);
	sntpPackage.SetSendingTime(TimeNow());
	udpClient.write(sntpPackage.GetBytes(), SNTP_PACKET_SIZE);
	udpClient.endPacket();
	

	uint32_t beginWait = millis();
	while(millis() - beginWait < responseDelayMs) {
		int size = udpClient.parsePacket();
		if(size >= SNTP_PACKET_SIZE) {
			Time receiveTime = TimeNow();
			uint8_t* packetBuffer = new uint8_t[SNTP_PACKET_SIZE];
			udpClient.read(packetBuffer, SNTP_PACKET_SIZE);
			
			sntpPackage.SetServerReceiveTime(packetBuffer);
			sntpPackage.SetServerSendingTime(packetBuffer);
			sntpPackage.SetReceiveTime(receiveTime);
			return true;
		}
	}

	return false;
}
