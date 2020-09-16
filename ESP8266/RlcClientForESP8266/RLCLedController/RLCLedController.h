#pragma once
#include "FastLED.h"
#include <SD.h>
#include "../Service/PinController.h"
#include "../RLCMessage/RLCEnums.h"
#include <TimeNow.h>
#include <ILogger.h>

typedef void (*FastLedInitialization)();
typedef void (*ReopenFile)();

enum class LEDControllerStatuses {
	Stoped,
	Played,
	Paused
};

inline const char* ToString(LEDControllerStatuses ledControlledStatus)
{
	switch(ledControlledStatus)
	{
	case LEDControllerStatuses::Stoped:   return "Stoped";
	case LEDControllerStatuses::Played:   return "Played";
	case LEDControllerStatuses::Paused:   return "Paused";
	default:      return "None";
	}
}

class RLCLedController
{
public:
	unsigned int LedCount;
	CRGB* LedArray;
	unsigned int PWMChannelCount;
	int* PWMChannels;
	LEDControllerStatuses Status;
	boolean IsInitialized;
	unsigned int FrameBytes;

	//время одного кадра, мс (по умолчанию 50 мс)
	uint32_t frameTime = 50;

	RLCLedController(ILogger& loggerRef);
	~RLCLedController();

	void Initialize(FastLedInitialization initializerMethod, File &cyclogrammFile, ReopenFile reopenFileMethod);

	bool defaultLightOn;

	void Play();
	void PlayFrom(Time &launchFromTime, Time &lauchTime);
	void Rewind(Time &launchFromTime, Time &lauchTime, ClientStateEnum &clientState);
	void Stop();
	void Pause();
	void Show();
	
	void NextFrame();

private:
	ILogger& logger;
	ReopenFile reopenFileMethod;
	File cyclogrammFile;
	boolean showNext = false;
	boolean cyclogrammEnded = false;

	// Номер кадр с которого будет начато воспроизведение
	//unsigned long launchFrame;

	// Текущая позиция кадра (порядковый номер кадра, не индекс байта в файла!)
	uint32_t framePosition = 0;

	// Устанавливает позицию в файле на необходимый кадр
	void SetPosition(uint64_t framePosition);

	// Сбрасывает позицию в файле на начало
	void ResetPosition();

	// Устанавливает время циклограммы с которого необходимо начать воспроизведение
	bool SetLaunchTime(Time &launchFromTime, Time &lauchTime);
	
	// Получение индекса байта в файле для текущего кадра
	inline uint64_t GetFrameBytePosition(uint64_t framePos);

	// Получение текущего времени воспроизведения циклограммы
	Time GetCurrentPlayTime();

	// Получение длины циклограммы
	Time GetCyclogrammLength();

	uint32_t GetFrameFromTime(Time& time);

	//Подготовка данных для следующего кадра
	void NextFrameDataPreparation();

	//Проверяет доступность файла циклограммы. Если не файл не доступен, пытается переоткрыть его.
	bool CheckFileAvailability();
};

