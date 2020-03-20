#pragma once

//#include "../../RLCLibrary/Infrastructure/GlobalTypes.h"
#include "../Time.h"

void SetTimeToArray(Time time, uint8_t* destArray, int startIndex);

Time GetTimeFromArray(uint8_t* sourceArray, int startIndex);