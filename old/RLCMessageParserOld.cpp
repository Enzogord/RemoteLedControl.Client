#include "RLCMessageParserOld.h"
#include "HardwareSerial.h"



RLCMessageParserOld::RLCMessageParserOld()
{
}

RLCMessageParserOld::RLCMessageParserOld(RLCSetting* settings_ptr, uint32* currenctTime_ptr, IPAddress* serverIP_ptr, WiFiUDP* udp_ptr)
{
	Setting_ptr = settings_ptr;
	CurrenctTime_ptr = currenctTime_ptr;
	ServerIP_ptr = serverIP_ptr;
	Udp_ptr = udp_ptr;
}


RLCMessageParserOld::~RLCMessageParserOld()
{
}

byte RLCMessageParserOld::Parse()
{
	byte result = 0;
	byte ptype;
	byte PlateCount;
	byte PlateNumber;
	byte Command;	
	
	unsigned int ContentLength;
	unsigned int psize = (*Udp_ptr).parsePacket();
	if (psize == (*Setting_ptr).UDPPackageSize) {
		byte sourceType = (*Udp_ptr).read();
		if (sourceType != 0)
		{
			return 0;
		}
		unsigned long Key = ((*Udp_ptr).read() << 24) + ((*Udp_ptr).read() << 16) + ((*Udp_ptr).read() << 8) + ((*Udp_ptr).read());
		if (Key == (*Setting_ptr).ProjectKey)
		{
			ptype = (*Udp_ptr).read();
			Serial.print("Receiving package: "); Serial.println(ptype);
			switch (ptype)
			{
			case 1:
				ContentLength = ((*Udp_ptr).read() << 8) + ((*Udp_ptr).read());
				PlateCount = (*Udp_ptr).read();
				if (PlateCount == 0)
				{
					result = 1;
					break;
				}
				for (byte i = 1; i <= PlateCount; i++)
				{
					PlateNumber = (*Udp_ptr).read();
					if (PlateNumber == (*Setting_ptr).PlateNumber)
					{
						result = 1;
						break;
					}
				}
			case 2:
				ContentLength = ((*Udp_ptr).read() << 8) + ((*Udp_ptr).read());
				PlateCount = (*Udp_ptr).read();
				if (PlateCount == 0)
				{
					result = 2;
					break;
				}
				for (byte i = 1; i <= PlateCount; i++)
				{
					PlateNumber = (*Udp_ptr).read();
					if (PlateNumber == (*Setting_ptr).PlateNumber)
					{
						result = 2;
						break;
					}
				}
				break;
			case 5:
				ContentLength = ((*Udp_ptr).read() << 8) + ((*Udp_ptr).read());
				(*ServerIP_ptr).operator[](0) = (*Udp_ptr).read();
				(*ServerIP_ptr).operator[](1) = (*Udp_ptr).read();
				(*ServerIP_ptr).operator[](2) = (*Udp_ptr).read();
				(*ServerIP_ptr).operator[](3) = (*Udp_ptr).read();
				result = 5;
				Serial.println((*ServerIP_ptr).toString());
				break;
			case 6:
				result = 6;
				break;
			case 7:
				ContentLength = ((*Udp_ptr).read() << 8) + ((*Udp_ptr).read());
				*CurrenctTime_ptr = ((*Udp_ptr).read() << 24) + ((*Udp_ptr).read() << 16) + ((*Udp_ptr).read() << 8) + ((*Udp_ptr).read());
				*CurrenctTime_ptr *= ((*Setting_ptr).LEDCount * 3);
				result = 7;
				break;
			case 12:
				Serial.print("Package 12 recieved, "); Serial.print("Client: ");
				ContentLength = ((*Udp_ptr).read() << 8) + ((*Udp_ptr).read());
				PlateNumber = (*Udp_ptr).read();
				Serial.println(PlateNumber);
				if (PlateNumber == (*Setting_ptr).PlateNumber)
				{
					*CurrenctTime_ptr = ((*Udp_ptr).read() << 24) + ((*Udp_ptr).read() << 16) + ((*Udp_ptr).read() << 8) + ((*Udp_ptr).read());
					*CurrenctTime_ptr *= ((*Setting_ptr).LEDCount * 3);
					result = 7;
				}
				break;
			default:
				break;
			}
		}
	}
	return result;
}
