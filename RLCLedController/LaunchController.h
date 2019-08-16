#pragma once
#include "TimeSynchronization/Time.h"
#include "TimeSynchronization/SyncTime.h"
#include "RLCLedController.h"

class LaunchController
{
public:
	LaunchController();
	LaunchController(SyncTime &syncTime, RLCLedController &ledController);
	~LaunchController();

	void SetLaunchTime(Time &launchFromTime, Time &lauchTime, int frameTime);
	void ClearLaunchTime();
	void CheckLaunch();

private:
	RLCLedController ledController;
	SyncTime syncTime;
	Time launchTime;
	uint64_t cyclogrammDuration;
};

