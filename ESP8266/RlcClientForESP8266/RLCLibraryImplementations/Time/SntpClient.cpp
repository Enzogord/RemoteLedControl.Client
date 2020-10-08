#include "SntpClient.h"
#include <TimeNow.h>

SntpClient::SntpClient(IPAddress& ipAddressRef, uint16_t port)
	: ipAddress(ipAddressRef)
{
	SntpClient::port = port;
}

bool SntpClient::SendSntpRequest(SntpPackage* sntpPackage, int responseDelayMs)
{
	udp.begin(port);
	udp.setTimeout(responseDelayMs);

	udp.beginPacket(ipAddress, port);
	sntpPackage->SetSendingTime(TimeNow());
	udp.write(sntpPackage->GetBytes(), SNTP_PACKET_SIZE);
	udp.endPacket();


	uint32_t beginWait = millis();
	bool result = false;
	while (millis() - beginWait < responseDelayMs) {
		int size = udp.parsePacket();
		if (size >= SNTP_PACKET_SIZE) {
			Time receiveTime = TimeNow();
			memset(packetBuffer, 0, SNTP_PACKET_SIZE);
			udp.readBytes(packetBuffer, SNTP_PACKET_SIZE);
			sntpPackage->SetServerReceiveTime(packetBuffer);
			sntpPackage->SetServerSendingTime(packetBuffer);
			sntpPackage->SetReceiveTime(receiveTime);
			result = true;
			break;
		}
	}
	udp.stop();
	return result;
}
