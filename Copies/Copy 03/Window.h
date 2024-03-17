#ifndef WINDOW_H
#define WINDOW_H

#include <Windows.h>

class Window
{
	enum BackGroundColor
	{
		White = 1, Gray = 2, Black = 3
	};

	HWND m_Hwnd{ NULL };
	LPCTSTR m_WindowName{ L"RXR-App" };
	LPCTSTR m_WindowTitle{ L"RXR-Window" };
	int m_Width{ 800 };
	int m_Height{ 600 };
	bool m_Fullscreen{ false };
	bool m_Initialized{ false };
	BackGroundColor m_BackgroundColor{ Black };

public:
	Window(int width, int height)
	{
		m_Width = width;
		m_Height = height;
	}

	HWND* getHandleToTheWindow()
	{
		if (!m_Initialized)
		{
			OutputDebugStringA("Not initialized window!");
		}
		return &m_Hwnd;
	}

	bool Init(HINSTANCE hInstance)
	{
		if (m_Fullscreen)
		{
			HMONITOR hmon = MonitorFromWindow(m_Hwnd,
				MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = { sizeof(mi) };
			GetMonitorInfo(hmon, &mi);

			m_Width = mi.rcMonitor.right - mi.rcMonitor.left;
			m_Height = mi.rcMonitor.bottom - mi.rcMonitor.top;
		}

		WNDCLASSEX wc;

		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = NULL;
		wc.cbWndExtra = NULL;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + m_BackgroundColor);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = m_WindowName;
		wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

		if (!RegisterClassEx(&wc))
		{
			MessageBox(NULL, L"Error registering class", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		m_Hwnd = CreateWindowEx(NULL, m_WindowName, m_WindowTitle,
			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			m_Width, m_Height, NULL, NULL, hInstance, NULL);

		if (!m_Hwnd)
		{
			MessageBox(NULL, L"Error creating window", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}

		m_Initialized = true;

		if (m_Fullscreen)
		{
			SetWindowLong(m_Hwnd, GWL_STYLE, 0);
		}

		ShowWindow(m_Hwnd, 1);
		UpdateWindow(m_Hwnd);

		return true;
	}

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE)
			{
				if (MessageBox(0, L"Are you sure you want to exit?", L"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES)
					DestroyWindow(hwnd);
			}
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
};
#endif