
#include "SovaSetting.h"

void SovaSetting::ReadSetting(File SettingFile)
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

void SovaSetting::SaveSetting(String name, String value)
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
		ProjectKey = atoi(value.c_str());
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
}