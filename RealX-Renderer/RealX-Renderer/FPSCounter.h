#ifndef FPS_COUNTER_H
#define FPS_COUNTER_H

#include <iostream>
#include <fstream>
#include "stdfax.h"
#include "psapi.h"

class FPSCounter
{
private:
	double         m_TimerFrequency{};
	long long      m_LastFrameTime{};
	long long      m_LastSecond{};
	double		   m_FrameDelta{};
	int			   m_Fps{};
	int			   m_FrameId{};
	int			   m_NumberOfProcessors{};
	std::ofstream  m_LogFile{};
	ULARGE_INTEGER m_LastCPU;
	ULARGE_INTEGER m_LastSystemCPU;
	ULARGE_INTEGER m_LastUserCPU;
	HANDLE	       m_Self;
	PROCESS_MEMORY_COUNTERS_EX m_Pmc{};

public:
	// Custom constructor.
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
		m_LogFile << "Time, Frames per second [F/S], RAM usage [B], CPU usage [%]\n";

		InitCPUMonitor();
	}

	// Initializing CPU monitor.
	void InitCPUMonitor()
	{
		SYSTEM_INFO systemInfo;
		FILETIME ftime, fsys, fuser;
		GetSystemInfo(&systemInfo);
		m_NumberOfProcessors = systemInfo.dwNumberOfProcessors;
		GetSystemTimeAsFileTime(&ftime);
		memcpy(&m_LastCPU, &ftime, sizeof(FILETIME));
		m_Self = GetCurrentProcess();
		GetProcessTimes(m_Self, &ftime, &ftime, &fsys, &fuser);
		memcpy(&m_LastSystemCPU, &fsys, sizeof(FILETIME));
		memcpy(&m_LastUserCPU, &fuser, sizeof(FILETIME));
	}

	// Call this once per frame.
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

		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&m_Pmc, sizeof(m_Pmc));
		SIZE_T memoryUsed = m_Pmc.WorkingSetSize;

		m_LogFile << m_FrameId << ", " << m_Fps << ", " <<memoryUsed << ", " << getCurrentCPUUsage() << "\n";
	}

	// Get current CPU usage in %.
	double getCurrentCPUUsage()
	{
		FILETIME ftime, fsys, fuser;
		ULARGE_INTEGER now, sys, user;
		double percent;

		GetSystemTimeAsFileTime(&ftime);
		memcpy(&now, &ftime, sizeof(FILETIME));
		GetProcessTimes(m_Self, &ftime, &ftime, &fsys, &fuser);
		memcpy(&sys, &fsys, sizeof(FILETIME));
		memcpy(&user, &fuser, sizeof(FILETIME));
		percent = (sys.QuadPart - m_LastSystemCPU.QuadPart) + (user.QuadPart - m_LastUserCPU.QuadPart);
		percent /= (now.QuadPart - m_LastCPU.QuadPart);
		percent /= m_NumberOfProcessors;
		m_LastCPU = now;
		m_LastUserCPU = user;
		m_LastSystemCPU = sys;

		return percent * 100;
	}

};
#endif