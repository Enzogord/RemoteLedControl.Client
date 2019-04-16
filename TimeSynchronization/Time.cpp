#include "Time.h"

Time::Time()
{
}

Time::Time(uint32_t seconds, uint32_t microseconds)
{
	Seconds = seconds;
	Microseconds = microseconds;
}

Time::~Time()
{
}

void Time::AddMicroseconds(uint32_t microseconds)
{
	uint32_t totalMicros = Microseconds + microseconds;
	Seconds += totalMicros / 1000000;
	Microseconds = totalMicros % 1000000;
}