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
	pwmValuesBuffer = new uint8_t[PWMChannelCount];
	IsInitialized = true;
}


void RLCLedController::Start()
{
	nextFramePlayTime = TimeNow();
	Stop();
}

#pragma region Control


void RLCLedController::Play(uint32_t frame, Time frameStartTime)
{
	if (!IsInitialized) {
		return;
	}

	isPlayScheduled = true;
	scheduledFrame = frame;
	scheduledPlayTime = frameStartTime;
	logger.Print("Scheduled play. ", false);
	logger.Print("frame: ", scheduledFrame, false);
	logger.Print(" on time: ", scheduledPlayTime.GetSeconds(), false);
	logger.Print("sec, ", scheduledPlayTime.GetMicroseconds(), false);
	logger.Print("us");
}

void RLCLedController::Stop()
{
	InternalStop();
	isPlayScheduled = false;
}

void RLCLedController::InternalStop()
{
	if (!IsInitialized) {
		return;
	}
	ResetPosition();
	FrameDataPreparation();

	Status = LEDControllerStatuses::Stoped;
	logger.Print("LED controller stoped.");
}

inline void RLCLedController::ResetPosition()
{
	SetPosition(0);
}

void RLCLedController::TestConnection(Time startFrame)
{
	if(Status != LEDControllerStatuses::Stoped) {
		return;
	}
	Time now = TimeNow();
	if(now < startFrame) {
		testLightFrames = 10;
	}
	else {
		uint32_t timeDiff = (uint32_t)(now.TotalMicroseconds - startFrame.TotalMicroseconds);
		uint32_t framesLeft = timeDiff / 50000;
		if(framesLeft >= 10) {
			return;
		}
		testLightFrames = 10 - framesLeft;
	}
}

void RLCLedController::Pause(uint32_t frame)
{
	if (!IsInitialized) {
		return;
	}
	SetPosition(frame);
	FrameDataPreparation();
	Status = LEDControllerStatuses::Paused;
	isPlayScheduled = false;

	logger.Print("LED controller paused.");
	logger.Print("frame: ", scheduledFrame);
}


#pragma endregion

#pragma region Scheduler

void RLCLedController::ScheduledFramePreparation()
{
	if (!isPlayScheduled) {
		return;
	}
	Time now = TimeNow();
	uint32_t startFrame;
	Time playTime;
	if (scheduledPlayTime > now) {
		Time remainingTime = scheduledPlayTime - now;
		if (remainingTime.TotalMicroseconds < 50000) {
			startFrame = scheduledFrame;
			playTime = scheduledPlayTime;
		}
		else {
			return;
		}
	}
	else {
		Time overTime = now - scheduledPlayTime;
		uint32_t overFrames = ((uint32_t)overTime.TotalMicroseconds) / (uint32_t)50000;
		startFrame = scheduledFrame + overFrames + 1;
		playTime = scheduledPlayTime + ((overFrames + 1) * (uint32_t)50000);
	}

	SetPosition(startFrame);
	nextFramePlayTime = playTime;
	Status = LEDControllerStatuses::Played;
	logger.Print("LED controller played.");
	isPlayScheduled = false;
}

#pragma endregion


void RLCLedController::Show()
{	
	if(!CanShowNextFrame()) {
		return;
	}
	ShowFrame();
	ScheduledFramePreparation();
	if(Status == LEDControllerStatuses::Played) {
		NextFrame();
	}
}

bool RLCLedController::CanShowNextFrame() {
	if(!IsInitialized) {
		return false;
	}

	Time now = TimeNow();
	if(now < nextFramePlayTime) {
		Time timeUntilNextFrame = nextFramePlayTime - now;
		if(timeUntilNextFrame.TotalMicroseconds < 5000) {
			delayMicroseconds(timeUntilNextFrame.TotalMicroseconds);
			logger.Print("Play frame: ", framePosition, false);
			logger.Print(" on time: ", nextFramePlayTime.GetSeconds(), false);
			logger.Print("sec, ", nextFramePlayTime.GetMicroseconds(), false);
			logger.Print("us");
		}
		else {
			return false;
		}
	}
	nextFramePlayTime += (uint32_t)50000;

	return true;
}

void RLCLedController::ShowFrame()
{
	if(Status == LEDControllerStatuses::Stoped) {
		if(testLightFrames) {
			testLightFrames--;
			ShowTestFrames();
		}
		else {
			Clear();
		}
	}
	else {
		FastLED.show();
		for (unsigned int i = 0; i < PWMChannelCount; i++) {
			PinWrite(PWMChannels[i], pwmValuesBuffer[i]);
		}
	}
}

void RLCLedController::ShowTestFrames() {
	FastLED.showColor(CRGB::GhostWhite);
	for (unsigned int i = 0; i < PWMChannelCount; i++) {
		PinWrite(PWMChannels[i], HIGH);
	}
}

void RLCLedController::Clear()
{
	FastLED.clear(true);
	for (unsigned int i = 0; i < PWMChannelCount; i++) {
		PinWrite(PWMChannels[i], LOW);
	}
}

void RLCLedController::NextFrame()
{
	framePosition++;
	FrameDataPreparation();
}

void RLCLedController::FrameDataPreparation() {
	logger.Print("cyclogrammFile.available(): ", cyclogrammFile.available());
	if(!CheckFileAvailability() || cyclogrammFile.available() < FrameBytes) {
		InternalStop();
		return;
	}

	for (unsigned int i = 0; i < LedCount; i++) {
		LedArray[i].r = cyclogrammFile.read();
		LedArray[i].g = cyclogrammFile.read();
		LedArray[i].b = cyclogrammFile.read();
	}

	for (unsigned int i = 0; i < PWMChannelCount; i++) {
		uint8_t pwmOutput = cyclogrammFile.read();
		pwmValuesBuffer[i] = pwmOutput;
	}
}

bool RLCLedController::CheckFileAvailability()
{
	if (cyclogrammFile) {
		return true;
	}
	else {
		reopenFileMethod();
	}

	if (!cyclogrammFile) {
		return false;
	}

	SetPosition(framePosition);
	return true;
}

void RLCLedController::SetPosition(uint64_t framePosition)
{
	if(!IsInitialized) {
		return;
	}
	uint64_t frameBytePosition = GetFrameBytePosition(framePosition);
	if(frameBytePosition >= (cyclogrammFile.size() - FrameBytes)) {
		InternalStop();

		logger.Print("IsInitialized: ", IsInitialized);
		logger.Print("framePosition: ", framePosition);
		logger.Print("cyclogrammFile.size(): ", cyclogrammFile.size());
		logger.Print("frameBytePosition: ", frameBytePosition);
		logger.Print("Position out of range cyclogramm size or set position to last frame. LED controller stoped.");
	}
	cyclogrammFile.seek(frameBytePosition);	
	RLCLedController::framePosition = framePosition;
}

inline uint64_t RLCLedController::GetFrameBytePosition(uint64_t framePos)
{
	return (uint64_t)framePos * (uint64_t)FrameBytes;
}
