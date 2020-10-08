#include "TimeSynchronization.h"

TimeSynchronization::TimeSynchronization(ISntpClient* sntpClient, ILogger& loggerRef)
	: logger(loggerRef)
{
	TimeSynchronization::sntpClient = sntpClient;
}

TimeSynchronization::~TimeSynchronization()
{
}


bool TimeSynchronization::RunSntpRequest(SntpPackage* sntpPackage)
{
	uint8_t attemptsLeft = 5;
	bool result = false;
	do {
		if (sntpClient->SendSntpRequest(sntpPackage, 100)) {
			result = true;
		}
		else {
			if (attemptsLeft == 0) {
				break;
			}
			logger.Print("No SNTP response was received");
			attemptsLeft--;
		}
	} while (!result);
	return result;
}

int64_t TimeSynchronization::GetTimeShift(SntpPackage* sntpPackage)
{
	//T1 = sendTime
	//T2 = serverReceiveTime
	//T3 = serverSendTime
	//T4 = receiveTime

	//timeShift = ((Т2 – Т1) + (Т3 – Т4)) / 2
	uint64_t t1 = sntpPackage->GetSendingTime().TotalMicroseconds;
	uint64_t t2 = sntpPackage->GetServerReceiveTime().TotalMicroseconds;
	uint64_t t3 = sntpPackage->GetServerSendingTime().TotalMicroseconds;
	uint64_t t4 = sntpPackage->GetReceiveTime().TotalMicroseconds;

	int64_t a = Substract(t2, t1);
	int64_t b = Substract(t3, t4);
	int64_t timeShift = (a + b) / (int64_t)2;
	logger.Print("TimeShift: ", timeShift, false);logger.Print("us");
	return timeShift;
}

bool TimeSynchronization::TryRunRequestAndGetTimeShift(int64_t* timeShift)
{
	SntpPackage* sntpPackage = new SntpPackage();
	bool result = false;
	if (RunSntpRequest(sntpPackage)) {
		*timeShift = GetTimeShift(sntpPackage);
		result = true;
	}
	delete(sntpPackage);
	return result;
}

void TimeSynchronization::SynchronizeSingle()
{
	int64_t timeShift;
	while (!TryRunRequestAndGetTimeShift(&timeShift)) {
	}
	CorrectTime(timeShift);
}

void TimeSynchronization::SynchronizeMultiple(uint8_t iterationsCount, int distortionLimitModule)
{	
	//first single sync
	SynchronizeSingle();

	int64_t preliminaryTimeShift;
	do {
		if (!TryRunRequestAndGetTimeShift(&preliminaryTimeShift)) {
			return;
		}
		CorrectTime(preliminaryTimeShift);

	} while (preliminaryTimeShift > distortionLimitModule || preliminaryTimeShift < -distortionLimitModule);

	double k2 = 0.1;  // коэффициент фильтрации, 0.0-1.0
	double runningAverageValue = 0;

	int64_t timeShift;
	uint8_t index = 0;
	while (index < iterationsCount) {
		if (!TryRunRequestAndGetTimeShift(&timeShift)) {
			return;
		}
		if (timeShift > distortionLimitModule || timeShift < -distortionLimitModule) {
			continue;
		}
		runningAverageValue += ((double)timeShift - runningAverageValue) * k2;
		index++;
	}

	CorrectTime((int64_t)runningAverageValue);
}
