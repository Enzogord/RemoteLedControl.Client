/*
 Name:		RlcClientForESP8266.ino
 Created:	3/9/2020 4:04:55 PM
 Author:	Enzo
*/

#include <Ticker.h>
#include <SD.h>
#include <Hash.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "FastLED.h"
#include "RLCSetting/RLCSetting.h"
#include "RLCMessage/RLCMessage.h"
#include "RLCMessage/RLCMessageParser.h"
#include "RLCMessage/RLCEnums.h"
#include "RLCMessage/RLCMessageFactory.h"
#include "RLCLedController/RLCLedController.h"
#include "TimeSynchronization/SyncTime.h"
#include "Service/PinController.h"

#include <LogManager.h>
#include "RLCLibraryImplementations/Logger/SerialLogger.h"

#define DEBUG true

#define chipSelect 15
//таймаут ожидания при запросе по мультикасту
#define SERVER_IP_TIMEOUT 2000UL
#define NTP_PORT 11011
//таймаут ожидания успешного соединения в рабочем цикле, мс
#define TCP_CONNECTION_TIMEOUT_ON_WORK 10UL

//значения HIGH и LOW специально инвертированы из-за оборудования
#define ANALOG_HIGH 255
#define ANALOG_LOW 0

ILogger* logger;

Ticker tickerFrame;
Ticker tickerBattery;

File cyclogrammFile;
File settingFile;
IPAddress broadcastAddress;
IPAddress serverIP = IPAddress(0, 0, 0, 0);
String cyclogrammFileName = "Data.cyc";
uint8* rlcMessageBuffer = new uint8[RLC_MESSAGE_LENGTH];
WiFiClient tcpClient;
WiFiUDP udp;
SyncTime syncTime;
unsigned long lastTryingConnectionTime;
ClientStateEnum clientState;
RLCSetting rlcSettings;
RLCMessageParser messageParser;
RLCMessageFactory messageFactory;
boolean connectionInProgress = false;
RLCLedController rlcLedController = RLCLedController(&syncTime);
bool sendBatteryCharge;

void ConfigureLogger() {
	logger = new SerialLogger(115200);

#if DEBUG
	logger->Enable();
#endif
}

void Initializations()
{
	ConfigureLogger();

	Serial.begin(115200);
	pinMode(A0, INPUT);
	clientState = ClientStateEnum::Stoped;
	//syncTime.LastTime = Time(63681897600, 0); //01.01.2019 00:00:00:000000

	// Отключение точки доступа
	WiFi.softAPdisconnect(true);

	InitializeSDCard();
}

void InitializeSDCard()
{
	logger->Print("Initializing SD card");
	boolean sdActive = false;
	unsigned long sdTime = millis();
	while (!sdActive)
	{
		if ((millis() - sdTime) >= 50)
		{
			sdTime = millis();
			sdActive = SD.begin(chipSelect);
		}
	}

	logger->Print("SD card successfully initialized!");
}

void WaitingServerIPAddress()
{
	FastLED.showColor(CRGB::Magenta);
	logger->Print("Waiting Server IP address");
	RLCMessage requestServerIPMessage = messageFactory.RequestServerIP(clientState);
	uint8_t* requestBytes = requestServerIPMessage.GetBytes();
	do {
		logger->Print("Request Server IP address. ");
		udp.begin(rlcSettings.UDPPort);
		udp.beginPacket(broadcastAddress, rlcSettings.UDPPort);
		udp.write(requestBytes, RLC_MESSAGE_LENGTH);
		udp.endPacket();
		unsigned long timeCheckpoint = millis();
		while (serverIP == IPAddress(0, 0, 0, 0) && (millis() - timeCheckpoint) < SERVER_IP_TIMEOUT)
		{
			if (udp.available()) {
				memset(rlcMessageBuffer, 0, RLC_MESSAGE_LENGTH);
				udp.readBytes(rlcMessageBuffer, RLC_MESSAGE_LENGTH);
				logger->Print("Received server IP packet: ", rlcMessageBuffer[5]);
				RLCMessage response = messageParser.Parse(rlcMessageBuffer);
				if (!response.IsInitialized) {
					logger->Print("Response not initialized");
					break;
				}
				if (response.MessageType != MessageTypeEnum::SendServerIP) {
					logger->Print("Message type not SendServerIP");
					break;
				}
				if (response.IP == IPAddress(0, 0, 0, 0)) {
					logger->Print("Server IP is empty");
					break;
				}
				serverIP = response.IP;
			}
		}
		udp.stopAll();
		if (serverIP == IPAddress(0, 0, 0, 0))
		{
			logger->Print("Failed to receive server IP address, retry.");
		}
	} while (serverIP == IPAddress(0, 0, 0, 0));
	logger->Print("Received server IP: ", serverIP);
}

