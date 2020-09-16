#pragma once
#include "FastLED.h"
#include <SD.h>
#include "../Service/PinController.h"
#include "../RLCMessage/RLCEnums.h"
#include <TimeNow.h>
#include <ILogger.h>

typedef void (*FastLedInitialization)();
typedef void (*ReopenFile)();

enum class LEDControllerStatuses {
	Stoped,
	Played,
	Paused
};

inline const char* ToString(LEDControllerStatuses ledControlledStatus)
{
	switch(ledControlledStatus)
	{
	case LEDControllerStatuses::Stoped:   return "Stoped";
	case LEDControllerStatuses::Played:   return "Played";
	case LEDControllerStatuses::Paused:   return "Paused";
	default:      return "None";
	}
}

class RLCLedController
{
public:
	unsigned int LedCount;
	CRGB* LedArray;
	unsigned int PWMChannelCount;
	int* PWMChannels;
	LEDControllerStatuses Status;
	boolean IsInitialized;
	unsigned int FrameBytes;

	//����� ������ �����, �� (�� ��������� 50 ��)
	uint32_t frameTime = 50;

	RLCLedController(ILogger& loggerRef);
	~RLCLedController();

	void Initialize(FastLedInitialization initializerMethod, File &cyclogrammFile, ReopenFile reopenFileMethod);

	bool defaultLightOn;

	void Play();
	void PlayFrom(Time &launchFromTime, Time &lauchTime);
	void Rewind(Time &launchFromTime, Time &lauchTime, ClientStateEnum &clientState);
	void Stop();
	void Pause();
	void Show();
	
	void NextFrame();

private:
	ILogger& logger;
	ReopenFile reopenFileMethod;
	File cyclogrammFile;
	boolean showNext = false;
	boolean cyclogrammEnded = false;

	// ����� ���� � �������� ����� ������ ���������������
	//unsigned long launchFrame;

	// ������� ������� ����� (���������� ����� �����, �� ������ ����� � �����!)
	uint32_t framePosition = 0;

	// ������������� ������� � ����� �� ����������� ����
	void SetPosition(uint64_t framePosition);

	// ���������� ������� � ����� �� ������
	void ResetPosition();

	// ������������� ����� ����������� � �������� ���������� ������ ���������������
	bool SetLaunchTime(Time &launchFromTime, Time &lauchTime);
	
	// ��������� ������� ����� � ����� ��� �������� �����
	inline uint64_t GetFrameBytePosition(uint64_t framePos);

	// ��������� �������� ������� ��������������� �����������
	Time GetCurrentPlayTime();

	// ��������� ����� �����������
	Time GetCyclogrammLength();

	uint32_t GetFrameFromTime(Time& time);

	//���������� ������ ��� ���������� �����
	void NextFrameDataPreparation();

	//��������� ����������� ����� �����������. ���� �� ���� �� ��������, �������� ����������� ���.
	bool CheckFileAvailability();
};

