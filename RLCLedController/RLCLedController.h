#pragma once
#include "FastLED.h"
#include <SD.h>
#include "../Service/PinController.h"

typedef void (*FastLedInitialization)();
typedef void (*ReopenFile)();

enum class LEDControllerStatuses {
	Played,
	Stoped,
	Paused
};

inline const char* ToString(LEDControllerStatuses ledControlledStatus)
{
	switch(ledControlledStatus)
	{
	case LEDControllerStatuses::Played:   return "Played";
	case LEDControllerStatuses::Stoped:   return "Stoped";
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
	//время одного кадра, мс (по умолчанию 50 мс)
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
	

	unsigned int frameBytes;

	boolean showNext = false;	
	unsigned long framePosition = 0;
};

