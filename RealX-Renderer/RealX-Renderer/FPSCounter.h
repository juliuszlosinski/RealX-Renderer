#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

#include "stdfax.h"

class FPSCounter
{
private:
	double m_TimerFrequency{};
	long long m_LastFrameTime{};
	long long m_LastSecond{};
	double m_FrameDelta{};
	int m_Fps{};
	int m_FrameId{};

public:
	FPSCounter()
	{
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);

		// seconds
		//timerFrequency = double(li.QuadPart);

		// milliseconds
		m_TimerFrequency = double(li.QuadPart) / 1000.0;

		// microseconds
		//timerFrequency = double(li.QuadPart) / 1000000.0;

		QueryPerformanceCounter(&li);
		m_LastFrameTime = li.QuadPart;

	}

	// Call this once per frame
	double GetFrameDelta()
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		m_FrameDelta = double(li.QuadPart - m_LastFrameTime) / m_TimerFrequency;
		if (m_FrameDelta > 0)
			m_Fps = 1000 / m_FrameDelta;
		m_LastFrameTime = li.QuadPart;
		return m_FrameDelta;
	}

	// Printing FPS.
	void PrintFPS()
	{
		char msg[300];
		sprintf_s(msg, "FPS: %d \n", m_Fps);
		OutputDebugStringA(msg);
	}
};
#endif