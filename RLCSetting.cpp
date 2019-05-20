#include "RLCSetting.h"



void RLCSetting::SaveSetting(String name, String value)
{
	if (name == "SSID")
	{
		SSID = value;
		return;
	}
	if (name == "Password")
	{
		Password = value;
		return;
	}
	if (name == "PlateNumber")
	{
		PlateNumber = atoi(value.c_str());
		return;
	}
	if (name == "ProjectKey")
	{
		ProjectKey = strtoul(value.c_str(), NULL, 10);
		return;
	}
	if (name == "UDPPackageSize")
	{
		UDPPackageSize = atoi(value.c_str());
		return;
	}
	if (name == "UDPPort")
	{
		UDPPort = atoi(value.c_str());
		return;
	}
	if (name == "RefreshInterval")
	{
		RefreshInterval = atoi(value.c_str());
		return;
	}
	if (name == "ChannelCount")
	{
		ChannelCount = atoi(value.c_str());
		return;
	}
	if (name == "Pins")
	{
		Serial.println("DEBUG:");
		String source = value + ',';
		Serial.println(source);
		int Count = 0;
		for (size_t i = 0; i < source.length(); i++)
		{
			if (source[i] == ',')
			{
				Count += 1;
			}
		}
		PinsCount = Count;
		Pins = new ClientPin[Count];
		String buffer = "";
		String pinBuffer = "";
		String ledCountBuffer = "";
		int pinIndex = 0;
		for (size_t i = 0; i < source.length(); i++)
		{
			if (source[i] == '-')
			{
				pinBuffer = buffer;
				buffer = "";
			}
			else if (source[i] == ',')
			{
				ledCountBuffer = buffer;
				buffer = "";
			}
			else
			{
				buffer += source[i];
			}
			if ((pinBuffer != "") && (ledCountBuffer != ""))
			{
				char pinType = pinBuffer[0];
				String pinNumberBuffer = pinBuffer.substring(1);

				Serial.print("Pin index: "); Serial.println(pinIndex);
				Serial.print("Pin type: "); Serial.println(pinType);
				Serial.print("Pin number: "); Serial.println(pinNumberBuffer);
				
				Serial.print("Pin LedCount: "); Serial.println(ledCountBuffer);
				switch(pinType)
				{
				case 'S': 
					Pins[pinIndex].Type = PinType::SPI;
					break;
				case 'P': 
					Pins[pinIndex].Type = PinType::PWM;
					break;
				default:
					Serial.print("Pin type not parsed");
					Pins[pinIndex].Type = PinType::SPI;
					break;
				}
				Pins[pinIndex].Number = atoi(pinNumberBuffer.c_str());
				Pins[pinIndex].LedCount = atoi(ledCountBuffer.c_str());
				if(Pins[pinIndex].Type == PinType::SPI)
				{
					SPILedCount += Pins[pinIndex].LedCount;
				}
				if(Pins[pinIndex].Type == PinType::PWM)
				{
					PWMChannelCount++;
				}
				pinNumberBuffer = "";
				ledCountBuffer = "";
				pinIndex += 1;
			}

		}
		Serial.println("DEBUG END!");
		return;
	}
	if(name == "DefaultLightMode")
	{
		if(value == "On")
		{
			DefaultLightOn = true;
			return;
		}
		DefaultLightOn = false;
	}
	if(name == "SPILedGlobalBrightness")
	{
		int brightness = atoi(value.c_str());
		if(brightness >= 255)
		{
			SPILedGlobalBrightness = 255;
			return;
		}
		else if(brightness <= 0)
		{
			SPILedGlobalBrightness = 0;
			return;
		}
		SPILedGlobalBrightness = brightness;
	}
}

void RLCSetting::ReadSetting(File SettingFile)
{
	String Buffer = "";
	String BName = "";
	String BValue = "";
	byte c;
	while (SettingFile.available()) {
		c = SettingFile.read();
		switch (c)
		{
		case 61:
			BName = Buffer;
			Buffer = "";
			break;
		case 59:
			BValue = Buffer;
			Buffer = "";
			SaveSetting(BName, BValue);
			break;
		case 10:
			break;
		case 13:
			break;
		default:
			Buffer += char(c);
		}
	}
}

RLCSetting::RLCSetting()
{
}


RLCSetting::~RLCSetting()
{
}
