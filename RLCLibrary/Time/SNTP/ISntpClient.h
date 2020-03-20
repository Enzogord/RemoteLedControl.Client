#pragma once

#include "SntpPackage.h"

class ISntpClient
{
public:
	virtual bool SendSntpRequest(SntpPackage& sntpPackage, int responseDelayMs = 1000) = 0;
};
