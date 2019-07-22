#include "Time.h"

Time::Time()
{
}

Time::Time(uint32_t seconds, uint32_t microseconds)
{
	Seconds = seconds;
	Microseconds = microseconds;
}

Time::Time(uint8_t sourceArray[], int startIndex)
{
	Seconds = (unsigned long)sourceArray[startIndex] << 24;
	Seconds |= (unsigned long)sourceArray[startIndex+1] << 16;
	Seconds |= (unsigned long)sourceArray[startIndex+2] << 8;
	Seconds |= (unsigned long)sourceArray[startIndex+3];

	unsigned long fractions;
	fractions = (unsigned long)sourceArray[startIndex] << 24;
	fractions |= (unsigned long)sourceArray[startIndex+1] << 16;
	fractions |= (unsigned long)sourceArray[startIndex+1] << 8;
	fractions |= (unsigned long)sourceArray[startIndex+1];
	uint32_t microseconds = ((unsigned long long)fractions) * 1000000UL / 0x100000000L;

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

void Time::SetSecondsTo(uint8_t destArray[], int startIndex)
{
	destArray[startIndex] = Seconds >> 24;
	destArray[startIndex+1] = Seconds >> 16;
	destArray[startIndex+2] = Seconds >> 8;
	destArray[startIndex+3] = Seconds;
}

void Time::SetSecondFractionsTo(uint8_t destArray[], int startIndex)
{
	uint32_t currentFractions = ((unsigned long long)Microseconds) * 0x100000000L / 1000000UL;
	destArray[startIndex] = currentFractions >> 24;
	destArray[startIndex+1] = currentFractions >> 16;
	destArray[startIndex+2] = currentFractions >> 8;
	destArray[startIndex+3] = currentFractions;
}

