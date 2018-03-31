#include <SD.h>
#include <Hash.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "FastLED.h"
#include "SovaSettingLocal.h"

#define chipSelect 15

CRGB *LedArray;
File FileCyclogram;
File SettingFile;
SovaSettingLocal SovaSet;
WiFiUDP Udp;
IPAddress ServerIP;
String Cyclogramm = "Data.cyc";
byte ClientState;

uint32 CurrentTime = 0; // Момент времени с которого запуститься циклограмма
unsigned long MainLoopTime1, MainLoopTime2;  // Для подсчета временного интервала работы программы на главном цикле для отправки пакета с номером клиента на сервер с задержкой
unsigned long LedTime1, LedTime2;  // Для подсчета временного интервала работы программы на цикле включения светодиодов для сравнения с интервалом в циклограмме (50мс)
unsigned long PauseTime1, PauseTime2;  // Для подсчета временного интервала работы программы на цикле паузы
int ColorIndex; // Индекс в массиве LedArray, от 0 до количества светодиодов -1
byte PackageParseResult; // Результат парсинга полученного пакета (по кодам пакетов в протоколе передачи)

void setup()
{
	Serial.begin(115200);
	WiFi.softAPdisconnect(true);  // Отключение точки доступа

LabelRestartSD:  // Старт/рестарт инициализации SD карты
	Serial.println(); Serial.println();
	Serial.print("Initializing SD card");
	if (!SD.begin(chipSelect)) {
		Serial.println(" failed!");
		delay(50);
		goto LabelRestartSD;
	}
	else {
		Serial.println(" successful!");
	}

	SettingFile = SD.open("set.txt", FILE_READ);
	SovaSet.ReadSetting(SettingFile);
	Serial.println();
	Serial.println("-----Project parameters-----");
	Serial.print("SSID: \"");
	Serial.print(SovaSet.SSID);
	Serial.println("\"");
	Serial.print("Password: \"");
	Serial.print(SovaSet.Password);
	Serial.println("\"");
	Serial.print("PlateNumber: ");
	Serial.println(SovaSet.PlateNumber);
	Serial.print("ProjectKey: ");
	Serial.println(SovaSet.ProjectKey);
	Serial.print("UDPPackageSize: ");
	Serial.println(SovaSet.UDPPackageSize);
	Serial.print("UDPPort: ");
	Serial.println(SovaSet.UDPPort);
	Serial.print("LEDCount: ");
	Serial.println(SovaSet.LEDCount);
	Serial.print("Pins count: ");
	Serial.println(SovaSet.PinsCount);
	for (size_t i = 0; i < SovaSet.PinsCount; i++)
	{
		Serial.print("Pin: ");
		Serial.print(SovaSet.Pins[i].Number);
		Serial.print("-");
		Serial.println(SovaSet.Pins[i].LedCount);
	}
	FastLEDInitialization();
	Serial.print("----------------------------");
	Serial.println();

	// Подключение к WiFi с переподключением
	//WiFiConnect();
	WiFiNewConnectLoop();

	Udp.begin(SovaSet.UDPPort);
	while (ServerIP == IPAddress(0, 0, 0, 0))
	{
		Serial.println("Wait serverIP");
		SendPackageBroadcast(8);
		delay(100);
		ParsePackage();
	}
	FastLED.clear(true);
	MainLoopTime1 = millis();
}

void loop(void) {
LabelStop:
	MainLoopTime2 = millis();
	if ((MainLoopTime2 - MainLoopTime1) > 50)
	{
		ClientState = 1;
		SendPackage(3);
		FastLED.clear(true);
		MainLoopTime1 = millis();
	}
	else if ((MainLoopTime2 - MainLoopTime1) > 10)
	{
		PackageParseResult = ParsePackage();
		if (PackageParseResult == 1 || PackageParseResult == 7) {
		LabelStart:
			FileCyclogram = SD.open(Cyclogramm);
			if (FileCyclogram) {
				Serial.print("Current cyclogramm: "); Serial.println(Cyclogramm);
				ColorIndex = 0;
				LedTime1 = millis();
			labelSetTime:
				if (CurrentTime > 0)
				{
					FileCyclogram.seek(CurrentTime);
					CurrentTime = 0;
				}
				while (FileCyclogram.available()) {
					LedArray[ColorIndex].r = FileCyclogram.read();
					LedArray[ColorIndex].g = FileCyclogram.read();
					LedArray[ColorIndex].b = FileCyclogram.read();
					if (ColorIndex == SovaSet.LEDCount - 1) {
						FastLED.show();
					label1:
						LedTime2 = millis();
						if ((LedTime2 - LedTime1) >= SovaSet.RefreshInterval)
						{
							ColorIndex = 0;
							LedTime1 = millis();
							continue;
						}
						else
						{
							ClientState = 2;
							SendPackage(3);
							PackageParseResult = ParsePackage();
							switch (PackageParseResult) {
							case 1:
								goto label1;
							case 2:
								goto LabelStop;
							case 6:
								PauseTime1 = millis();
								goto labelPause;
							case 7:
								goto labelSetTime;
							default:
								goto label1;
							}
						labelPause:
							PauseTime2 = millis();
							if ((PauseTime2 - PauseTime1) >= 50)
							{
								ClientState = 3;
								SendPackage(3);
								PauseTime1 = millis();
								PackageParseResult = ParsePackage();
								switch (PackageParseResult)
								{
								case 1:
									goto label1;
								case 2:
									goto LabelStop;
								case 6:
									goto labelPause;
								case 7:
									goto labelSetTime;
								default:
									goto labelPause;
								}
							}
							else
							{
								goto labelPause;
							}
						}
					}
					ColorIndex++;
				}
			}
		}
	}
}

