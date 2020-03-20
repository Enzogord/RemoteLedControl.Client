#include "SntpFunctions.h"

void SetTimeToArray(Time time, uint8_t* destArray, int startIndex)
{
	uint32_t seconds = time.GetSeconds();
	destArray[startIndex++] = seconds >> 24;
	destArray[startIndex++] = seconds >> 16;
	destArray[startIndex++] = seconds >> 8;
	destArray[startIndex++] = seconds;

	uint32_t currentFractions = ((uint64_t)time.GetMicroseconds()) * 0x100000000L / 1000000UL;
	destArray[startIndex++] = currentFractions >> 24;
	destArray[startIndex++] = currentFractions >> 16;
	destArray[startIndex++] = currentFractions >> 8;
	destArray[startIndex] = currentFractions;
}

Time GetTimeFromArray(uint8_t* sourceArray, int startIndex)
{
	uint32_t seconds;
	seconds = (uint32_t)sourceArray[startIndex++] << 24;
	seconds |= (uint32_t)sourceArray[startIndex++] << 16;
	seconds |= (uint32_t)sourceArray[startIndex++] << 8;
	seconds |= (uint32_t)sourceArray[startIndex++];

	uint64_t totalMicroseconds = (uint64_t)(((uint64_t)seconds) * 1000000ULL);
	uint32_t fractions;
	fractions = (uint32_t)sourceArray[startIndex++] << 24;
	fractions |= (uint32_t)sourceArray[startIndex++] << 16;
	fractions |= (uint32_t)sourceArray[startIndex++] << 8;
	fractions |= (uint32_t)sourceArray[startIndex];
	uint64_t microseconds = (uint64_t)((uint64_t)fractions) * 1000000ULL / 0x100000000LL;
	totalMicroseconds += microseconds;
	return Time(totalMicroseconds);
}
