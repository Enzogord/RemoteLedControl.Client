#include "RLCLedController.h"

RLCLedController::RLCLedController()
{
	IsInitialized = false;
	Status = LEDControllerStatuses::Stoped;
}

RLCLedController::~RLCLedController()
{
}

void RLCLedController::Initialize(FastLedInitialization initializerMethod, File &cyclogrammFile, ReopenFile reopenFileMethod)
{
	RLCLedController::reopenFileMethod = reopenFileMethod;
	initializerMethod(ledArray, ledCount);
	frameBytes = ledCount * 3;
	IsInitialized = true;
}

void RLCLedController::Play()
{
	if(!IsInitialized)
	{
		return;
	}
	Status == LEDControllerStatuses::Played;
	Serial.println("LED controller start play.");
}

void RLCLedController::Stop()
{
	if(!IsInitialized)
	{
		return;
	}
	//—брос счетчиков и позиций в начальные положени€
	cyclogrammFile.seek(0);
	framePosition = 0;

	Status == LEDControllerStatuses::Stoped;
	Serial.println("LED controller stoped.");
}

void RLCLedController::Pause()
{
	if(!IsInitialized)
	{
		return;
	}
	Status == LEDControllerStatuses::Paused;
	Serial.println("LED controller paused.");
}

void RLCLedController::Show()
{
	if(!IsInitialized)
	{
		return;
	}
	if(!cyclogrammFile)
	{
		reopenFileMethod();
		if(cyclogrammFile && cyclogrammFile.available())
		{
			SetPosition(framePosition);
		}
	}
	if(showNext)
	{
		if(cyclogrammFile.available() >= frameBytes)
		{
			for(unsigned int i = 0; i < ledCount; i++)
			{
				ledArray[i].r = cyclogrammFile.read();
				ledArray[i].g = cyclogrammFile.read();
				ledArray[i].b = cyclogrammFile.read();
			}
			FastLED.show();
		}

		if(cyclogrammFile && cyclogrammFile.available() <= frameBytes)
		{
			Stop();
			Serial.println("Cyclogramm ended. LED controller stoped.");
		}
		
		showNext = false;
	}
}

void RLCLedController::NextFrame()
{
	if(!IsInitialized)
	{
		return;
	}
	if(Status == LEDControllerStatuses::Played)
	{
		if(showNext)
		{
			Serial.println("!!!!!!!!! FATAL ERROR. FRAME OVERLAY !!!!!!!!!!!");
		}
		showNext = true;
		framePosition += ledCount * 3;
	}
}

void RLCLedController::SetPosition(unsigned long framePosition)
{	
	if(!IsInitialized)
	{
		return;
	}
	if(framePosition >= (cyclogrammFile.size() - frameBytes))
	{
		Stop();
		Serial.println("Position out of range cyclogramm size or set position to last frame. LED controller stoped.");
	}
	RLCLedController::framePosition = framePosition;
}
