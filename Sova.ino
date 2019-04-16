
#include <SD.h>
#include <Hash.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "FastLED.h"
#include "RLCSetting.h"
#include "RLCMessageParserOld.h"
#include "RLCMessage/RLCMessage.h"
#include "RLCMessage/RLCMessageParser.h"
#include "RLCMessage/RLCEnums.h"
#include "RLCMessage/RLCMessageFactory.h"

#define chipSelect 15
//таймаут ожидания при запросе по мультикасту
#define SERVER_IP_TIMEOUT 2000UL


CRGB *ledArray;
File cyclogrammFile;
File settingFile;
IPAddress broadcastAddress;
IPAddress serverIP = IPAddress(0, 0, 0, 0);
String cyclogrammFileName = "Data.cyc";
uint8 *rlcMessageBuffer = new uint8[RLC_MESSAGE_LENGTH];
WiFiClient tcpClient;
WiFiUDP udp;

ClientStateEnum clientState;
RLCSetting rlcSettings;
RLCMessageParser messageParser;
RLCMessageFactory messageFactory;

uint32 CurrentTime = 0; // Момент времени с которого запуститься циклограмма
unsigned long MainLoopTime1, MainLoopTime2;  // Для подсчета временного интервала работы программы на главном цикле для отправки пакета с номером клиента на сервер с задержкой
unsigned long LedTime1, LedTime2;  // Для подсчета временного интервала работы программы на цикле включения светодиодов для сравнения с интервалом в циклограмме (50мс)
unsigned long PauseTime1, PauseTime2;  // Для подсчета временного интервала работы программы на цикле паузы
int ColorIndex; // Индекс в массиве LedArray, от 0 до количества светодиодов -1
byte PackageParseResult; // Результат парсинга полученного пакета (по кодам пакетов в протоколе передачи)

void Initializations() 
{
	Serial.begin(115200);
	clientState = ClientStateEnum::Stoped;
	// Отключение точки доступа
	WiFi.softAPdisconnect(true);

	InitializeSDCard();
}

void InitializeSDCard()
{
	Serial.print("Initializing SD card");
	boolean sdActive = false;
	unsigned long sdTime = millis();
	while(!sdActive)
	{
		if((millis() - sdTime) >= 50)
		{
			sdTime = millis();
			sdActive = SD.begin(chipSelect);
		}
	}

	Serial.println(" successful!");
}

void WaitingServerIPAddress() 
{
	Serial.println("Waiting Server IP address");
	RLCMessage requestServerIPMessage = messageFactory.RequestServerIP(clientState);
	uint8_t* requestBytes = requestServerIPMessage.GetBytes();
	do {
		Serial.print("Request Server IP address. ");
		udp.begin(rlcSettings.UDPPort);
		udp.beginPacket(broadcastAddress, rlcSettings.UDPPort);
		udp.write(requestBytes, RLC_MESSAGE_LENGTH);
		udp.endPacket();
		unsigned long timeCheckpoint = millis();
		while(serverIP == IPAddress(0, 0, 0, 0) && (millis() - timeCheckpoint) < SERVER_IP_TIMEOUT)
		{
			if(udp.available()) {
				memset(rlcMessageBuffer, 0, RLC_MESSAGE_LENGTH);
				udp.readBytes(rlcMessageBuffer, RLC_MESSAGE_LENGTH);
				RLCMessage response = messageParser.Parse(rlcMessageBuffer);
				if(response.IsInitialized && response.MessageType == MessageTypeEnum::SendServerIP && response.IP != IPAddress(0, 0, 0, 0)) {
					serverIP = response.IP;
				}
			}
		}
		udp.stopAll();
		if(serverIP == IPAddress(0, 0, 0, 0))
		{
			Serial.println("Failed to receive server IP address, retry.");
		}
	} while(serverIP == IPAddress(0, 0, 0, 0));
	Serial.print("Received server IP: "); Serial.println(serverIP);
}

