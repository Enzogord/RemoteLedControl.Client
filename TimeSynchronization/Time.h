#pragma once
#include <arduino.h>

struct Time {
	uint32_t Seconds;
	uint32_t Microseconds;
public:
	Time();
	Time(uint32_t seconds, uint32_t fractions);
	Time(uint8_t sourceArray[], int startIndex);
	~Time();

	void AddMicroseconds(uint32_t microseconds);

	//Set 4 bytes of seconds to array at index position
	void SetSecondsTo(uint8_t destArray[], int startIndex);

	//Set 4 bytes of second fractions to array at index position
	void SetSecondFractionsTo(uint8_t destArray[], int startIndex);
};

