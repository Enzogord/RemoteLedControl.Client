#pragma once
#include <arduino.h>

struct Time {
	uint32_t Seconds;
	uint32_t Microseconds;
public:
	Time();
	Time(uint32_t seconds, uint32_t fractions);
	~Time();

	void AddMicroseconds(uint32_t microseconds);
};