void ConnectToRLCServer(IPAddress &ipAddress, uint16_t port)
{
	unsigned long onStartMillis = millis();
	Serial.print("Trying connect to "); Serial.print(ipAddress); Serial.print(":"); Serial.println(port);
	//пересоздаем клиент, потому что при разрыве связи не может заного подключиться
	tcpClient = WiFiClient();
	Serial.print("Connecting");
	FastLED.showColor(CRGB::BlueViolet, 100);
	int tcpConnected = -1;
	tcpConnected = tcpClient.connect(ipAddress, port);
	while(!tcpConnected)
	{		
		if((millis() - onStartMillis) >= 500)
		{
			onStartMillis = millis();
			tcpConnected = tcpClient.connect(ipAddress, port);
			Serial.print(".");
		}
	}
	FastLED.clear(true);
	Serial.println(" Connected.");
	tcpClient.setTimeout(0);
	tcpClient.keepAlive(2, 1);
	//tcpClient.write("Starter message");
}

void ReadTCPConnection() 
{
	uint8_t packetBuffer[1024];

	while (tcpClient.available()) {
		Serial.print("[TCP] Received data form server: ");
		tcpClient.readBytes(packetBuffer, 1024);
		RLCMessage message = messageParser.Parse(packetBuffer);
		if(message.IsInitialized && message.Key == rlcSettings.ProjectKey && message.SourceType == SourceTypeEnum::Server)
		{
			OnReceiveMessage(message);
		}
	}
}

void OnReceiveMessage(RLCMessage &message)
{
	switch(message.MessageType)
	{
	case MessageTypeEnum::Play:
		Serial.println("Receive Play message");
		break;
	case MessageTypeEnum::Pause:
		Serial.println("Receive Pause message");
		break;
	case MessageTypeEnum::Stop:
		Serial.println("Receive Stop message");
		break;
	case MessageTypeEnum::PlayFrom:
		Serial.println("Receive PlayFrom message");
		break;
	case MessageTypeEnum::NotSet:
	default:
		break;
	}
}

void setup()
{
	Initializations();

	settingFile = SD.open("set.txt", FILE_READ);
	rlcSettings.ReadSetting(settingFile);
	Serial.println();
	Serial.println("-----Project parameters-----");
	Serial.print("SSID: \"");
	Serial.print(rlcSettings.SSID);
	Serial.println("\"");
	Serial.print("Password: \"");
	Serial.print(rlcSettings.Password);
	Serial.println("\"");
	Serial.print("PlateNumber: ");
	Serial.println(rlcSettings.PlateNumber);
	Serial.print("ProjectKey: ");
	Serial.println(rlcSettings.ProjectKey);
	Serial.print("UDPPackageSize: ");
	Serial.println(rlcSettings.UDPPackageSize);
	Serial.print("UDPPort: ");
	Serial.println(rlcSettings.UDPPort);
	Serial.print("LEDCount: ");
	Serial.println(rlcSettings.LEDCount);
	Serial.print("Pins count: ");
	Serial.println(rlcSettings.PinsCount);
	for (size_t i = 0; i < rlcSettings.PinsCount; i++)
	{
		Serial.print("Pin: ");
		Serial.print(rlcSettings.Pins[i].Number);
		Serial.print("-");
		Serial.println(rlcSettings.Pins[i].LedCount);
	}

	FastLEDInitialization();
	Serial.print("----------------------------");
	Serial.println();

	messageFactory = RLCMessageFactory(rlcSettings.ProjectKey, rlcSettings.PlateNumber);
	//	messageParser = RLCMessageParserOld(&rlcSettings, &CurrentTime, &serverIP, &udp);
	
	// Подключение к WiFi с переподключением
	WiFiConnect();
	WaitingServerIPAddress();
	ConnectToRLCServer(serverIP, rlcSettings.UDPPort);
	Serial.println("After trying tcp connection");
	FastLED.clear(true);
	//MainLoopTime1 = millis();
}

