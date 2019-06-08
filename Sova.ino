#include <Ticker.h>
#include <SD.h>
#include <Hash.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "FastLED.h"
#include "RLCSetting.h"
#include "RLCMessage/RLCMessage.h"
#include "RLCMessage/RLCMessageParser.h"
#include "RLCMessage/RLCEnums.h"
#include "RLCMessage/RLCMessageFactory.h"
#include "RLCLedController/RLCLedController.h"
#include "TimeSynchronization/SyncTime.h"
#include "Service/PinController.h"

#define chipSelect 15
//таймаут ожидания при запросе по мультикасту
#define SERVER_IP_TIMEOUT 2000UL
#define NTP_PORT 11011
//таймаут ожидания успешного соединения в рабочем цикле, мс
#define TCP_CONNECTION_TIMEOUT_ON_WORK 10

//значения HIGH и LOW специально инвертированы из-за оборудования
#define ANALOG_HIGH 0
#define ANALOG_LOW 255

Ticker ticker;

//CRGB *ledArray;
File cyclogrammFile;
File settingFile;
IPAddress broadcastAddress;
IPAddress serverIP = IPAddress(0, 0, 0, 0);
String cyclogrammFileName = "Data.cyc";
uint8 *rlcMessageBuffer = new uint8[RLC_MESSAGE_LENGTH];
WiFiClient tcpClient;
WiFiUDP udp;
SyncTime syncTime;
unsigned long lastTryingConnectionTime;
ClientStateEnum clientState;
RLCSetting rlcSettings;
RLCMessageParser messageParser;
RLCMessageFactory messageFactory;
boolean connectionInProgress = false;
RLCLedController rlcLedController;

