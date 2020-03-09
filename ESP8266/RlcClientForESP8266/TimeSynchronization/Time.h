#pragma once
#include <arduino.h>

struct Time {
	/*uint32_t Seconds;
	uint32_t Microseconds;*/
public:
	Time();
	Time(int64_t totalMicroseconds);
	Time(uint32_t seconds, uint32_t fractions);
	Time(uint8_t sourceArray[], int startIndex);
	~Time();

	int64_t TotalMicroseconds;

	uint32_t GetSeconds();
	uint32_t GetMicroseconds();

	//void AddMicroseconds(uint32_t microseconds);

	//Set 4 bytes of seconds to array at index position
	void SetSecondsTo(uint8_t destArray[], int startIndex);

	//Set 4 bytes of second fractions to array at index position
	void SetSecondFractionsTo(uint8_t destArray[], int startIndex);	

	/*inline bool operator==(const Time& comparedtime) { return Seconds == comparedtime.Seconds && Microseconds == comparedtime.Microseconds; }
	inline bool operator!=(const Time& comparedtime) { return !(*this == comparedtime); }
	inline bool operator< (const Time& comparedtime) { return Seconds < comparedtime.Seconds || (Seconds == comparedtime.Seconds && Microseconds < comparedtime.Microseconds); }
	inline bool operator> (const Time& comparedtime) { return Seconds > comparedtime.Seconds || (Seconds == comparedtime.Seconds && Microseconds > comparedtime.Microseconds); }
	inline bool operator<=(const Time& comparedtime) { return Seconds < comparedtime.Seconds || (Seconds == comparedtime.Seconds && Microseconds < comparedtime.Microseconds) || *this == comparedtime; }
	inline bool operator>=(const Time& comparedtime) { return Seconds > comparedtime.Seconds || (Seconds == comparedtime.Seconds && Microseconds > comparedtime.Microseconds) || *this == comparedtime; }*/
	inline void operator+=(const Time& rhs) { TotalMicroseconds += rhs.TotalMicroseconds; }
	inline void operator-=(const Time& rhs) { TotalMicroseconds -= rhs.TotalMicroseconds; }
	inline void operator*=(const Time& rhs) { TotalMicroseconds *= rhs.TotalMicroseconds; }
	inline void operator/=(const Time& rhs) { TotalMicroseconds /= rhs.TotalMicroseconds; }
	inline void operator%=(const Time& rhs) { TotalMicroseconds %= rhs.TotalMicroseconds; }

	inline void operator+=(const uint32_t& rhs) { TotalMicroseconds += (uint64_t)rhs; }
	inline void operator-=(const uint32_t& rhs) { TotalMicroseconds -= (uint64_t)rhs; }
	inline void operator*=(const uint32_t& rhs) { TotalMicroseconds *= (uint64_t)rhs; }
	inline void operator/=(const uint32_t& rhs) { TotalMicroseconds /= (uint64_t)rhs; }
	inline void operator%=(const uint32_t& rhs) { TotalMicroseconds %= (uint64_t)rhs; }

	inline void operator+=(const uint64_t& rhs) { TotalMicroseconds += rhs; }
	inline void operator-=(const uint64_t& rhs) { TotalMicroseconds -= rhs; }
	inline void operator*=(const uint64_t& rhs) { TotalMicroseconds *= rhs; }
	inline void operator/=(const uint64_t& rhs) { TotalMicroseconds /= rhs; }
	inline void operator%=(const uint64_t& rhs) { TotalMicroseconds %= rhs; }

	inline bool operator==(const Time& rhs) { return TotalMicroseconds == rhs.TotalMicroseconds; }
	inline bool operator!=(const Time& rhs) { return TotalMicroseconds != rhs.TotalMicroseconds; }
	inline bool operator<(const Time& rhs) { return TotalMicroseconds < rhs.TotalMicroseconds; }
	inline bool operator>(const Time& rhs) { return TotalMicroseconds > rhs.TotalMicroseconds; }
	inline bool operator<=(const Time& rhs) { return TotalMicroseconds <= rhs.TotalMicroseconds; }
	inline bool operator>=(const Time& rhs) { return TotalMicroseconds >= rhs.TotalMicroseconds; }
};

inline Time operator+(const Time& lhs, const Time& rhs) { return Time(lhs.TotalMicroseconds + rhs.TotalMicroseconds); }
inline Time operator-(const Time& lhs, const Time& rhs) { return Time(lhs.TotalMicroseconds - rhs.TotalMicroseconds); }
inline Time operator*(const Time& lhs, const Time& rhs) { return Time(lhs.TotalMicroseconds * rhs.TotalMicroseconds); }
inline Time operator/(const Time& lhs, const Time& rhs) { return Time(lhs.TotalMicroseconds / rhs.TotalMicroseconds); }
inline Time operator%(const Time& lhs, const Time& rhs) { return Time(lhs.TotalMicroseconds % rhs.TotalMicroseconds); }

inline Time operator+(const Time& lhs, const uint32_t& rhs) { return Time(lhs.TotalMicroseconds + (uint64_t)rhs); }
inline Time operator-(const Time& lhs, const uint32_t& rhs) { return Time(lhs.TotalMicroseconds - (uint64_t)rhs); }
inline Time operator*(const Time& lhs, const uint32_t& rhs) { return Time(lhs.TotalMicroseconds * (uint64_t)rhs); }
inline Time operator/(const Time& lhs, const uint32_t& rhs) { return Time(lhs.TotalMicroseconds / (uint64_t)rhs); }
inline Time operator%(const Time& lhs, const uint32_t& rhs) { return Time(lhs.TotalMicroseconds % (uint64_t)rhs); }

inline Time operator+(const Time& lhs, const uint64_t& rhs) { return Time(lhs.TotalMicroseconds + rhs); }
inline Time operator-(const Time& lhs, const uint64_t& rhs) { return Time(lhs.TotalMicroseconds - rhs); }
inline Time operator*(const Time& lhs, const uint64_t& rhs) { return Time(lhs.TotalMicroseconds * rhs); }
inline Time operator/(const Time& lhs, const uint64_t& rhs) { return Time(lhs.TotalMicroseconds / rhs); }
inline Time operator%(const Time& lhs, const uint64_t& rhs) { return Time(lhs.TotalMicroseconds % rhs); }



/*
inline Time operator-(const Time& lhs, const Time& rhs)
{
	Time resultTime;
	resultTime.Seconds = lhs.Seconds + rhs.Seconds;
	resultTime.Microseconds = lhs.Microseconds + rhs.Microseconds;
	if(resultTime.Microseconds >= 1000000)
	{
		resultTime.Microseconds -= 1000000;
		resultTime.Seconds++;
	}
	return resultTime;

	Time result = a;
	int32_t micro = a.Microseconds - b.Microseconds;
	if(micro < 0)
	{
		result.Seconds--;
		result.Microseconds = 1000000 + micro;
	}
	else
	{
		result.Microseconds = micro;

	}

	result.Seconds -= b.Seconds;
	return result;
}*/