void loop(void) {
	ReadTCPConnection();



	/*
LabelStop:
	MainLoopTime2 = millis();
	if ((MainLoopTime2 - MainLoopTime1) > 50)
	{
		clientState = 1;
		SendPackage(3);
		FastLED.clear(true);
		MainLoopTime1 = millis();
	}
	else if ((MainLoopTime2 - MainLoopTime1) > 10)
	{
		PackageParseResult = messageParser.Parse();
		if (PackageParseResult > 0) { Serial.print("PackageResult: "); Serial.println(PackageParseResult); }
		if (PackageParseResult == 1 || PackageParseResult == 7) {
		LabelStart:
			cyclogrammFile = SD.open(cyclogrammFileName);
			if (cyclogrammFile) {
				Serial.print("Current cyclogramm: "); Serial.println(cyclogrammFileName);
				ColorIndex = 0;
				LedTime1 = millis();
			labelSetTime:
				if (CurrentTime > 0)
				{
					cyclogrammFile.seek(CurrentTime);
					CurrentTime = 0;
				}
				while (cyclogrammFile.available()) {
					ledArray[ColorIndex].r = cyclogrammFile.read();
					ledArray[ColorIndex].g = cyclogrammFile.read();
					ledArray[ColorIndex].b = cyclogrammFile.read();
					if (ColorIndex == rlcSettings.LEDCount - 1) {
						FastLED.show();
					label1:
						LedTime2 = millis();
						if ((LedTime2 - LedTime1) >= rlcSettings.RefreshInterval)
						{
							ColorIndex = 0;
							LedTime1 = millis();
							continue;
						}
						else
						{
							clientState = 2;
							SendPackage(3);
							PackageParseResult = messageParser.Parse();
							if (PackageParseResult > 0) { Serial.print("PackageResult: "); Serial.println(PackageParseResult); }
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
								clientState = 3;
								SendPackage(3);
								PauseTime1 = millis();
								PackageParseResult = messageParser.Parse();
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
	}*/
}

void WiFiConnect() {
	unsigned long connectionTime = 0;
	connectionTime = 0;
	Serial.print("Connecting to WiFi: "); Serial.println(rlcSettings.SSID);
	LabelConnection:
	WiFi.disconnect();
	WiFi.begin(rlcSettings.SSID.c_str(), rlcSettings.Password.c_str());
	FastLED.showColor(CRGB::Gold, 100);
	while (WiFi.status() != WL_CONNECTED) {
		delay(10);
		if (connectionTime > 10000)
		{
			goto LabelConnection;
		}
		connectionTime += 10;
	}
	broadcastAddress = ~WiFi.subnetMask() | WiFi.gatewayIP();
	FastLED.clear(true);
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.print("Local IP: "); Serial.println(WiFi.localIP());
}

void FastLEDInitialization() {
	ledArray = new CRGB[rlcSettings.LEDCount];
	int startLED = 0;
	for (size_t i = 0; i < rlcSettings.PinsCount; i++)
	{
		switch (rlcSettings.Pins[i].Number)
		{
		case 0:
			FastLED.addLeds<WS2812B, 0, GRB>(ledArray, startLED, rlcSettings.Pins[i].LedCount);
			Serial.print("Select Pin 0, ");
			break;
		case 2:
			FastLED.addLeds<WS2812B, 2, GRB>(ledArray, startLED, rlcSettings.Pins[i].LedCount);
			Serial.print("Select Pin 2, ");
			break;
		case 4:
			FastLED.addLeds<WS2812B, 4, GRB>(ledArray, startLED, rlcSettings.Pins[i].LedCount);
			Serial.print("Select Pin 4, ");
			break;
		case 5:
			FastLED.addLeds<WS2812B, 5, GRB>(ledArray, startLED, rlcSettings.Pins[i].LedCount);
			Serial.print("Select Pin 5, ");
			break;
		default:
			break;
		}
		Serial.print("Pin: "); Serial.print(rlcSettings.Pins[i].Number);
		Serial.print(", Start LED: "); Serial.print(startLED);
		Serial.print(", LED count: "); Serial.println(rlcSettings.Pins[i].LedCount);
		startLED += rlcSettings.Pins[i].LedCount;
	}
}

