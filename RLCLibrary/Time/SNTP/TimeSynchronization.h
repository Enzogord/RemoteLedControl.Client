#pragma once

#include "../../RLCLibrary/Infrastructure/GlobalTypes.h"
#include "../../RLCLibrary/Logger/ILogger.h"
#include "../../RLCLibrary/Infrastructure/Math.h"

#include "SntpPackage.h"
#include "ISntpClient.h"
#include "../TimeNow.h"
#include "SntpFunctions.h"

class TimeSynchronization
{
private:
	ILogger& logger;
	ISntpClient& sntpClient;
	void RunSntpRequest(SntpPackage& sntpPackage);
	int64_t GetTimeShift(SntpPackage& sntpPackage);
	int64_t RunRequestAndGetTimeShift();

public:
	TimeSynchronization(ISntpClient& sntpClient, ILogger& logger);
	~TimeSynchronization();

	void SynchronizeSingle();
	void SynchronizeMultiple(uint8_t iterationsCount = 10, int distortionLimitModule = 10000);
};