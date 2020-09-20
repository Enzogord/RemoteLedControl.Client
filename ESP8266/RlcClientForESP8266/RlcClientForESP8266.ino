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
#include "RLCMessage/MessageIdRegistry.h"
#include "RLCLedController/RLCLedController.h"
#include "Service/PinController.h"
#include "RLCLibraryImplementations/Logger/SerialLogger.h"
#include "RLCLibraryImplementations/Time/SntpClient.h"
#include <SNTP/TimeSynchronization.h>
#include <Time.h>
#include <TimeNow.h>

#define DEBUG true

#define chipSelect 15
//таймаут ожидания при запросе по мультикасту
#define SERVER_IP_TIMEOUT 2000UL
#define NTP_PORT 11011

//значения HIGH и LOW специально инвертированы из-за оборудования
#define ANALOG_HIGH 255
#define ANALOG_LOW 0

ILogger* logger;

Ticker tickerBattery;

WiFiUDP udp;
File cyclogrammFile;
File settingFile;
IPAddress broadcastAddress;
IPAddress serverIP = IPAddress(0, 0, 0, 0);
String cyclogrammFileName = "Data.cyc";
uint8* rlcMessageBuffer = new uint8[RLC_MESSAGE_LENGTH];
unsigned long lastTryingConnectionTime;
RLCSetting rlcSettings;
RLCMessageParser messageParser;
RLCMessageFactory messageFactory;
boolean connectionInProgress = false;
RLCLedController* rlcLedController;
MessageIdRegistry messageIdsRegistry = MessageIdRegistry();
bool sendBatteryCharge;

#pragma region Проводной запуск

bool wiredMode = false;
const int wiredButtonPin = 16;
int wiredButtonState = 0;

//переменная хранящее пуск воспроизведения
bool wiredStartIsInitialized;
//переменная хранящее пуск воспроизведения
bool wiredStarted;

//Функция получения состояния кнопки с учетом дребезга контакта
int GetButtonState()
{
	//старое состояние кнопки
	int oldButtonState = wiredButtonState;
	wiredButtonState = digitalRead(wiredButtonPin);
	if(oldButtonState != wiredButtonState) {
		delay(10);
		wiredButtonState = digitalRead(wiredButtonPin);
	}
	//Вывод состояния кнопки (только для отладки)
	if(oldButtonState != wiredButtonState) {
		logger->Print("button state: ", wiredButtonState);
	}
	return wiredButtonState;
}

#pragma endregion Проводной запуск


void ConfigureLogger() {
	logger = new SerialLogger(115200);

#if DEBUG
	logger->Enable();
#endif
}

