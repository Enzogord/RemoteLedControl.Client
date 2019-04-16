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
	LastTime.AddMicroseconds(deltaMicros);
	LastMicros += deltaMicros;
	return LastTime;
}

int SyncTime::SynchronizeTime(IPAddress &address, uint16_t port)
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

	uint32_t currentFractions = ((unsigned long long)currentTime.Microseconds) * 0x100000000L / 1000000UL;
	//Serial.print("currentFractions: "); Serial.println(currentFractions);

	//Заполнение времени отправки
	packetBuffer[24] = currentTime.Seconds >> 24;
	packetBuffer[25] = currentTime.Seconds >> 16;
	packetBuffer[26] = currentTime.Seconds >> 8;
	packetBuffer[27] = currentTime.Seconds;

	packetBuffer[28] = currentFractions >> 24;
	packetBuffer[29] = currentFractions >> 16;
	packetBuffer[30] = currentFractions >> 8;
	packetBuffer[31] = currentFractions;
	//Serial.print("Sended bytes: ");
	for (int i = 24; i < 32; i++)
	{
		Serial.print(packetBuffer[i]); Serial.print(", ");
	}
	Serial.println();
	Serial.print("Time before sended: "); Serial.print(currentTime.Seconds); Serial.print(" sec, "); Serial.println(currentTime.Microseconds);

	udpService.beginPacket(address, port);
	udpService.write(packetBuffer, NTP_PACKET_SIZE);
	udpService.endPacket();

	uint32_t beginWait = millis();
	while (millis() - beginWait < 1500) {
		int size = udpService.parsePacket();
		if (size >= NTP_PACKET_SIZE) {
			Serial.println("Receive NTP Response");
			udpService.read(packetBuffer, NTP_PACKET_SIZE);
			
			//T1
			unsigned long t1Secs;
			t1Secs = (unsigned long)packetBuffer[24] << 24;
			t1Secs |= (unsigned long)packetBuffer[25] << 16;
			t1Secs |= (unsigned long)packetBuffer[26] << 8;
			t1Secs |= (unsigned long)packetBuffer[27];
			unsigned long t1Fractions;
			t1Fractions = (unsigned long)packetBuffer[28] << 24;
			t1Fractions |= (unsigned long)packetBuffer[29] << 16;
			t1Fractions |= (unsigned long)packetBuffer[30] << 8;
			t1Fractions |= (unsigned long)packetBuffer[31];
			uint32_t t1Microseconds = ((unsigned long long)t1Fractions) * 1000000UL / 0x100000000L;
			Time t1 = Time(t1Secs, t1Microseconds);
			//Serial.print("T1: "); Serial.print(t1.Seconds); Serial.print(" sec, "); Serial.println(t1.Microseconds);

			//T2
			unsigned long t2Secs;
			t2Secs = (unsigned long)packetBuffer[32] << 24;
			t2Secs |= (unsigned long)packetBuffer[33] << 16;
			t2Secs |= (unsigned long)packetBuffer[34] << 8;
			t2Secs |= (unsigned long)packetBuffer[35];
			unsigned long t2Fractions;
			t2Fractions = (unsigned long)packetBuffer[36] << 24;
			t2Fractions |= (unsigned long)packetBuffer[37] << 16;
			t2Fractions |= (unsigned long)packetBuffer[38] << 8;
			t2Fractions |= (unsigned long)packetBuffer[39];
			uint32_t t2Microseconds = ((unsigned long long)t2Fractions) * 1000000UL / 0x100000000L;
			Time t2 = Time(t2Secs, t2Microseconds);
			//Serial.print("T2: "); Serial.print(t2.Seconds); Serial.print(" sec, "); Serial.println(t2.Microseconds);

			//T3
			unsigned long t3Secs;
			t3Secs = (unsigned long)packetBuffer[40] << 24;
			t3Secs |= (unsigned long)packetBuffer[41] << 16;
			t3Secs |= (unsigned long)packetBuffer[42] << 8;
			t3Secs |= (unsigned long)packetBuffer[43];
			unsigned long t3Fractions;
			t3Fractions = (unsigned long)packetBuffer[44] << 24;
			t3Fractions |= (unsigned long)packetBuffer[45] << 16;
			t3Fractions |= (unsigned long)packetBuffer[46] << 8;
			t3Fractions |= (unsigned long)packetBuffer[47];
			uint32_t t3Microseconds = ((unsigned long long)t3Fractions) * 1000000UL / 0x100000000L;
			Time t3 = Time(t3Secs, t3Microseconds);
			//Serial.print("T3: "); Serial.print(t3.Seconds); Serial.print(" sec, "); Serial.println(t3.Microseconds);

			//T4
			Time t4 = Now();
			//Serial.print("T4: "); Serial.print(t4.Seconds); Serial.print(" sec, "); Serial.println(t4.Microseconds);

			LastTime = GetCorrectedTime(t1, t2, t3, t4);
			LastSynchronizationTime = LastTime;
			LastMicros = micros();

			return 1;
		}
	}
	return 0;
	Serial.println("No NTP Response");
}

Time SyncTime::GetCorrectedTime(Time sendTime, Time serverReceiveTime, Time serverSendTime, Time receiveTime)
{
	//T1 = sendTime
	//T2 = serverReceiveTime
	//T3 = serverSendTime
	//T4 = receiveTime
	//(T4-T3)+(T2-T1)
	//((Т2 – Т1) + (Т3 – Т4)) / 2
	
	//Time receivedTime = receiveTime;
	Time A = SubstractTime(serverReceiveTime, sendTime);
	Time B = SubstractTime(serverSendTime, receiveTime);

	//sum А + B
	uint32_t microC = A.Microseconds + B.Microseconds;
	uint64_t sumSeconds = (uint64_t)A.Seconds + (uint64_t)B.Seconds + (uint64_t)(microC / 1000000);
	uint32_t sumMicroseconds = microC % 1000000;

	//divided by 2, and sum with receiveTime
	sumSeconds *= 1000000;
	sumSeconds += sumMicroseconds;
	sumSeconds /= 2;
	uint64_t received64 = ((uint64_t)receiveTime.Seconds * (uint64_t)1000000) + (uint64_t)receiveTime.Microseconds;
	sumSeconds += received64;

	Time result = Time(sumSeconds / 1000000, sumSeconds % 1000000);

	return result;
}

Time SyncTime::SubstractTime(Time a, Time b) {
	Time result = a;
	int32_t micro = a.Microseconds - b.Microseconds;
	if (micro < 0)
	{
		result.Seconds--;
		result.Microseconds = 1000000 + micro;
	}
	else
	{
		result.Microseconds = micro;

	}
	result.Seconds -= b.Seconds;
	return result;
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