void WiFiConnect() {
	unsigned long ConnectionTime = 0;
	if (WiFi.SSID() == SovaSet.SSID)
	{
		Serial.print("Reconnecting to WiFi: "); Serial.println(WiFi.SSID());
		WiFi.reconnect();
		FastLED.showColor(CRGB(255, 70, 0), 100);
		while (WiFi.status() != WL_CONNECTED) {
			delay(10);
			if (ConnectionTime > 10000)
			{
				break;
			}
			ConnectionTime += 10;
		}
		FastLED.clear(true);
	}
	ConnectionTime = 0;
	if (WiFi.status() != WL_CONNECTED)
	{
		Serial.print("New connecting to WiFi: "); Serial.println(SovaSet.SSID);
		WiFi.disconnect();
		WiFi.begin(SovaSet.SSID.c_str(), SovaSet.Password.c_str());
		FastLED.showColor(CRGB::Gold, 100);
		while (WiFi.status() != WL_CONNECTED) {
			delay(10);
		}
		FastLED.clear(true);
	}
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.print("Local IP: "); Serial.println(WiFi.localIP());
}

void WiFiNewConnectLoop() {
	unsigned long ConnectionTime = 0;
	ConnectionTime = 0;
	Serial.print("Try connecting to WiFi: "); Serial.println(SovaSet.SSID);
LabelConnection:
	WiFi.disconnect();
	WiFi.begin(SovaSet.SSID.c_str(), SovaSet.Password.c_str());
	FastLED.showColor(CRGB::Gold, 100);
	while (WiFi.status() != WL_CONNECTED) {
		delay(10);
		if (ConnectionTime > 10000)
		{
			goto LabelConnection;
		}
		ConnectionTime += 10;
	}
	FastLED.clear(true);
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.print("Local IP: "); Serial.println(WiFi.localIP());
}

void FastLEDInitialization() {
	LedArray = new CRGB[SovaSet.LEDCount];
	int startLED = 0;
	for (size_t i = 0; i < SovaSet.PinsCount; i++)
	{
		switch (SovaSet.Pins[i].Number)
		{
		case 0:
			FastLED.addLeds<WS2812B, 0, GRB>(LedArray, startLED, SovaSet.Pins[i].LedCount);
			Serial.print("Select Pin 0, ");
			break;
		case 2:
			FastLED.addLeds<WS2812B, 2, GRB>(LedArray, startLED, SovaSet.Pins[i].LedCount);
			Serial.print("Select Pin 2, ");
			break;
		case 4:
			FastLED.addLeds<WS2812B, 4, GRB>(LedArray, startLED, SovaSet.Pins[i].LedCount);
			Serial.print("Select Pin 4, ");
			break;
		case 5:
			FastLED.addLeds<WS2812B, 5, GRB>(LedArray, startLED, SovaSet.Pins[i].LedCount);
			Serial.print("Select Pin 5, ");
			break;
		default:
			break;
		}
		Serial.print("Pin: "); Serial.print(SovaSet.Pins[i].Number);
		Serial.print(", Start LED: "); Serial.print(startLED);
		Serial.print(", LED count: "); Serial.println(SovaSet.Pins[i].LedCount);
		startLED += SovaSet.Pins[i].LedCount;
	}
}

