#pragma once
#include "FastLED.h"
#include <SD.h>

typedef void (*FastLedInitialization)(CRGB* ledArray, unsigned int &ledCount);
typedef void (*ReopenFile)();

enum class LEDControllerStatuses {
	Played,
	Stoped,
	Paused
};

class RLCLedController
{
public:
	LEDControllerStatuses Status;
	boolean IsInitialized;
	//����� ������ �����, �� (�� ��������� 50 ��)
	unsigned int FrameTime = 50;

	RLCLedController();
	~RLCLedController();

	void Initialize(FastLedInitialization initializerMethod, File &cyclogrammFile, ReopenFile reopenFileMethod);

	void Play();
	void Stop();
	void Pause();
	void Show();
	void NextFrame();
	void SetPosition(unsigned long framePosition);

private:
	ReopenFile reopenFileMethod;
	File cyclogrammFile;
	CRGB *ledArray;

	unsigned int ledCount;
	unsigned int frameBytes;

	boolean showNext = false;	
	unsigned long framePosition = 0;
};

