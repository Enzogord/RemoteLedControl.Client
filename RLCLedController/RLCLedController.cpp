#include "RLCLedController.h"

RLCLedController::RLCLedController(SyncTime *syncTime)
{
	IsInitialized = false;
	Status = LEDControllerStatuses::Stoped;
	timeProvider = syncTime;
}

RLCLedController::~RLCLedController()
{
}

void RLCLedController::Initialize(FastLedInitialization initializerMethod, File &cyclogrammFile, ReopenFile reopenFileMethod)
{
	RLCLedController::reopenFileMethod = reopenFileMethod;
	RLCLedController::cyclogrammFile = cyclogrammFile;
	initializerMethod();

	FrameBytes = (LedCount * 3) + PWMChannelCount;
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
	if(SetLaunchTime(launchFromTime, lauchTime)) 
	{
		Status = LEDControllerStatuses::Played;
		Serial.println("LED controller start play.");
	}	
}

void RLCLedController::Rewind(Time &launchFromTime, Time &lauchTime, ClientStateEnum &clientState)
{
	if(!IsInitialized)
	{
		return;
	}
	if(SetLaunchTime(launchFromTime, lauchTime))
	{
		Serial.print("Received status: "); Serial.println(ToString(clientState));
		switch(clientState)
		{
			case ClientStateEnum::Playing:
				Status = LEDControllerStatuses::Played;
				break;
			case ClientStateEnum::Paused:
				Status = LEDControllerStatuses::Paused;
				showNext = true;
				cyclogrammFile.seek(cyclogrammFile.position() - FrameBytes);
				Show();
				break;
			default:
				return;
		}
	}
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

			for(unsigned int i = 0; i < PWMChannelCount; i++)
			{
				uint8_t pwmOutput = cyclogrammFile.read();
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

bool RLCLedController::SetLaunchTime(Time &launchFromTime, Time &launchTime)
{
	Time now = (*timeProvider).Now();
	Time cyclogrammLength = GetCyclogrammLength();
	if(launchTime <= now)
	{
		if(((now - launchTime) + launchFromTime) >= cyclogrammLength)
		{
			return false;
		}
	}
	else
	{
		Time corTime = (now - launchTime) + launchFromTime;
		// корректировка если рассинхронизаци€ произошла так что врем€ отправки станет больше чем врем€ получени€
		// в пределах 1 сек, с учетом возможних задержек на врем€ передачи
		if(corTime <= 0 || corTime >= cyclogrammLength || (launchTime - now).TotalMicroseconds > 1000000)
		{
			return false;
		}
	}

	Time correctedLaunchTime = (now - launchTime) + launchFromTime;
	uint64_t launchFrame = (correctedLaunchTime.TotalMicroseconds / 1000) / frameTime;
	SetPosition(launchFrame);
	return true;
}

inline uint64_t RLCLedController::GetFrameBytePosition(uint64_t framePos)
{
	return (uint64_t)framePos * (uint64_t)FrameBytes;
}

Time RLCLedController::GetCurrentPlayTime()
{
	return Time((uint64_t)framePosition * (uint64_t)frameTime * (uint64_t)1000);
}

Time RLCLedController::GetCyclogrammLength()
{
	return Time((cyclogrammFile.size() / FrameBytes) * frameTime * 1000);
}