#include <SD.h>

class SovaSetting
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
private:
	void SaveSetting(String name, String value);

};
