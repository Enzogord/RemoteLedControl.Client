#pragma once

#include "../Infrastructure/GlobalTypes.h"
#include "Time.h"

typedef uint64_t(*GetCurrentMicros)();

void SetWorkTimeFunction(GetCurrentMicros);

Time TimeNow();

void CorrectTime(int64_t timeShift);
void SetTime(int64_t newTime);