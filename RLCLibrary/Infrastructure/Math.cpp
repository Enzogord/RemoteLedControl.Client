#include "Math.h"

int64_t Substract(uint64_t lhs, uint64_t rhs)
{
	if(lhs == rhs) {
		return 0;
	}

	int64_t max = 0;
	for(int64_t cur = 1; cur > max; cur = (cur << 1) + 1) {
		max = cur;
	}
	uint64_t result;
	if(lhs > rhs) {
		result = (lhs - rhs);
		if(result < max) {
			return (int64_t)result;
		}
		else {
			return max;
		}
	}
	else {
		result = (rhs - lhs);

		if(result < max) {
			return -(int64_t)result;
		}
		else {
			return max;
		}
	}
}