void Initializations() 
{
	Serial.begin(115200);
	clientState = ClientStateEnum::Stoped;
	syncTime.LastTime = Time(63681897600, 0); //01.01.2019 00:00:00:000000

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
	FastLED.showColor(CRGB::Magenta);
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
				Serial.print("Received server IP packet: "); Serial.println(rlcMessageBuffer[5]);
				RLCMessage response = messageParser.Parse(rlcMessageBuffer);
				if(!response.IsInitialized) {
					Serial.println("Response not initialized");
					break;
				}
				if(response.MessageType != MessageTypeEnum::SendServerIP) {
					Serial.println("Message type not SendServerIP");
					break;
				}
				if(response.IP == IPAddress(0, 0, 0, 0)) {
					Serial.println("Server IP is empty");
					break;
				}
				serverIP = response.IP;
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

void WaitingTimeSynchronization(IPAddress &ipAddress, uint16_t port)
{
	FastLED.showColor(CRGB::Green);
	udp.begin(port);
	syncTime.Init(udp);
	while(!syncTime.SynchronizeTime(ipAddress, port))
	{
	}
	udp.stopAll();
}

//ожидание успешного TCP соеднинения
void WaitingConnectToRLCServer(IPAddress &ipAddress, uint16_t port)
{
	Serial.print("Waiting connect to "); Serial.print(ipAddress); Serial.print(":"); Serial.println(port);
	FastLED.showColor(CRGB::Blue);
	unsigned long connectionTimePoint = millis();

	int tcpConnected = 0;
	while(!tcpConnected)
	{
		if((millis() - connectionTimePoint) >= 1000)
		{
			tcpConnected = TryConnectToRLCServer(ipAddress, port, 1000UL);
		}
	}
}

//попытка однократного TCP соединения 
int TryConnectToRLCServer(IPAddress &ipAddress, uint16_t port, unsigned long timeout)
{
	int tcpConnected = 0;
	//пересоздаем клиент, потому что при разрыве связи не может заного подключиться
	tcpClient = WiFiClient();
	Serial.println("Connecting to TCP server");
	tcpClient.setTimeout(timeout);

	tcpConnected = tcpClient.connect(ipAddress, port);
	
	if(tcpConnected)
	{
		lastTryingConnectionTime = millis();
		tcpClient.setTimeout(0);
		tcpClient.keepAlive(2, 1);
		Serial.println("TCP connected.");
	}
	return tcpConnected;
}

void ReadTCPConnection() 
{
	if(tcpClient.connected()) {
		connectionInProgress = false;
	}

	if(!tcpClient.connected() && !connectionInProgress)
	{
		Serial.print("Disconnected. Try reconnect");
		TryConnectToRLCServer(serverIP, rlcSettings.UDPPort, TCP_CONNECTION_TIMEOUT_ON_WORK);
		lastTryingConnectionTime = millis();
		//не задерживаем главный цикл после попытки подключения, сразу возвращая управление
		return;
	}

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
	RLCMessage responseMessage;
	switch(message.MessageType)
	{
	case MessageTypeEnum::Play:
		Serial.println("Receive Play message");
		rlcLedController.Play();
		break;
	case MessageTypeEnum::Pause:
		Serial.println("Receive Pause message");
		rlcLedController.Pause();
		break;
	case MessageTypeEnum::Stop:
		Serial.println("Receive Stop message");
		rlcLedController.Stop();
		break;
	case MessageTypeEnum::PlayFrom:
		Serial.println("Receive PlayFrom message");
		rlcLedController.SetPosition(message.TimeFrame);
		rlcLedController.Play();
		break;
	case MessageTypeEnum::RequestClientInfo:
		responseMessage = messageFactory.SendClientInfo(clientState);
		SendMessage(responseMessage);
		break;
	default:
		break;
	}
}

void SendMessage(RLCMessage message) {
	tcpClient.write(message.GetBytes(), RLC_MESSAGE_LENGTH);
}

void OpenCyclogrammFile()
{
	if(!cyclogrammFile) {
		cyclogrammFile = SD.open(cyclogrammFileName);
		Serial.print("Cyclogramm opened: "); Serial.println(cyclogrammFile.name());
		Serial.print("Cyclogramm data available: "); Serial.println(cyclogrammFile.available());
	}
}

void NextFrameHandler()
{
	rlcLedController.NextFrame();
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
	Serial.println(rlcSettings.SPILedCount);
	Serial.print("Pins count: ");
	Serial.println(rlcSettings.PinsCount);	
	for (size_t i = 0; i < rlcSettings.PinsCount; i++)
	{
		Serial.print("Pin: ");
		Serial.print(ToString(rlcSettings.Pins[i].Type));
		Serial.print(rlcSettings.Pins[i].Number);
		Serial.print("-");
		Serial.println(rlcSettings.Pins[i].LedCount);
	}
	Serial.print("DefaultLightMode: ");
	Serial.println(rlcSettings.DefaultLightOn);
	Serial.print("SPILedGlobalBrightness: ");
	Serial.println(rlcSettings.SPILedGlobalBrightness);

	OpenCyclogrammFile();
	rlcLedController.Initialize(FastLEDInitialization, cyclogrammFile, OpenCyclogrammFile);

	Serial.print("----------------------------");
	Serial.println();

	IsDigitalOutputss = rlcSettings.IsDigitalPWMSignal;
	InvertedOutputss = rlcSettings.InvertedPWMSignal;
	messageFactory = RLCMessageFactory(rlcSettings.ProjectKey, rlcSettings.PlateNumber);
	
	WiFiConnect();
	WaitingServerIPAddress();
	WaitingTimeSynchronization(serverIP, NTP_PORT);
	WaitingConnectToRLCServer(serverIP, rlcSettings.UDPPort);
	DefaultLight();
	ticker.attach_ms(rlcLedController.FrameTime, NextFrameHandler);
}

void loop(void) {
	ReadTCPConnection();	
	rlcLedController.Show();
}

void WiFiConnect() {
	unsigned long connectionTime = 0;
	connectionTime = 0;
	Serial.print("Connecting to WiFi: "); Serial.println(rlcSettings.SSID);
	LabelConnection:
	WiFi.disconnect();
	WiFi.begin(rlcSettings.SSID.c_str(), rlcSettings.Password.c_str());
	FastLED.showColor(CRGB::Yellow);
	while (WiFi.status() != WL_CONNECTED) {
		delay(10);
		if (connectionTime > 10000)
		{
			goto LabelConnection;
		}
		connectionTime += 10;
	}
	broadcastAddress = ~WiFi.subnetMask() | WiFi.gatewayIP();
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.print("Local IP: "); Serial.println(WiFi.localIP());
}

void FastLEDInitialization()
{
	rlcLedController.LedCount = rlcSettings.SPILedCount;
	rlcLedController.LedArray = new CRGB[rlcLedController.LedCount];
	rlcLedController.PWMChannelCount = rlcSettings.PWMChannelCount;
	rlcLedController.PWMChannels = new int[rlcLedController.PWMChannelCount];
	int pwmChannelIndex = 0;
	int startLED = 0;
	for (size_t i = 0; i < rlcSettings.PinsCount; i++)
	{
		if(rlcSettings.Pins[i].Type == PinType::SPI)
		{
			switch(rlcSettings.Pins[i].Number)
			{
			case 0:
				FastLED.addLeds<WS2812B, 0, GRB>(rlcLedController.LedArray, startLED, rlcSettings.Pins[i].LedCount);
				Serial.print("Select Pin 0, ");
				break;
			case 2:
				FastLED.addLeds<WS2812B, 2, GRB>(rlcLedController.LedArray, startLED, rlcSettings.Pins[i].LedCount);
				Serial.print("Select Pin 2, ");
				break;
			case 4:
				FastLED.addLeds<WS2812B, 4, GRB>(rlcLedController.LedArray, startLED, rlcSettings.Pins[i].LedCount);
				Serial.print("Select Pin 4, ");
				break;
			case 5:
				FastLED.addLeds<WS2812B, 5, GRB>(rlcLedController.LedArray, startLED, rlcSettings.Pins[i].LedCount);
				Serial.print("Select Pin 5, ");
				break;
			default:
				break;
			}
			Serial.print("Pin SPI: "); Serial.print(rlcSettings.Pins[i].Number);
			Serial.print(", Start LED: "); Serial.print(startLED);
			Serial.print(", LED count: "); Serial.println(rlcSettings.Pins[i].LedCount);
			startLED += rlcSettings.Pins[i].LedCount;
		}
		else if(rlcSettings.Pins[i].Type == PinType::PWM)
		{
			rlcLedController.PWMChannels[pwmChannelIndex] = rlcSettings.Pins[i].Number;
			pinMode(rlcSettings.Pins[i].Number, OUTPUT);
			Serial.print("Pin PWM: "); Serial.print(rlcLedController.PWMChannels[pwmChannelIndex]);
			pwmChannelIndex++;
		}
	}
	FastLED.setBrightness(rlcSettings.SPILedGlobalBrightness);
}

void DefaultLight() {
	if(!rlcLedController.IsInitialized)
	{
		return;
	}
	//Включение свечения по умолчанию, если включена настройка
	if(rlcSettings.DefaultLightOn)
	{
		Serial.print("LED Brightness: "); Serial.print(FastLED.getBrightness());
		FastLED.showColor(CRGB::White);
		for(size_t i = 0; i < rlcLedController.PWMChannelCount; i++)
		{
			PinWrite(rlcLedController.PWMChannels[i], ANALOG_HIGH);
		}
	}
	else
	{
		FastLED.clear(true);
		for(size_t i = 0; i < rlcLedController.PWMChannelCount; i++)
		{
			PinWrite(rlcLedController.PWMChannels[i], ANALOG_LOW);
		}
	}
}