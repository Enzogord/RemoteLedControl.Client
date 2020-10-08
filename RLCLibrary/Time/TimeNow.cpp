#include "TimeNow.h"


uint64_t lastTime;
uint64_t lastMicroseconds;
GetCurrentMicros CurrentMicrosHandler;

void SetWorkTimeFunction(GetCurrentMicros handler)
{
	CurrentMicrosHandler = handler;
}

Time TimeNow()
{
	uint64_t deltaMicros = CurrentMicrosHandler() - lastMicroseconds;
	lastMicroseconds += deltaMicros;
	lastTime += deltaMicros;
	return Time(lastTime);
}

void CorrectTime(int64_t timeShift)
{
	TimeNow();
	lastTime += timeShift;
}

void SetTime(int64_t newTime)
{
	lastTime = newTime;
	lastMicroseconds = CurrentMicrosHandler();
}