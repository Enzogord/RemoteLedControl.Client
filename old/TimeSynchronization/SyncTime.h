#pragma once
#include "Time.h"
#include <WiFiUdp.h>

#define SECS_PER_DAY  ((time_t)(SECS_PER_HOUR * 24UL))
#define LEAP_YEAR(Y) ( ((1970+(Y))>0) && !((1970+(Y))%4) && ( ((1970+(Y))%100) || !((1970+(Y))%400) ) )
static const uint8_t monthDays[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

struct TimeParameters{
	uint8_t Second;
	uint8_t Minute;
	uint8_t Hour;
	uint8_t Day;
	uint8_t Month;
	uint8_t Year;
};

class SyncTime
{
public:
	SyncTime();
	~SyncTime();
	void Init(WiFiUDP &udp);
	Time Now();

	Time LastSynchronizationTime;
	Time LastTime;

	uint32_t LastMicros;

	Time RequestAndGetTimeShift(IPAddress& address, uint16_t port);
	int SynchronizeTimeWithoutAverageSet(IPAddress& address, uint16_t port);
	int SynchronizeTimeMultiple(IPAddress& address, uint16_t port, uint8_t syncCount);
	Time GetTimeShift(Time sendTime, Time serverReceiveTime, Time serverSendTime, Time receiveTime);

	Time GetCorrectedTimeWithoutAverageSet(Time sendTime, Time serverReceiveTime, Time serverSendTime, Time receiveTime);

	void CalcTime(uint32_t seconds, TimeParameters &tm);
private:
	int* ntpDeltas;
};