/*
void SendPackage(uint8 PackageType)
{
	uint32 f = rlcSettings.ProjectKey;
	uint16 ContLength;
	uint16 PackageLength = rlcSettings.UDPPackageSize;
	uint8 *PackageBody = new uint8[PackageLength];
	//Cient - 1, Server - 0
	PackageBody[0] = 1;
	PackageBody[1] = (f >> 24) & 0xFF;
	PackageBody[2] = (f >> 16) & 0xFF;
	PackageBody[3] = (f >> 8) & 0xFF;
	PackageBody[4] = (f >> 0) & 0xFF;
	PackageBody[5] = PackageType;
	switch (PackageType)
	{
	case 3:
		ContLength = 2;
		PackageBody[6] = (ContLength >> 8) & 0xFF;
		PackageBody[7] = (ContLength >> 0) & 0xFF;
		PackageBody[8] = rlcSettings.PlateNumber;
		PackageBody[8] = rlcSettings.PlateNumber;
		PackageBody[9] = clientState;
		break;
	default:
		break;
	}
	for (size_t i = ContLength + 7; i < PackageLength; i++)
	{
		PackageBody[i] = 0;
	}

	udp.beginPacket(serverIP, rlcSettings.UDPPort);
	udp.write(PackageBody, PackageLength);
	udp.endPacket();
	delete[] PackageBody;
}

void SendPackageBroadcast(uint8 PackageType)
{
	IPAddress ipMulti(255, 255, 255, 255);
	uint32 f = rlcSettings.ProjectKey;
	uint16 ContLength;
	uint16 PackageLength = rlcSettings.UDPPackageSize;
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
	udp.beginPacketMulticast(ipMulti, rlcSettings.UDPPort, WiFi.localIP());
	udp.write(PackageBody, PackageLength);
	udp.endPacket();
	delete[] PackageBody;
}

byte ParsePackage() {
	byte result = 0;
	byte ptype;
	byte PlateCount;
	byte PlateNumber;
	byte Command;

	unsigned int ContentLength;
	unsigned int psize = udp.parsePacket();
	if (psize == rlcSettings.UDPPackageSize) {
		unsigned long Key = (udp.read() << 24) + (udp.read() << 16) + (udp.read() << 8) + (udp.read());
		if (Key == rlcSettings.ProjectKey)
		{
			ptype = udp.read();
			switch (ptype)
			{
			case 1:
				ContentLength = (udp.read() << 8) + (udp.read());
				PlateCount = udp.read();
				if (PlateCount == 0)
				{
					result = 1;
					break;
				}
				for (byte i = 1; i <= PlateCount; i++)
				{
					PlateNumber = udp.read();
					if (PlateNumber == rlcSettings.PlateNumber)
					{
						result = 1;
						break;
					}
				}
			case 2:
				ContentLength = (udp.read() << 8) + (udp.read());
				PlateCount = udp.read();
				if (PlateCount == 0)
				{
					result = 2;
					break;
				}
				for (byte i = 1; i <= PlateCount; i++)
				{
					PlateNumber = udp.read();
					if (PlateNumber == rlcSettings.PlateNumber)
					{
						result = 2;
						break;
					}
				}
				break;
			case 5:
				ContentLength = (udp.read() << 8) + (udp.read());
				serverIP.operator[](0) = udp.read();
				serverIP.operator[](1) = udp.read();
				serverIP.operator[](2) = udp.read();
				serverIP.operator[](3) = udp.read();
				result = 5;
				Serial.println(serverIP.toString());
				break;
			case 6:
				result = 6;
				break;
			case 7:
				ContentLength = (udp.read() << 8) + (udp.read());
				CurrentTime = (udp.read() << 24) + (udp.read() << 16) + (udp.read() << 8) + (udp.read());
				CurrentTime *= (rlcSettings.LEDCount * 3);
				result = 7;
				break;
			case 12:
				Serial.print("Package 12 recieved, "); Serial.print("Client: ");
				ContentLength = (udp.read() << 8) + (udp.read());
				PlateNumber = udp.read();
				Serial.println(PlateNumber);
				if (PlateNumber == rlcSettings.PlateNumber)
				{
					CurrentTime = (udp.read() << 24) + (udp.read() << 16) + (udp.read() << 8) + (udp.read());
					CurrentTime *= (rlcSettings.LEDCount * 3);
					result = 7;
				}
				break;
			default:
				break;
			}
		}
	}
	return result;
}*/