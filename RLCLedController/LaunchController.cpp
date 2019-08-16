#include "LaunchController.h"

LaunchController::LaunchController()
{
}

LaunchController::LaunchController(SyncTime &syncTime, RLCLedController &ledController)
{
	LaunchController::syncTime = syncTime;
	LaunchController::ledController = ledController;
}

LaunchController::~LaunchController()
{
}

void LaunchController::SetLaunchTime(Time &launchFromTime, Time &lauchTime)
{
	/*Time now = syncTime.Now();
	Time correctedLaunchTime = (now - lauchTime) + launchFromTime;*/
	//correctedLaunchTime.TotalMicroseconds / 
	

	//LaunchController::launchTime = launchTime;
}

void LaunchController::ClearLaunchTime()
{
	LaunchController::launchTime = 0;
}

void CorrectPosition(Time &lateness) 
{
		
}

void Launch() 
{

}

void LaunchController::CheckLaunch()
{
	Time now = syncTime.Now();
	Time lateness;

	if(launchTime < now)
	{
		lateness = now - launchTime;
		CorrectPosition(lateness);
		Launch();
		return;
	}
	
	if(launchTime == now || ((now + (uint32_t)50000)) > launchTime)
	{
		Launch();
	}
}
