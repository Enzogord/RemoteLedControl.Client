#include "TimeSynchronization.h"

TimeSynchronization::TimeSynchronization(ISntpClient& sntpClientRef, ILogger& loggerRef)
	: sntpClient(sntpClientRef), logger(loggerRef)
{
}

TimeSynchronization::~TimeSynchronization()
{
}


void TimeSynchronization::RunSntpRequest(SntpPackage& sntpPackage)
{
	while(!sntpClient.SendSntpRequest(sntpPackage)) {
		logger.Print("No SNTP response was received");
	}
}

int64_t TimeSynchronization::GetTimeShift(SntpPackage& sntpPackage)
{
	//T1 = sendTime
	//T2 = serverReceiveTime
	//T3 = serverSendTime
	//T4 = receiveTime

	//timeShift = ((Ò2 – Ò1) + (Ò3 – Ò4)) / 2
	uint64_t t1 = sntpPackage.GetSendingTime().TotalMicroseconds;
	uint64_t t2 = sntpPackage.GetServerReceiveTime().TotalMicroseconds;
	uint64_t t3 = sntpPackage.GetServerSendingTime().TotalMicroseconds;
	uint64_t t4 = sntpPackage.GetReceiveTime().TotalMicroseconds;

	int64_t a = Substract(t2, t1);
	int64_t b = Substract(t3, t4);
	int64_t timeShift = (a + b) / (int64_t)2;
	logger.Print("TimeShift: ", timeShift, false);logger.Print("us");
	return timeShift;
}

int64_t TimeSynchronization::RunRequestAndGetTimeShift()
{
	SntpPackage sntpPackage;
	RunSntpRequest(sntpPackage);
	int64_t timeShift = GetTimeShift(sntpPackage);
	return timeShift;
}

void TimeSynchronization::SynchronizeSingle()
{
	int64_t timeShift = RunRequestAndGetTimeShift();
	CorrectTime(timeShift);
}

void TimeSynchronization::SynchronizeMultiple(uint8_t iterationsCount, int distortionLimitModule)
{	
	SynchronizeSingle();
		
	int64_t preliminaryTimeShift = RunRequestAndGetTimeShift();
	while(preliminaryTimeShift > distortionLimitModule || preliminaryTimeShift < -distortionLimitModule) {
		CorrectTime(preliminaryTimeShift);
		preliminaryTimeShift = RunRequestAndGetTimeShift();
	}

	double averageTimeShift = 0;

	uint8_t index = 0;
	while(index < iterationsCount) {
		int64_t timeShift = RunRequestAndGetTimeShift();
		if(timeShift > distortionLimitModule || timeShift < -distortionLimitModule) {
			continue;
		}
		averageTimeShift += ((double)timeShift / (double)iterationsCount);
		index++;
	}
	logger.Print("Average timeshift: ", averageTimeShift, false); logger.Print("us");
	CorrectTime((int64_t)averageTimeShift);
}