void Initializations()
{
	ConfigureLogger();

	//Указываем метод для получения текущего времени выполнения программы
	SetWorkTimeFunction(micros64);

	rlcLedController = new RLCLedController(*logger);

	pinMode(A0, INPUT);

	// Отключение точки доступа
	WiFi.softAPdisconnect(true);

	InitializeSDCard();

	// инициализируем пин, подключенный к кнопке, как вход
	pinMode(wiredButtonPin, INPUT);
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
	RLCMessage requestServerIPMessage = messageFactory.RequestServerIP(GetClientState());
	uint8_t* requestBytes = requestServerIPMessage.GetBytes();
	WiFiUDP udpClient;
	udpClient.begin(rlcSettings.UDPPort);

	do {
		logger->Print("Request Server IP address. ");
		udpClient.beginPacket(broadcastAddress, rlcSettings.UDPPort);
		udpClient.write(requestBytes, RLC_MESSAGE_LENGTH);
		udpClient.endPacket();
		unsigned long timeCheckpoint = millis();
		while (serverIP == IPAddress(0, 0, 0, 0) && (millis() - timeCheckpoint) < SERVER_IP_TIMEOUT)
		{
			int packetLength = udpClient.parsePacket();
			if(packetLength == RLC_MESSAGE_LENGTH) {
				memset(rlcMessageBuffer, 0, RLC_MESSAGE_LENGTH);
				udpClient.readBytes(rlcMessageBuffer, RLC_MESSAGE_LENGTH);
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
		
		if (serverIP == IPAddress(0, 0, 0, 0))
		{
			logger->Print("Failed to receive server IP address, retry.");
		}
	} while (serverIP == IPAddress(0, 0, 0, 0));
	udpClient.stop();
	logger->Print("Received server IP: ", serverIP);
}

inline void StartReceiveFromRLCServer() {
	udp.begin(rlcSettings.UDPPort);
}

void ReadMessagesFromServer() {
	while(true) {
		int packetLength = udp.parsePacket();
		if(!packetLength) {
			return;
		}
		if(packetLength == RLC_MESSAGE_LENGTH) 
		{
			ReadRLCMessage();
		}
		else
		{
			//else ignoring incorrect packet
			logger->Print("Ignoring packet with length: ", packetLength);
			uint8* packetBuffer = new uint8[packetLength];
			udp.readBytes(packetBuffer, packetLength);
			delete(packetBuffer);
		}
	}
}

inline void ReadRLCMessage() {
	memset(rlcMessageBuffer, 0, RLC_MESSAGE_LENGTH);
	udp.readBytes(rlcMessageBuffer, RLC_MESSAGE_LENGTH);
	RLCMessage message = messageParser.Parse(rlcMessageBuffer);
	logger->Print("Receive message with type: ", rlcMessageBuffer[9]);
	if (message.IsInitialized && message.Key == rlcSettings.ProjectKey && message.SourceType == SourceTypeEnum::Server) {
		if(messageIdsRegistry.Contains(message.MessageId)) {
			Response(message.MessageId);
			return;
		}
		OnReceiveMessage(message);
		Response(message.MessageId);
		messageIdsRegistry.AppendId(message.MessageId);
	}
}

void Response(int32_t messageId)
{
	RLCMessage message = messageFactory.SendClientInfo(GetClientState());
	message.MessageId = messageId;
	SendMessage(message);
}

void SendMessage(RLCMessage& message) {
	uint8_t* buffer = message.GetBytes();
	udp.beginPacket(serverIP, rlcSettings.UDPPort);
	udp.write(buffer, RLC_MESSAGE_LENGTH);
	udp.endPacket();
	delete(buffer);
}

void OnReceiveMessage(RLCMessage& message)
{
	switch (message.MessageType)
	{
	case MessageTypeEnum::State:
		logger->Print("Receive State message");
		ChangeState(message);
		break;
	case MessageTypeEnum::RequestClientInfo:
		SendClientInfo();
		break;
	default:
		logger->Print("Receive message: ", ToString(message.MessageType));
		break;
	}
}

void ChangeState(RLCMessage message)
{
	switch (message.ClientState) {
		case ClientStateEnum::Stoped:
			rlcLedController->Stop();
			break;
		case ClientStateEnum::Playing:
			rlcLedController->Play(message.Frame, message.FrameStartTime);
			break;
		case ClientStateEnum::Paused:
			rlcLedController->Pause(message.Frame);
			break;
		default:
			break;
	}
}


ClientStateEnum GetClientState() {
	switch (rlcLedController->Status)
	{
	case LEDControllerStatuses::Stoped:   
		return ClientStateEnum::Stoped;
	case LEDControllerStatuses::Played:   
		return ClientStateEnum::Playing;
	case LEDControllerStatuses::Paused:  
		return ClientStateEnum::Paused;
	default:
		return ClientStateEnum::NotSet;
	}
}

void SendClientInfo() {
	RLCMessage message = messageFactory.SendClientInfo(GetClientState());
	SendMessage(message);
}

void OpenCyclogrammFile()
{
	if (!cyclogrammFile) {
		cyclogrammFile = SD.open(cyclogrammFileName);
		logger->Print("Cyclogramm opened: ", cyclogrammFile.name());
		logger->Print("Cyclogramm data available: ", cyclogrammFile.available());
	}
}

inline void BatteryChargeHandler()
{
	sendBatteryCharge = true;
}

void SendBatteryCharge()
{
	if (sendBatteryCharge)
	{
		uint16_t chargeValue = (uint16_t)analogRead(A0);
		RLCMessage batteryChargeMessage = messageFactory.BatteryCharge(GetClientState(), chargeValue);

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
	rlcLedController->LedCount = rlcSettings.SPILedCount;
	rlcLedController->LedArray = new CRGB[rlcLedController->LedCount];
	rlcLedController->PWMChannelCount = rlcSettings.PWMChannelCount;
	rlcLedController->PWMChannels = new int[rlcLedController->PWMChannelCount];
	int pwmChannelIndex = 0;
	int startLED = 0;
	for (size_t i = 0; i < rlcSettings.PinsCount; i++)
	{
		if (rlcSettings.Pins[i].Type == PinType::SPI)
		{
			switch (rlcSettings.Pins[i].Number)
			{
			case 0:
				FastLED.addLeds<WS2812B, 0, GRB>(rlcLedController->LedArray, startLED, rlcSettings.Pins[i].LedCount);
				logger->Print("Select Pin 0, ", false);
				break;
			case 2:
				FastLED.addLeds<WS2812B, 2, GRB>(rlcLedController->LedArray, startLED, rlcSettings.Pins[i].LedCount);
				logger->Print("Select Pin 2, ", false);
				break;
			case 4:
				FastLED.addLeds<WS2812B, 4, GRB>(rlcLedController->LedArray, startLED, rlcSettings.Pins[i].LedCount);
				logger->Print("Select Pin 4, ", false);
				break;
			case 5:
				FastLED.addLeds<WS2812B, 5, GRB>(rlcLedController->LedArray, startLED, rlcSettings.Pins[i].LedCount);
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
			rlcLedController->PWMChannels[pwmChannelIndex] = rlcSettings.Pins[i].Number;
			pinMode(rlcSettings.Pins[i].Number, OUTPUT);
			logger->Print("Pin PWM: ", rlcLedController->PWMChannels[pwmChannelIndex]);
			pwmChannelIndex++;
		}
	}
	FastLED.setBrightness(rlcSettings.SPILedGlobalBrightness);
}

/*
void DefaultLight() {
	if (!rlcLedController->IsInitialized)
	{
		return;
	}
	if(rlcLedController->defaultLightOn == false) {
		defaultLightTicker.detach();
	};

	FastLED.showColor(CRGB::White);
	for(size_t i = 0; i < rlcLedController->PWMChannelCount; i++) {
		PinWrite(rlcLedController->PWMChannels[i], ANALOG_HIGH);
	}
}*/

/*
void ClearLight()
{
	if(!rlcLedController->IsInitialized) {
		return;
	}
	FastLED.clear(true);
	for(size_t i = 0; i < rlcLedController->PWMChannelCount; i++) {
		PinWrite(rlcLedController->PWMChannels[i], ANALOG_LOW);
	}	
}*/

void WaitingTimeSynchronization(IPAddress& ipAddress, uint16_t port)
{
	FastLED.showColor(CRGB::Green);

	ISntpClient* sntpClient = new SntpClient(ipAddress, port);
	TimeSynchronization timeSync = TimeSynchronization(*sntpClient, *logger);
	timeSync.SynchronizeMultiple(10, 20000);
}

void CheckWiredStart()
{
	if(wiredStarted) {
		return;
	}
	// считываем значения с входа кнопки  
	wiredButtonState = GetButtonState();

	if(wiredButtonState == HIGH && !wiredStartIsInitialized) {
		logger->Print("Programm initialized");
		wiredStartIsInitialized = true;
	}

	// проверяем нажата ли кнопка
	// если нажата, то buttonState будет LOW:
	if(wiredButtonState == LOW && wiredStartIsInitialized && !wiredStarted) {
		logger->Print("Programm started");
		rlcLedController->Play(0, TimeNow() + (uint32_t)4000);
		wiredStarted = true;
		//конец последовательности воспроизведения
		wiredStartIsInitialized = false;
	}
}
/*
void StartDefaultLightTicker()
{
	if(!rlcLedController->IsInitialized) {
		return;
	}
	rlcLedController->defaultLightOn = true;
	defaultLightTicker.attach_ms(50, DefaultLight);
}*/

void WiredSetup()
{
	// считываем значения с входа кнопки  
	wiredButtonState = GetButtonState();
	wiredMode = wiredButtonState == HIGH;
}

void WirelessSetup()
{
	messageFactory = RLCMessageFactory(rlcSettings.ProjectKey, rlcSettings.PlateNumber);

	WiFiConnect();
	WaitingServerIPAddress();
	WaitingTimeSynchronization(serverIP, NTP_PORT);
	StartReceiveFromRLCServer();

	//tickerBattery.attach_ms(5000, BatteryChargeHandler);
}

void setup()
{	
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
	for(size_t i = 0; i < rlcSettings.PinsCount; i++) {
		logger->Print("Pin: ", ToString(rlcSettings.Pins[i].Type), false);
		logger->Print("", rlcSettings.Pins[i].Number, false);
		logger->Print("-", rlcSettings.Pins[i].LedCount);
	}
	logger->Print("DefaultLightMode: ", rlcSettings.DefaultLightOn);
	logger->Print("SPILedGlobalBrightness: ", rlcSettings.SPILedGlobalBrightness);

	OpenCyclogrammFile();
	rlcLedController->Initialize(FastLEDInitialization, cyclogrammFile, OpenCyclogrammFile);

	IsDigitalOutput = rlcSettings.IsDigitalPWMSignal;
	InvertedOutput = rlcSettings.InvertedPWMSignal;
	WiredSetup();

	
	logger->Print("--------------------");
	if(wiredMode) {
		logger->Print("Wired mode enabled");
	}
	else {
		logger->Print("Wireless mode enabled");
		WirelessSetup();
	}
	//StartDefaultLightTicker();
}

void loop(void) {
	if(wiredMode) {
		CheckWiredStart();
	}
	else {
		SendBatteryCharge();
		ReadMessagesFromServer();
	}
	
	rlcLedController->Show();
}