void WaitingTimeSynchronization(IPAddress& ipAddress, uint16_t port)
{
	FastLED.showColor(CRGB::Green);
	udp.begin(port);
	syncTime.Init(udp);
	syncTime.SynchronizeTimeMultiple(serverIP, NTP_PORT, 30);
	udp.stopAll();
}

//ожидание успешного TCP соединения
void WaitingConnectToRLCServer(IPAddress& ipAddress, uint16_t port)
{
	logger->Print("Waiting connect to ", ipAddress, false); logger->Print(":", port);
	FastLED.showColor(CRGB::Blue);
	unsigned long connectionTimePoint = millis();

	int tcpConnected = 0;
	while (!tcpConnected)
	{
		if ((millis() - connectionTimePoint) >= 1000)
		{
			tcpConnected = TryConnectToRLCServer(ipAddress, port, 1000UL);
		}
	}
}

//попытка однократного TCP соединения 
int TryConnectToRLCServer(IPAddress& ipAddress, uint16_t port, unsigned long timeout)
{
	int tcpConnected = 0;
	//пересоздаем клиент, потому что при разрыве связи не может заново подключиться
	tcpClient = WiFiClient();
	logger->Print("Connecting to TCP server");
	tcpClient.setTimeout(timeout);

	tcpConnected = tcpClient.connect(ipAddress, port);

	if (tcpConnected)
	{
		lastTryingConnectionTime = millis();
		tcpClient.setTimeout(0);
		tcpClient.keepAlive(2, 1);
		logger->Print("TCP connected.");
	}
	return tcpConnected;
}

void ReadTCPConnection()
{
	if (tcpClient.connected()) {
		connectionInProgress = false;
	}

	if (!tcpClient.connected() && !connectionInProgress)
	{
		logger->Print("Disconnected. Try reconnect");
		TryConnectToRLCServer(serverIP, rlcSettings.UDPPort, TCP_CONNECTION_TIMEOUT_ON_WORK);
		lastTryingConnectionTime = millis();
		//не задерживаем главный цикл после попытки подключения, сразу возвращая управление
		return;
	}

	uint8_t packetBuffer[1024];

	while (tcpClient.available()) {
		logger->Print("[TCP] Received data form server: ");
		tcpClient.readBytes(packetBuffer, 1024);
		RLCMessage message = messageParser.Parse(packetBuffer);
		logger->Print("message.IsInitialized: ", message.IsInitialized, true);
		logger->Print("message.Key: ", message.Key);
		logger->Print("message.SourceType: ", ToString(message.SourceType));
		if (message.IsInitialized && message.Key == rlcSettings.ProjectKey && message.SourceType == SourceTypeEnum::Server)
		{
			OnReceiveMessage(message);
		}
	}
}

void OnReceiveMessage(RLCMessage& message)
{
	RLCMessage responseMessage;
	switch (message.MessageType)
	{
	case MessageTypeEnum::Play:
		logger->Print("Receive Play message");
		rlcLedController.Play();
		break;
	case MessageTypeEnum::Pause:
		logger->Print("Receive Pause message");
		rlcLedController.Pause();
		break;
	case MessageTypeEnum::Stop:
		logger->Print("Receive Stop message");
		rlcLedController.Stop();
		break;
	case MessageTypeEnum::PlayFrom:
		logger->Print("Receive PlayFrom message");
		CheckStartFrameTicker(message.SendTime);
		rlcLedController.PlayFrom(message.PlayFromTime, message.SendTime);
		break;
	case MessageTypeEnum::Rewind:
		logger->Print("Receive Rewind message");
		CheckStartFrameTicker(message.SendTime);
		rlcLedController.Rewind(message.PlayFromTime, message.SendTime, message.ClientState);
		break;
	case MessageTypeEnum::RequestClientInfo:
		responseMessage = messageFactory.SendClientInfo(clientState);
		SendMessage(responseMessage);
		break;
	default:
		logger->Print("Receive message: ", ToString(message.MessageType));
		break;
	}
}

