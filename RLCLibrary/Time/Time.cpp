#include "Time.h"

Time::Time()
{
	TotalMicroseconds = 0;
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

uint32_t Time::GetSeconds()
{
	return (uint32_t)(TotalMicroseconds / 1000000);
}

uint32_t Time::GetMilliseconds()
{
	return (uint32_t)((TotalMicroseconds % 1000000) / 1000);
}

uint32_t Time::GetMicroseconds()
{
	return (uint32_t)(TotalMicroseconds % 1000000);
}
