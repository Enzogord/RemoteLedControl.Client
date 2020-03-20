#include "RLCLedController.h"

RLCLedController::RLCLedController(ILogger& loggerRef)
	: logger(loggerRef)
{
	IsInitialized = false;
	Status = LEDControllerStatuses::Stoped;
}

RLCLedController::~RLCLedController()
{
}

void RLCLedController::Initialize(FastLedInitialization initializerMethod, File& cyclogrammFile, ReopenFile reopenFileMethod)
{
	RLCLedController::reopenFileMethod = reopenFileMethod;
	RLCLedController::cyclogrammFile = cyclogrammFile;
	initializerMethod();

	FrameBytes = (LedCount * 3) + PWMChannelCount;
	IsInitialized = true;
}

void RLCLedController::Play()
{
	if(!IsInitialized) {
		return;
	}
	Status = LEDControllerStatuses::Played;
	logger.Print("LED controller start play.");
}

void RLCLedController::Stop()
{
	if(!IsInitialized) {
		return;
	}
	FastLED.clear(true);
	for(unsigned int i = 0; i < PWMChannelCount; i++) {
		PinWrite(PWMChannels[i], LOW);
	}
	ResetPosition();
	Status = LEDControllerStatuses::Stoped;
	logger.Print("LED controller stoped.");
}

void RLCLedController::Pause()
{
	if(!IsInitialized) {
		return;
	}
	Status = LEDControllerStatuses::Paused;
	logger.Print("LED controller paused.");
}

void RLCLedController::Show()
{
	if(!IsInitialized) {
		return;
	}
	if(!cyclogrammFile) {
		reopenFileMethod();
		if(cyclogrammFile && cyclogrammFile.available()) {
			SetPosition(framePosition);
		}
	}
	if(showNext) {
		if(cyclogrammFile.available() >= FrameBytes) {
			unsigned long a1 = millis();
			for(unsigned int i = 0; i < LedCount; i++) {
				LedArray[i].r = cyclogrammFile.read();
				LedArray[i].g = cyclogrammFile.read();
				LedArray[i].b = cyclogrammFile.read();
			}

			for(unsigned int i = 0; i < PWMChannelCount; i++) {
				uint8_t pwmOutput = cyclogrammFile.read();
				PinWrite(PWMChannels[i], pwmOutput);
			}
			FastLED.show();
		}

		if(cyclogrammFile && cyclogrammFile.available() <= FrameBytes) {
			Stop();
			logger.Print("Cyclogramm ended. LED controller stoped.");
		}

		showNext = false;
	}
}

void RLCLedController::NextFrame()
{
	if(!IsInitialized) {
		return;
	}
	if(Status == LEDControllerStatuses::Played) {
		if(showNext) {
			logger.Print("!!!!!!!!! FATAL ERROR. FRAME OVERLAY!!!!!!!!!!!, NEED INCREASE FRAME TIME OR REDUCE DATA VOLUME");
		}
		showNext = true;
		framePosition++;
		/*
		Time now = TimeNow();
		logger.Print("Current frame: ", false); logger.Print("", framePosition, false); logger.Print(", ", false);
		logger.Print("", now.GetSeconds(), false); logger.Print("sec, ", false); logger.Print("", now.GetMicroseconds(), false); logger.Print("us");
		Time curPlay = GetCurrentPlayTime();
		logger.Print("", curPlay.GetSeconds(), false); logger.Print("sec, ", false); logger.Print("", curPlay.GetMicroseconds(), false); logger.Print("us");
		*/
	}
}

void RLCLedController::SetPosition(uint64_t framePosition)
{
	if(!IsInitialized) {
		return;
	}
	uint64_t frameBytePosition = GetFrameBytePosition(framePosition);
	if(frameBytePosition >= (cyclogrammFile.size() - FrameBytes)) {
		Stop();
		logger.Print("Position out of range cyclogramm size or set position to last frame. LED controller stoped.");
	}
	cyclogrammFile.seek(frameBytePosition);
	RLCLedController::framePosition = framePosition;
}

void RLCLedController::ResetPosition()
{
	cyclogrammFile.seek(0);
	framePosition = 0;
}

inline uint64_t RLCLedController::GetFrameBytePosition(uint64_t framePos)
{
	return (uint64_t)framePos * (uint64_t)FrameBytes;
}


#pragma region Time dependent

//запуск циклограммы с нужного момента времени циклограммы. ¬ заданное врем€.
//launchFromTime - врем€ в циклограмме с которого должно начатьс€ воспроизведение
//lauchTime - реальное врем€ в которое должен произойти запуск
void RLCLedController::PlayFrom(Time& launchFromTime, Time& lauchTime)
{
	if(!IsInitialized) {
		return;
	}
	if(SetLaunchTime(launchFromTime, lauchTime)) {
		Status = LEDControllerStatuses::Played;
		logger.Print("LED controller start play.");
	}
}

void RLCLedController::Rewind(Time& launchFromTime, Time& lauchTime, ClientStateEnum& clientState)
{
	if(!IsInitialized) {
		return;
	}
	if(SetLaunchTime(launchFromTime, lauchTime)) {
		logger.Print("Received status: ", false); logger.Print(ToString(clientState));
		switch(clientState) {
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

bool RLCLedController::SetLaunchTime(Time& launchFromTime, Time& sendTime)
{
	Time now = TimeNow();

	Time cyclogrammLength = GetCyclogrammLength();
	uint32_t framesCount = (cyclogrammFile.size() / FrameBytes);
	logger.Print("Time now: ", false); logger.Print("", now.GetSeconds(), false); logger.Print("sec, ", false); logger.Print("", now.GetMicroseconds(), false); logger.Print("us");
	logger.Print("Send time: ", false); logger.Print("", sendTime.GetSeconds(), false); logger.Print("sec, ", false); logger.Print("", sendTime.GetMicroseconds(), false); logger.Print("us");
	logger.Print("Launch from time: ", false); logger.Print("", launchFromTime.GetSeconds(), false); logger.Print("sec, ", false); logger.Print("", launchFromTime.GetMicroseconds(), false); logger.Print("us");

	uint32_t targetFrame = GetFrameFromTime(launchFromTime);
	logger.Print("targetFrame: ", targetFrame);
	uint32_t correctedFrame;
	Time corTime;
	if(now > sendTime) {
		Time delta = now - sendTime;
		correctedFrame = targetFrame + GetFrameFromTime(delta);
	}
	else {
		Time delta = sendTime - now;
		int32_t cf = (int32_t)targetFrame - (int32_t)GetFrameFromTime(delta);
		if(cf < 0) {
			cf = 0;
		}
		correctedFrame = cf;
	}

	correctedFrame++;

	if(correctedFrame > framesCount) {
		logger.Print("out of cyclogramm!!!!!!!!!!!");
		return false;
	}
	logger.Print("correctedFrame: ", correctedFrame);
	SetPosition(correctedFrame);
	return true;
}

uint32_t RLCLedController::GetFrameFromTime(Time& time)
{
	return time.TotalMicroseconds / (uint32_t)1000 / frameTime;
}

Time RLCLedController::GetCurrentPlayTime()
{
	return Time((uint64_t)framePosition * (uint64_t)frameTime * (uint64_t)1000);
}

Time RLCLedController::GetCyclogrammLength()
{
	return Time((cyclogrammFile.size() / FrameBytes) * frameTime * 1000);
}

#pragma endregion Time dependent
