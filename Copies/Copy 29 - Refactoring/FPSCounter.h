#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

#include <iostream>
#include <fstream>
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
	std::ofstream m_LogFile{};

public:
	FPSCounter(std::string pathToFile)
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

		m_LogFile.open(pathToFile);
		m_LogFile << "i, fps\n";
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

	// Exiting fps counter.
	void Exit()
	{
		m_LogFile.close();
	}

	// Printing FPS.
	void PrintFPS()
	{
		m_FrameId++;
		char msg[300];
		sprintf_s(msg, "FPS: %d \n", m_Fps);
		OutputDebugStringA(msg);
		m_LogFile << m_FrameId << ", " << m_Fps << "\n";
	}
};
#endif