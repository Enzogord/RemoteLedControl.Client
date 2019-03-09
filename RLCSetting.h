#pragma once
#include <SD.h>

struct ClientPin
{
	byte Number;
	uint16 LedCount;
};

class RLCSetting
{
private:
	void SaveSetting(String name, String value);
public:
	void ReadSetting(File SettingFile);
	String  SSID;
	String  Password;
	byte PlateNumber;
	unsigned long ProjectKey;
	unsigned int UDPPackageSize;
	unsigned int UDPPort;
	byte RefreshInterval;
	unsigned int LEDCount;
	byte ChannelCount;
	ClientPin* Pins;
	byte PinsCount;
	RLCSetting();
	~RLCSetting();
};

