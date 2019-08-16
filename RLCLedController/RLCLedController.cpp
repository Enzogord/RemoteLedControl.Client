#include "RLCLedController.h"

RLCLedController::RLCLedController()
{
}

RLCLedController::RLCLedController(SyncTime &syncTime)
{
	IsInitialized = false;
	Status = LEDControllerStatuses::Stoped;
	//launchController = LaunchController(syncTime, *this);
}

RLCLedController::~RLCLedController()
{
}

void RLCLedController::Initialize(FastLedInitialization initializerMethod, File &cyclogrammFile, ReopenFile reopenFileMethod)
{
	RLCLedController::reopenFileMethod = reopenFileMethod;
	RLCLedController::cyclogrammFile = cyclogrammFile;
	initializerMethod();
	FrameBytes = (LedCount * 3) + *PWMChannels;
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

//запуск циклограммы с нужного момента времени циклограммы. ¬ заданное врем€.
//launchFromTime - врем€ в циклограмме с которого должно начатьс€ воспроизведение
//lauchTime - реальное врем€ в которое должен произойти запуск
void RLCLedController::PlayFrom(Time &launchFromTime, Time &lauchTime)
{
	if(!IsInitialized)
	{
		return;
	}
	SetLaunchTime(launchFromTime, lauchTime);
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
	ResetPosition();
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
		if(cyclogrammFile.available() >= FrameBytes)
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

		if(cyclogrammFile && cyclogrammFile.available() <= FrameBytes)
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
			Serial.println("!!!!!!!!! FATAL ERROR. FRAME OVERLAY!!!!!!!!!!!, NEED INCREASE FRAME TIME OR REDUCE DATA VOLUME");
		}
		showNext = true;
		framePosition += FrameBytes;
	}
}

void RLCLedController::SetPosition(uint64_t framePosition)
{	
	if(!IsInitialized)
	{
		return;
	}
	uint64_t frameBytePosition = GetFrameBytePosition(framePosition);
	if(frameBytePosition >= (cyclogrammFile.size() - FrameBytes))
	{
		Stop();
		Serial.println("Position out of range cyclogramm size or set position to last frame. LED controller stoped.");
	}
	cyclogrammFile.seek(frameBytePosition);
	RLCLedController::framePosition = framePosition;
}

void RLCLedController::ResetPosition()
{
	cyclogrammFile.seek(0);
	framePosition = 0;
}

void RLCLedController::SetLaunchTime(Time &launchFromTime, Time &lauchTime)
{
	Time now = syncTime.Now();
	Time correctedLaunchTime = (now - lauchTime) + launchFromTime;
	Time currentPlayTime = GetCurrentPlayTime();

	if(correctedLaunchTime < currentPlayTime)
	{
		correctedLaunchTime += (currentPlayTime - correctedLaunchTime);
	}
	uint64_t launchFrame = correctedLaunchTime.TotalMicroseconds / frameTime;
	SetPosition(launchFrame);
}

inline uint64_t RLCLedController::GetFrameBytePosition(uint64_t framePos)
{
	return (uint64_t)framePos * (uint64_t)FrameBytes;
}

Time RLCLedController::GetCurrentPlayTime()
{
	return Time((uint64_t)framePosition * (uint64_t)frameTime * (uint64_t)1000);
}