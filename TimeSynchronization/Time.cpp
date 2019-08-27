#include "Time.h"

Time::Time()
{
}

Time::Time(int64_t totalMicroseconds)
{
	TotalMicroseconds = totalMicroseconds;
}

Time::Time(uint32_t seconds, uint32_t microseconds)
{
	TotalMicroseconds = (uint64_t)(((uint64_t)seconds) * 1000000ULL);
	TotalMicroseconds += (uint64_t)microseconds;
}

Time::Time(uint8_t sourceArray[], int startIndex)
{
	uint32_t seconds;
	seconds = (unsigned long)sourceArray[startIndex] << 24;
	seconds |= (unsigned long)sourceArray[startIndex+1] << 16;
	seconds |= (unsigned long)sourceArray[startIndex+2] << 8;
	seconds |= (unsigned long)sourceArray[startIndex+3];

	TotalMicroseconds = (uint64_t)(((uint64_t)seconds) * 1000000ULL);
	uint32_t fractions;
	fractions = (uint32_t)sourceArray[startIndex+4] << 24;
	fractions |= (uint32_t)sourceArray[startIndex+5] << 16;
	fractions |= (uint32_t)sourceArray[startIndex+6] << 8;
	fractions |= (uint32_t)sourceArray[startIndex+7];
	uint64_t microseconds = (uint64_t)((uint64_t)fractions) * 1000000ULL / 0x100000000LL;
	TotalMicroseconds += microseconds;
}

Time::~Time()
{
}

uint32_t Time::GetSeconds()
{
	return (uint32_t)(TotalMicroseconds / 1000000);
}

uint32_t Time::GetMicroseconds()
{
	return (uint32_t)(TotalMicroseconds % 1000000);
}

void Time::SetSecondsTo(uint8_t destArray[], int startIndex)
{
	uint32_t seconds = GetSeconds();
	destArray[startIndex] = seconds >> 24;
	destArray[startIndex+1] = seconds >> 16;
	destArray[startIndex+2] = seconds >> 8;
	destArray[startIndex+3] = seconds;
}

void Time::SetSecondFractionsTo(uint8_t destArray[], int startIndex)
{
	uint32_t currentFractions = ((unsigned long long)GetMicroseconds()) * 0x100000000L / 1000000UL;
	destArray[startIndex] = currentFractions >> 24;
	destArray[startIndex+1] = currentFractions >> 16;
	destArray[startIndex+2] = currentFractions >> 8;
	destArray[startIndex+3] = currentFractions;
}