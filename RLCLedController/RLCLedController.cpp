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
	RLCLedController::cyclogrammFile = cyclogrammFile;
	initializerMethod();
	frameBytes = LedCount * 3;
	IsInitialized = true;
}

void RLCLedController::Play()
{
	if(!IsInitialized)
	{
		return;
	}
	Status = LEDControllerStatuses::Played;
	Serial.println("LED controller start play.");
}

void RLCLedController::Stop()
{
	if(!IsInitialized)
	{
		return;
	}
	FastLED.clear(true);
	for(unsigned int i = 0; i < PWMChannelCount; i++)
	{
		PinWrite(PWMChannels[i], LOW);
	}
	//—брос счетчиков и позиций в начальные положени€
	cyclogrammFile.seek(0);
	framePosition = 0;

	Status = LEDControllerStatuses::Stoped;
	Serial.println("LED controller stoped.");
}

void RLCLedController::Pause()
{
	if(!IsInitialized)
	{
		return;
	}
	Status = LEDControllerStatuses::Paused;
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
			unsigned long a1 = millis();
			for(unsigned int i = 0; i < LedCount; i++)
			{				
				LedArray[i].r = cyclogrammFile.read();
				LedArray[i].g = cyclogrammFile.read();
				LedArray[i].b = cyclogrammFile.read();
			}

			Serial.print("Frame generation time: "); Serial.println(millis() - a1);
			for(unsigned int i = 0; i < PWMChannelCount; i++)
			{
				uint8_t pwmOutput = cyclogrammFile.read();				
				Serial.print("PWM output: "); Serial.println(pwmOutput);
				PinWrite(PWMChannels[i], pwmOutput);
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
	Serial.print("Status: "); Serial.println(ToString(Status));
	if(Status == LEDControllerStatuses::Played)
	{
		if(showNext)
		{
			Serial.println("!!!!!!!!! FATAL ERROR. FRAME OVERLAY !!!!!!!!!!!");
		}
		Serial.println("New frame");
		showNext = true;
		framePosition += frameBytes;
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
