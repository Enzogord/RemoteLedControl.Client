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

#define chipSelect 15
//������� �������� ��� ������� �� �����������
#define SERVER_IP_TIMEOUT 2000UL
#define NTP_PORT 11011
//������� �������� ��������� ���������� � ������� �����, ��
#define TCP_CONNECTION_TIMEOUT_ON_WORK 10UL

//�������� HIGH � LOW ���������� ������������� ��-�� ������������
#define ANALOG_HIGH 255
#define ANALOG_LOW 0

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

void Initializations()
{
	Serial.begin(115200);
	pinMode(A0, INPUT);
	clientState = ClientStateEnum::Stoped;
	//syncTime.LastTime = Time(63681897600, 0); //01.01.2019 00:00:00:000000

	// ���������� ����� �������
	WiFi.softAPdisconnect(true);

	InitializeSDCard();
}

void InitializeSDCard()
{
	Serial.print("Initializing SD card");
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
		while (serverIP == IPAddress(0, 0, 0, 0) && (millis() - timeCheckpoint) < SERVER_IP_TIMEOUT)
		{
			if (udp.available()) {
				memset(rlcMessageBuffer, 0, RLC_MESSAGE_LENGTH);
				udp.readBytes(rlcMessageBuffer, RLC_MESSAGE_LENGTH);
				Serial.print("Received server IP packet: "); Serial.println(rlcMessageBuffer[5]);
				RLCMessage response = messageParser.Parse(rlcMessageBuffer);
				if (!response.IsInitialized) {
					Serial.println("Response not initialized");
					break;
				}
				if (response.MessageType != MessageTypeEnum::SendServerIP) {
					Serial.println("Message type not SendServerIP");
					break;
				}
				if (response.IP == IPAddress(0, 0, 0, 0)) {
					Serial.println("Server IP is empty");
					break;
				}
				serverIP = response.IP;
			}
		}
		udp.stopAll();
		if (serverIP == IPAddress(0, 0, 0, 0))
		{
			Serial.println("Failed to receive server IP address, retry.");
		}
	} while (serverIP == IPAddress(0, 0, 0, 0));
	Serial.print("Received server IP: "); Serial.println(serverIP);
}

void WaitingTimeSynchronization(IPAddress& ipAddress, uint16_t port)
{
	FastLED.showColor(CRGB::Green);
	udp.begin(port);
	syncTime.Init(udp);
	syncTime.SynchronizeTimeMultiple(serverIP, NTP_PORT, 30);
	udp.stopAll();
}

//�������� ��������� TCP ����������
void WaitingConnectToRLCServer(IPAddress& ipAddress, uint16_t port)
{
	Serial.print("Waiting connect to "); Serial.print(ipAddress); Serial.print(":"); Serial.println(port);
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

//������� ������������ TCP ���������� 
int TryConnectToRLCServer(IPAddress& ipAddress, uint16_t port, unsigned long timeout)
{
	int tcpConnected = 0;
	//����������� ������, ������ ��� ��� ������� ����� �� ����� ������ ������������
	tcpClient = WiFiClient();
	Serial.println("Connecting to TCP server");
	tcpClient.setTimeout(timeout);

	tcpConnected = tcpClient.connect(ipAddress, port);

	if (tcpConnected)
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
	if (tcpClient.connected()) {
		connectionInProgress = false;
	}

	if (!tcpClient.connected() && !connectionInProgress)
	{
		Serial.print("Disconnected. Try reconnect");
		TryConnectToRLCServer(serverIP, rlcSettings.UDPPort, TCP_CONNECTION_TIMEOUT_ON_WORK);
		lastTryingConnectionTime = millis();
		//�� ����������� ������� ���� ����� ������� �����������, ����� ��������� ����������
		return;
	}

	uint8_t packetBuffer[1024];

	while (tcpClient.available()) {
		Serial.print("[TCP] Received data form server: ");
		tcpClient.readBytes(packetBuffer, 1024);
		RLCMessage message = messageParser.Parse(packetBuffer);
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
		Serial.println("--- Receive PlayFrom message ---");
		CheckStartFrameTicker(message.SendTime);
		rlcLedController.PlayFrom(message.PlayFromTime, message.SendTime);
		break;
	case MessageTypeEnum::Rewind:
		Serial.println("Receive Rewind message");
		CheckStartFrameTicker(message.SendTime);
		rlcLedController.Rewind(message.PlayFromTime, message.SendTime, message.ClientState);
		break;
	case MessageTypeEnum::RequestClientInfo:
		responseMessage = messageFactory.SendClientInfo(clientState);
		SendMessage(responseMessage);
		break;
	default:
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
	Serial.print("Time now: "); Serial.print(now.GetSeconds()); Serial.print(" sec, "); Serial.print(now.GetMicroseconds()); Serial.println("us");
	Serial.print("Send time: "); Serial.print(sendTime.GetSeconds()); Serial.print(" sec, "); Serial.print(sendTime.GetMicroseconds()); Serial.println("us");
	Serial.print("deltaTime: "); Serial.print((uint32_t)deltaTime); Serial.println(" ms");
	Serial.print("waitTime: "); Serial.print(waitTime); Serial.println("ms");


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
		Serial.print("Cyclogramm opened: "); Serial.println(cyclogrammFile.name());
		Serial.print("Cyclogramm data available: "); Serial.println(cyclogrammFile.available());
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
		//Serial.print("Send battery charge level: "); Serial.println(chargeValue);
		RLCMessage batteryChargeMessage = messageFactory.BatteryCharge(clientState, chargeValue);

		SendMessage(batteryChargeMessage);
		sendBatteryCharge = false;
	}
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
		if (rlcSettings.Pins[i].Type == PinType::SPI)
		{
			switch (rlcSettings.Pins[i].Number)
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
		else if (rlcSettings.Pins[i].Type == PinType::PWM)
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
	if (!rlcLedController.IsInitialized)
	{
		return;
	}
	//��������� �������� �� ���������, ���� �������� ���������
	if (rlcSettings.DefaultLightOn)
	{
		Serial.print("LED Brightness: "); Serial.print(FastLED.getBrightness());
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

// the setup function runs once when you press reset or power the board
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

// the loop function runs over and over again until power down or reset
void loop(void) {
	SendBatteryCharge();
	ReadTCPConnection();
	rlcLedController.Show();
}