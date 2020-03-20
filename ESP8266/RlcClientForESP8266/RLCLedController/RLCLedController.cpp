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
		framePosition ++;
		Time now = TimeNow();
		Serial.print("Current frame: "); Serial.print(framePosition); Serial.print(", ");
		Serial.print(now.GetSeconds()); Serial.print("sec, "); Serial.print(now.GetMicroseconds()); Serial.print("us");
		Time curPlay = GetCurrentPlayTime();
		Serial.print(curPlay.GetSeconds()); Serial.print("sec, "); Serial.print(curPlay.GetMicroseconds()); Serial.println("us");
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

bool RLCLedController::SetLaunchTime(Time &launchFromTime, Time &sendTime)
{
	Time now = TimeNow();

	Time cyclogrammLength = GetCyclogrammLength();
	uint32_t framesCount =  (cyclogrammFile.size() / FrameBytes);
	//Time currentPlayTime = GetCurrentPlayTime();
	Serial.print("Time now: "); Serial.print(now.GetSeconds()); Serial.print("sec, "); Serial.print(now.GetMicroseconds()); Serial.println("us");
	Serial.print("Send time: "); Serial.print(sendTime.GetSeconds()); Serial.print("sec, "); Serial.print(sendTime.GetMicroseconds()); Serial.println("us");
	Serial.print("Launch from time: "); Serial.print(launchFromTime.GetSeconds()); Serial.print("sec, "); Serial.print(launchFromTime.GetMicroseconds()); Serial.println("us");
	//Serial.print("currentPlayTime"); Serial.println((int)(currentPlayTime.TotalMicroseconds / 1000));
	
	uint32_t targetFrame = GetFrameFromTime(launchFromTime);
	Serial.print("targetFrame: "); Serial.println(targetFrame);
	uint32_t correctedFrame;
	Time corTime;
	if(now > sendTime)
	{
		Time delta = now - sendTime;
		correctedFrame = targetFrame + GetFrameFromTime(delta);
		//corTime = launchFromTime + (now - sendTime);
	}
	else
	{
		Time delta = sendTime - now;
		int32_t cf = (int32_t)targetFrame - (int32_t)GetFrameFromTime(delta);
		if(cf < 0)
		{
			cf = 0;
		}
		correctedFrame = cf;
		//corTime = launchFromTime - (sendTime - now);
	}

	correctedFrame++;

	if (correctedFrame > framesCount)
	{
		Serial.println("out of cyclogramm!!!!!!!!!!!");
		return false;
	}
	Serial.print("correctedFrame: ");Serial.println(correctedFrame);
	SetPosition(correctedFrame);
	return true;
	/*
	Time deltaTime = (now - sendTime);
	if (deltaTime <= 0)
	{
		deltaTime = 0;
	}

	Time corTime;
	if(currentPlayTime >= launchFromTime)
	{
		corTime = launchFromTime + deltaTime;
	}
	else
	{
		Time currentWithDelta = currentPlayTime + deltaTime;
		if(currentWithDelta > launchFromTime)
		{
			corTime = currentWithDelta;
		}
		else
		{
			corTime = launchFromTime;
		}
	}*/

	/*
	--------
	if (corTime >= cyclogrammLength)
	{
		int corTimeMcs = (int)(corTime.TotalMicroseconds / 1000);
		Serial.print("corTime"); Serial.println(corTimeMcs);
		Serial.print("cyclogrammLength"); Serial.println((int)(cyclogrammLength.TotalMicroseconds/1000));
		Serial.println("out of cyclogramm!!!!!!!!!!!");
		return false;
	}
	uint64_t launchFrame = (corTime.TotalMicroseconds / 1000) / frameTime;
	Serial.print("corTime:"); Serial.println((int)(corTime.TotalMicroseconds / 1000));
	Serial.print("launchFrame:"); Serial.println((int)(launchFrame+1));
	SetPosition(launchFrame+1);
	return true;*/
}

uint32_t RLCLedController::GetFrameFromTime(Time &time)
{
	return time.TotalMicroseconds / (uint32_t)1000 / frameTime;
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