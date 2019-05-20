#pragma once
#include <SD.h>

enum class PinType {
	SPI,
	PWM
};

inline const char* ToString(PinType pinType)
{
	switch(pinType)
	{
	case PinType::SPI:   return "SPI";
	case PinType::PWM:   return "PWM";
	default:      return "NotSet";
	}
}

struct ClientPin
{
	PinType Type;
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
	unsigned short PlateNumber;
	unsigned long ProjectKey;
	unsigned int UDPPackageSize;
	unsigned int UDPPort;
	byte RefreshInterval;
	unsigned int SPILedCount;
	unsigned int PWMChannelCount;
	byte ChannelCount;
	bool DefaultLightOn;
	uint8_t SPILedGlobalBrightness;
	ClientPin* Pins;
	byte PinsCount;
	RLCSetting();
	~RLCSetting();
};

