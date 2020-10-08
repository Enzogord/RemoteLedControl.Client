#pragma once

#include "../Infrastructure/GlobalTypes.h"

class Time
{
public:
	Time();
	Time(int64_t totalMicroseconds);
	Time(uint32_t seconds, uint32_t fractions);

	int64_t TotalMicroseconds;
	
	uint32_t GetSeconds();
	uint32_t GetMilliseconds();
	uint32_t GetMicroseconds();

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


inline Time operator+(const Time& lhs, const Time& rhs) { 
	return Time(lhs.TotalMicroseconds + rhs.TotalMicroseconds); 
}

inline Time operator-(const Time& lhs, const Time& rhs) { 
	return Time(lhs.TotalMicroseconds - rhs.TotalMicroseconds); 
}

inline Time operator*(const Time& lhs, const Time& rhs) { 
	return Time(lhs.TotalMicroseconds * rhs.TotalMicroseconds); 
}

inline Time operator/(const Time& lhs, const Time& rhs) { 
	return Time(lhs.TotalMicroseconds / rhs.TotalMicroseconds); 
}

inline Time operator%(const Time& lhs, const Time& rhs) { 
	return Time(lhs.TotalMicroseconds % rhs.TotalMicroseconds); 
}


inline Time operator+(const Time& lhs, const uint32_t& rhs) { 
	return Time(lhs.TotalMicroseconds + (uint64_t)rhs); 
}

inline Time operator-(const Time& lhs, const uint32_t& rhs) { 
	return Time(lhs.TotalMicroseconds - (uint64_t)rhs); 
}

inline Time operator*(const Time& lhs, const uint32_t& rhs) { 
	return Time(lhs.TotalMicroseconds * (uint64_t)rhs); 
}

inline Time operator/(const Time& lhs, const uint32_t& rhs) { 
	return Time(lhs.TotalMicroseconds / (uint64_t)rhs); 
}

inline Time operator%(const Time& lhs, const uint32_t& rhs) { 
	return Time(lhs.TotalMicroseconds % (uint64_t)rhs); 
}


inline Time operator+(const Time& lhs, const uint64_t& rhs) { 
	return Time(lhs.TotalMicroseconds + rhs); 
}

inline Time operator-(const Time& lhs, const uint64_t& rhs) { 
	return Time(lhs.TotalMicroseconds - rhs); 
}

inline Time operator*(const Time& lhs, const uint64_t& rhs) { 
	return Time(lhs.TotalMicroseconds * rhs); 
}

inline Time operator/(const Time& lhs, const uint64_t& rhs) { 
	return Time(lhs.TotalMicroseconds / rhs); 
}

inline Time operator%(const Time& lhs, const uint64_t& rhs) {
	return Time(lhs.TotalMicroseconds % rhs); 
}