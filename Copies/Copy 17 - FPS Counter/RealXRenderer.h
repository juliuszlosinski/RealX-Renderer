#ifndef REALXRENDERER_H
#define REALXRENDERER_H

#include "Window.h"
#include "D3D12App.h"

class RealXRenderer
{
	Window* m_Window{nullptr};
	D3D12App* m_D3D12App{nullptr};
	int m_Width{ 800 };
	int m_Height{ 600 };

public:
	RealXRenderer()
	{
		m_Window = new Window{m_Width, m_Height};
		m_D3D12App = new D3D12App{m_Width, m_Height};
	}

	RealXRenderer(int width, int height)
	{
		m_Width = width;
		m_Height = height;
		m_Window = new Window{m_Width, m_Height};
		m_D3D12App = new D3D12App{m_Width, m_Height};
	}

	int Init(HINSTANCE hInstance)
	{
		if (!m_Window->Init(hInstance))
		{
			MessageBox(0, L"Window Initialization - Failed", L"Error", MB_OK);
				return false;
		}
		if (!m_D3D12App->Init(m_Window->getHandleToTheWindow()))
		{
			MessageBox(0, L"D3D12 Initialization - Failed", L"Error", MB_OK);
				return false;
		}
		return true;
	}

	void Run()
	{
		MSG msg;
		ZeroMemory(&msg, sizeof(MSG));

		while (true)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
					break;

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				Render();
			}
		}
	}

	void Render()
	{
		OutputDebugStringA("Rendering\n");
		m_D3D12App->Render();
	}
};
#endif