void SendPackage(uint8 PackageType)
{
	uint32 f = SovaSet.ProjectKey;
	uint16 ContLength;
	uint16 PackageLength = SovaSet.UDPPackageSize;
	uint8 *PackageBody = new uint8[PackageLength];
	PackageBody[0] = (f >> 24) & 0xFF;
	PackageBody[1] = (f >> 16) & 0xFF;
	PackageBody[2] = (f >> 8) & 0xFF;
	PackageBody[3] = (f >> 0) & 0xFF;
	PackageBody[4] = PackageType;
	switch (PackageType)
	{
	case 3:
		ContLength = 2;
		PackageBody[5] = (ContLength >> 8) & 0xFF;
		PackageBody[6] = (ContLength >> 0) & 0xFF;
		PackageBody[7] = SovaSet.PlateNumber;
		PackageBody[8] = ClientState;
		break;
	default:
		break;
	}
	for (size_t i = ContLength + 7; i < PackageLength; i++)
	{
		PackageBody[i] = 0;
	}

	Udp.beginPacket(ServerIP, SovaSet.UDPPort);
	Udp.write(PackageBody, PackageLength);
	Udp.endPacket();
	delete[] PackageBody;
}
void SendPackageBroadcast(uint8 PackageType)
{
	IPAddress ipMulti(255, 255, 255, 255);
	uint32 f = SovaSet.ProjectKey;
	uint16 ContLength;
	uint16 PackageLength = SovaSet.UDPPackageSize;
	uint8 *PackageBody = new uint8[PackageLength];
	PackageBody[0] = (f >> 24) & 0xFF;
	PackageBody[1] = (f >> 16) & 0xFF;
	PackageBody[2] = (f >> 8) & 0xFF;
	PackageBody[3] = (f >> 0) & 0xFF;
	PackageBody[4] = PackageType;
	switch (PackageType)
	{
	case 8:
		for (size_t i = 5; i < PackageLength; i++)
		{
			PackageBody[i] = 0;
		}
		break;
	default:
		break;
	}
	Udp.beginPacketMulticast(ipMulti, SovaSet.UDPPort, WiFi.localIP());
	Udp.write(PackageBody, PackageLength);
	Udp.endPacket();
	delete[] PackageBody;
}
byte ParsePackage() {
	byte result = 0;
	byte ptype;
	byte PlateCount;
	byte PlateNumber;
	byte Command;

	unsigned int ContentLength;
	unsigned int psize = Udp.parsePacket();
	if (psize == SovaSet.UDPPackageSize) {
		unsigned long Key = (Udp.read() << 24) + (Udp.read() << 16) + (Udp.read() << 8) + (Udp.read());
		if (Key == SovaSet.ProjectKey)
		{
			ptype = Udp.read();
			switch (ptype)
			{
			case 1:
				ContentLength = (Udp.read() << 8) + (Udp.read());
				PlateCount = Udp.read();
				if (PlateCount == 0)
				{
					result = 1;
					break;
				}
				for (byte i = 1; i <= PlateCount; i++)
				{
					PlateNumber = Udp.read();
					if (PlateNumber == SovaSet.PlateNumber)
					{
						result = 1;
						break;
					}
				}
			case 2:
				ContentLength = (Udp.read() << 8) + (Udp.read());
				PlateCount = Udp.read();
				if (PlateCount == 0)
				{
					result = 2;
					break;
				}
				for (byte i = 1; i <= PlateCount; i++)
				{
					PlateNumber = Udp.read();
					if (PlateNumber == SovaSet.PlateNumber)
					{
						result = 2;
						break;
					}
				}
				break;
			case 5:
				ContentLength = (Udp.read() << 8) + (Udp.read());
				ServerIP.operator[](0) = Udp.read();
				ServerIP.operator[](1) = Udp.read();
				ServerIP.operator[](2) = Udp.read();
				ServerIP.operator[](3) = Udp.read();
				result = 5;
				Serial.println(ServerIP.toString());
				break;
			case 6:
				result = 6;
				break;
			case 7:
				ContentLength = (Udp.read() << 8) + (Udp.read());
				CurrentTime = (Udp.read() << 24) + (Udp.read() << 16) + (Udp.read() << 8) + (Udp.read());
				CurrentTime *= (SovaSet.LEDCount * 3);
				result = 7;
				break;
			case 12:
				Serial.print("Package 12 recieved, "); Serial.print("Client: ");
				ContentLength = (Udp.read() << 8) + (Udp.read());
				PlateNumber = Udp.read();
				Serial.println(PlateNumber);
				if (PlateNumber == SovaSet.PlateNumber)
				{
					CurrentTime = (Udp.read() << 24) + (Udp.read() << 16) + (Udp.read() << 8) + (Udp.read());
					CurrentTime *= (SovaSet.LEDCount * 3);
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