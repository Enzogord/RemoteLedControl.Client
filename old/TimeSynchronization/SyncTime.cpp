#include "SyncTime.h"

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
WiFiUDP udpService;

SyncTime::SyncTime()
{
}

SyncTime::~SyncTime()
{
}

void SyncTime::Init(WiFiUDP &udp)
{
	udpService = udp;
}

Time SyncTime::Now()
{
	uint32_t deltaMicros = micros() - LastMicros;
	LastTime += deltaMicros;
	LastMicros += deltaMicros;
	return LastTime;
}

Time SyncTime::RequestAndGetTimeShift(IPAddress &address, uint16_t port)
{
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
							 // 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;

	Time currentTime = Now();
	currentTime.SetSecondsTo(packetBuffer, 24);
	currentTime.SetSecondFractionsTo(packetBuffer, 28);

	udpService.beginPacket(address, port);
	udpService.write(packetBuffer, NTP_PACKET_SIZE);
	udpService.endPacket();

	uint32_t beginWait = millis();
	while (millis() - beginWait < 1500) {
		int size = udpService.parsePacket();
		if (size >= NTP_PACKET_SIZE) {
			//Serial.println("Receive NTP Response");
			udpService.read(packetBuffer, NTP_PACKET_SIZE);
			
			//T1
			Time t1 = Time(packetBuffer, 24);

			//T2
			Time t2 = Time(packetBuffer, 32);

			//T3
			Time t3 = Time(packetBuffer, 40);

			//T4
			Time t4 = Now();
			return GetTimeShift(t1, t2, t3, t4);
		}
	}
	return Time();
	//Serial.println("No NTP Response");
}


int SyncTime::SynchronizeTimeWithoutAverageSet(IPAddress& address, uint16_t port)
{
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
							 // 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;

	Time currentTime = Now();
	currentTime.SetSecondsTo(packetBuffer, 24);
	currentTime.SetSecondFractionsTo(packetBuffer, 28);

	udpService.beginPacket(address, port);
	udpService.write(packetBuffer, NTP_PACKET_SIZE);
	udpService.endPacket();

	uint32_t beginWait = millis();
	while (millis() - beginWait < 1500) {
		int size = udpService.parsePacket();
		if (size >= NTP_PACKET_SIZE) {
			//Serial.println("Receive NTP Response");
			udpService.read(packetBuffer, NTP_PACKET_SIZE);

			//T1
			Time t1 = Time(packetBuffer, 24);

			//T2
			Time t2 = Time(packetBuffer, 32);

			//T3
			Time t3 = Time(packetBuffer, 40);

			//T4
			Time t4 = Now();

			LastTime = GetCorrectedTimeWithoutAverageSet(t1, t2, t3, t4);
			LastSynchronizationTime = LastTime;
			LastMicros = micros();

			return 1;
		}
	}
	return 0;
	//Serial.println("No NTP Response");
}

Time SyncTime::GetCorrectedTimeWithoutAverageSet(Time sendTime, Time serverReceiveTime, Time serverSendTime, Time receiveTime)
{
	//T1 = sendTime
	//T2 = serverReceiveTime
	//T3 = serverSendTime
	//T4 = receiveTime
	//(T4-T3)+(T2-T1)
	//((Ò2 – Ò1) + (Ò3 – Ò4)) / 2

	//Time receivedTime = receiveTime;
	// A = T2 - T1
	Time A = serverReceiveTime - sendTime;// SubstractTime(serverReceiveTime, sendTime);

	// B = T3 - T4
	Time B = serverSendTime - receiveTime;// SubstractTime(serverSendTime, receiveTime);

	//sum À + B
	Time C = A + B;
	C /= (uint64_t)2;
	C += receiveTime;
	Serial.print("C:"); Serial.println((int)C.TotalMicroseconds);

	return C;
}

int SyncTime::SynchronizeTimeMultiple(IPAddress& address, uint16_t port, uint8_t syncCount)
{
	while(!SynchronizeTimeWithoutAverageSet(address, port)) {
		Serial.println("Time not syncronized");
	}
	Serial.print("1. LastTime:"); Serial.println((int)LastTime.TotalMicroseconds);

	ntpDeltas = new int[syncCount];
	memset(ntpDeltas, 0, syncCount);
	int64_t average = 0;
	for (size_t i = 0; i < syncCount; i++)
	{
		Time t = RequestAndGetTimeShift(address, port);
		Serial.print("t:"); Serial.println((int)t.TotalMicroseconds);
		ntpDeltas[i] = t.TotalMicroseconds;
		average += t.TotalMicroseconds;
	}
	int64_t avg = (average / syncCount);
	Serial.print("avg:"); Serial.println((int)avg);

	LastTime = Now();
	LastTime.TotalMicroseconds + avg;
	Serial.print("2. LastTime:"); Serial.print(LastTime.GetSeconds()); Serial.print("."); Serial.println(LastTime.GetMicroseconds());
	LastTime.TotalMicroseconds + avg;
	LastSynchronizationTime = LastTime;
	LastMicros = micros();
	return 0;
}


Time SyncTime::GetTimeShift(Time sendTime, Time serverReceiveTime, Time serverSendTime, Time receiveTime)
{
	//T1 = sendTime
	//T2 = serverReceiveTime
	//T3 = serverSendTime
	//T4 = receiveTime
	//(T4-T3)+(T2-T1)
	//((Ò2 – Ò1) + (Ò3 – Ò4)) / 2

	//Time receivedTime = receiveTime;
	// A = T2 - T1
	Time A = serverReceiveTime - sendTime;// SubstractTime(serverReceiveTime, sendTime);

	// B = T3 - T4
	Time B = serverSendTime - receiveTime;// SubstractTime(serverSendTime, receiveTime);

	//sum À + B
	Time C = A + B;
	C /= (uint64_t)2;
	return C;
}

void SyncTime::CalcTime(uint32_t seconds, TimeParameters &tm) {

	uint8_t year;
	uint8_t month, monthLength;
	uint32_t time;
	unsigned long days;

	time = seconds;//LastTime.Seconds;
	tm.Second = time % 60;
	time /= 60; // now it is minutes
	tm.Minute = time % 60;
	time /= 60; // now it is hours
	tm.Hour = time % 24;
	time /= 24; // now it is days

	year = 0;
	days = 0;
	while ((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
		year++;
	}
	tm.Year = year; // year is offset from 1970 

	days -= LEAP_YEAR(year) ? 366 : 365;
	time -= days; // now it is days in this year, starting at 0

	days = 0;
	month = 0;
	monthLength = 0;
	for (month = 0; month < 12; month++) {
		if (month == 1) { // february
			if (LEAP_YEAR(year)) {
				monthLength = 29;
			}
			else {
				monthLength = 28;
			}
		}
		else {
			monthLength = monthDays[month];
		}

		if (time >= monthLength) {
			time -= monthLength;
		}
		else {
			break;
		}
	}
	tm.Month = month + 1;  // jan is month 1  
	tm.Day = time + 1;     // day of month
}