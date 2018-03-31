#pragma once

struct ClientPin
{
	byte Number;
	uint16 LedCount;
};

class SovaSettingLocal
{
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

	SovaSettingLocal()
	{
	}

	virtual ~SovaSettingLocal()
	{
	}

private:
	void SaveSetting(String name, String value);
};



void SovaSettingLocal::ReadSetting(File SettingFile)
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

void SovaSettingLocal::SaveSetting(String name, String value)
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
	if (name == "LEDCount")
	{
		LEDCount = atoi(value.c_str());
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
		String PinNumberBuffer = "";
		String LEDCountBuffer = "";
		int PinIndex = 0;
		for (size_t i = 0; i < source.length(); i++)
		{
			if (source[i] == '-')
			{
				PinNumberBuffer = buffer;
				buffer = "";
			}
			else if (source[i] == ',')
			{
				LEDCountBuffer = buffer;
				buffer = "";
			}
			else
			{
				buffer += source[i];
			}
			if ((PinNumberBuffer != "") && (LEDCountBuffer != ""))
			{
				Serial.print("Pin index: "); Serial.println(PinIndex);
				Serial.print("Pin Number: "); Serial.println(PinNumberBuffer);
				Serial.print("Pin LedCount: "); Serial.println(LEDCountBuffer);
				Pins[PinIndex].Number = atoi(PinNumberBuffer.c_str());
				Pins[PinIndex].LedCount = atoi(LEDCountBuffer.c_str());
				PinNumberBuffer = "";
				LEDCountBuffer = "";
				PinIndex += 1;
			}
						
		}
		Serial.println("DEBUG END!");
		
		//ChannelCount = atoi(value.c_str());
		return;
	}
}