bool frameTickerStarted;

void CheckStartFrameTicker(Time sendTime)
{
	
	if (frameTickerStarted)
	{
		return;
	}

	Time now = syncTime.Now();
	int64_t deltaTime = now.TotalMicroseconds - sendTime.TotalMicroseconds;
	if (deltaTime < 0)
	{
		deltaTime = 0;
	}
	deltaTime /= 1000;

	uint32_t waitTime = rlcLedController.frameTime - (deltaTime % rlcLedController.frameTime);
	logger->Print("Time now: ", now.GetSeconds(), false); 
	logger->Print(" sec, ", now.GetMicroseconds(), false); 
	logger->Print("us");

	logger->Print("Send time: ", sendTime.GetSeconds(), false); 
	logger->Print(" sec, ", sendTime.GetMicroseconds(), false);
	logger->Print("us");

	logger->Print("deltaTime: ", (uint32_t)deltaTime, false); logger->Print(" ms");
	logger->Print("waitTime: ", waitTime, false); logger->Print("ms");


	tickerFrame.detach();
	tickerFrame.once_ms(waitTime, StartFrameTicker);
	frameTickerStarted = true;
}

void StartFrameTicker()
{
	tickerFrame.detach();
	tickerFrame.attach_ms(rlcLedController.frameTime, NextFrameHandler);
}

void SendMessage(RLCMessage& message) {
	uint8_t* buffer = message.GetBytes();
	tcpClient.write(buffer, RLC_MESSAGE_LENGTH);
	delete(buffer);
}

void OpenCyclogrammFile()
{
	if (!cyclogrammFile) {
		cyclogrammFile = SD.open(cyclogrammFileName);
		logger->Print("Cyclogramm opened: ", cyclogrammFile.name());
		logger->Print("Cyclogramm data available: ", cyclogrammFile.available());
	}
}

Time lastFrameTime;
void NextFrameHandler()
{
	rlcLedController.NextFrame();
	lastFrameTime = syncTime.Now();
}

void BatteryChargeHandler()
{
	if (tcpClient.connected())
	{
		sendBatteryCharge = true;
	}

}

void SendBatteryCharge()
{
	if (sendBatteryCharge && tcpClient.connected())
	{
		uint16_t chargeValue = (uint16_t)analogRead(A0);
		RLCMessage batteryChargeMessage = messageFactory.BatteryCharge(clientState, chargeValue);

		SendMessage(batteryChargeMessage);
		sendBatteryCharge = false;
	}
}

void WiFiConnect() {
	
	unsigned long connectionTime = 0;
	connectionTime = 0;
	logger->Print("Connecting to WiFi: ", rlcSettings.SSID);
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
	logger->Print("WiFi connected");
	logger->Print("Local IP: ", WiFi.localIP());
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
		if (rlcSettings.Pins[i].Type == PinType::SPI)
		{
			switch (rlcSettings.Pins[i].Number)
			{
			case 0:
				FastLED.addLeds<WS2812B, 0, GRB>(rlcLedController.LedArray, startLED, rlcSettings.Pins[i].LedCount);
				logger->Print("Select Pin 0, ", false);
				break;
			case 2:
				FastLED.addLeds<WS2812B, 2, GRB>(rlcLedController.LedArray, startLED, rlcSettings.Pins[i].LedCount);
				logger->Print("Select Pin 2, ", false);
				break;
			case 4:
				FastLED.addLeds<WS2812B, 4, GRB>(rlcLedController.LedArray, startLED, rlcSettings.Pins[i].LedCount);
				logger->Print("Select Pin 4, ", false);
				break;
			case 5:
				FastLED.addLeds<WS2812B, 5, GRB>(rlcLedController.LedArray, startLED, rlcSettings.Pins[i].LedCount);
				logger->Print("Select Pin 5, ", false);
				break;
			default:
				break;
			}
			logger->Print("Pin SPI: ", rlcSettings.Pins[i].Number, false);
			logger->Print(", Start LED: ", startLED, false);
			logger->Print(", LED count: ", rlcSettings.Pins[i].LedCount, false);
			startLED += rlcSettings.Pins[i].LedCount;
		}
		else if (rlcSettings.Pins[i].Type == PinType::PWM)
		{
			rlcLedController.PWMChannels[pwmChannelIndex] = rlcSettings.Pins[i].Number;
			pinMode(rlcSettings.Pins[i].Number, OUTPUT);
			logger->Print("Pin PWM: ", rlcLedController.PWMChannels[pwmChannelIndex]);
			pwmChannelIndex++;
		}
	}
	FastLED.setBrightness(rlcSettings.SPILedGlobalBrightness);
}

void DefaultLight() {
	if (!rlcLedController.IsInitialized)
	{
		return;
	}
	//Включение свечения по умолчанию, если включена настройка
	if (rlcSettings.DefaultLightOn)
	{
		//logger->Print("LED Brightness: ", FastLED.getBrightness());
		FastLED.showColor(CRGB::White);
		for (size_t i = 0; i < rlcLedController.PWMChannelCount; i++)
		{
			PinWrite(rlcLedController.PWMChannels[i], ANALOG_HIGH);
		}
	}
	else
	{
		FastLED.clear(true);
		for (size_t i = 0; i < rlcLedController.PWMChannelCount; i++)
		{
			PinWrite(rlcLedController.PWMChannels[i], ANALOG_LOW);
		}
	}
}

void setup()
{	
	ConfigureLogger();

	logger = new SerialLogger(115200);

#if DEBUG
	logger->Enable();
#endif
	logger->Print("Start.....");

	Initializations();

	settingFile = SD.open("set.txt", FILE_READ);
	rlcSettings.ReadSetting(settingFile);
	logger->PrintNewLine();
	logger->Print("-----Project parameters-----");
	logger->Print("SSID: ", rlcSettings.SSID);
	logger->Print("Password: ", rlcSettings.Password);
	logger->Print("PlateNumber: ", rlcSettings.PlateNumber);
	logger->Print("ProjectKey: ", rlcSettings.ProjectKey);
	logger->Print("UDPPackageSize: ", rlcSettings.UDPPackageSize);
	logger->Print("UDPPort: ", rlcSettings.UDPPort);
	logger->Print("LEDCount: ", rlcSettings.SPILedCount);
	logger->Print("Pins count: ", rlcSettings.PinsCount);
	for (size_t i = 0; i < rlcSettings.PinsCount; i++)
	{
		logger->Print("Pin: ", ToString(rlcSettings.Pins[i].Type), false);
		logger->Print("", rlcSettings.Pins[i].Number, false);
		logger->Print("-", rlcSettings.Pins[i].LedCount);
	}
	logger->Print("DefaultLightMode: ", rlcSettings.DefaultLightOn);
	logger->Print("SPILedGlobalBrightness: ", rlcSettings.SPILedGlobalBrightness);

	OpenCyclogrammFile();
	rlcLedController.Initialize(FastLEDInitialization, cyclogrammFile, OpenCyclogrammFile);

	IsDigitalOutput = rlcSettings.IsDigitalPWMSignal;
	InvertedOutput = rlcSettings.InvertedPWMSignal;
	messageFactory = RLCMessageFactory(rlcSettings.ProjectKey, rlcSettings.PlateNumber);

	WiFiConnect();
	WaitingServerIPAddress();
	WaitingTimeSynchronization(serverIP, NTP_PORT);
	WaitingConnectToRLCServer(serverIP, rlcSettings.UDPPort);

	DefaultLight();
	tickerBattery.attach_ms(1000, BatteryChargeHandler);
}


void loop(void) {
	SendBatteryCharge();
	ReadTCPConnection();
	rlcLedController.Show();
}