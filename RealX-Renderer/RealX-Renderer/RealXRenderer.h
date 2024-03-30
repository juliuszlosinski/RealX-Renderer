#ifndef REALXRENDERER_H
#define REALXRENDERER_H

#include "Window.h"
#include "D3D12AppXeSS.h"
#include "D3D12AppDefault.h"

class RealXRenderer
{
	Window*			 m_Window{nullptr};
	D3D12AppXeSS*	 m_D3D12AppXeSS{nullptr};
	D3D12AppDefault* m_D3D12AppDefault{ nullptr };
	int				 m_OutputWidth{ 1920 };
	int				 m_OutputHeight{ 1080 };
	int				 m_WindowWidth{ 800 };
	int				 m_WindowHeight{ 600 };
	bool		     m_XeSS{ false };

public:
	// Default costructor;
	RealXRenderer()
	{
		m_XeSS = false;
		m_Window = new Window{m_OutputWidth, m_OutputHeight};
		if (m_XeSS) 
		{
			m_D3D12AppXeSS = new D3D12AppXeSS{ static_cast<UINT>(m_OutputWidth), static_cast<UINT>(m_OutputHeight) };
		}
		else
		{
			m_D3D12AppDefault = new D3D12AppDefault{ m_OutputWidth, m_OutputHeight };
		}
	}

	// Custom constructor with viewport resolution and enabling/ disabling XeSS technology.
	RealXRenderer(int viewPortWidth, int viewPortHeight, bool xess)
	{
		m_XeSS = xess;
		m_OutputWidth = viewPortWidth;
		m_OutputHeight = viewPortHeight;
		m_Window = new Window{m_OutputWidth, m_OutputHeight};
		if (m_XeSS)
		{
			m_D3D12AppXeSS = new D3D12AppXeSS{ static_cast<UINT>(m_OutputWidth), static_cast<UINT>(m_OutputHeight) };
		}
		else 
		{
			m_D3D12AppDefault = new D3D12AppDefault{ m_OutputWidth, m_OutputHeight };
		}
	}

	// Custom constructor with enabling/ disabling XeSS technology.
	RealXRenderer(bool xess)
	{
		m_XeSS = xess;
		m_Window = new Window{ m_WindowWidth, m_WindowHeight };
		if (m_XeSS)
		{
			m_D3D12AppXeSS = new D3D12AppXeSS{ static_cast<UINT>(m_OutputWidth), static_cast<UINT>(m_OutputHeight) };
		}
		else
		{
			m_D3D12AppDefault = new D3D12AppDefault{ m_OutputWidth, m_OutputHeight };
		}
	}

	// Initializing the application.
	int Init(HINSTANCE hInstance)
	{
		if (!m_Window->Init(hInstance))
		{
			MessageBox(0, L"Window Initialization - Failed", L"Error", MB_OK);
				return false;
		}
		if (m_XeSS)
		{
			if (!m_D3D12AppXeSS->Init(*m_Window->getHandleToTheWindow()))
			{
				MessageBox(0, L"D3D12 Initialization - Failed", L"Error", MB_OK);
				return false;
			}
		}
		else 
		{
			if (!m_D3D12AppDefault->Init(*m_Window->getHandleToTheWindow()))
			{
				MessageBox(0, L"D3D12 Initialization - Failed", L"Error", MB_OK);
				return false;
			}
		}
		return true;
	}

	// Running application.
	void Run()
	{
		MSG msg;
		ZeroMemory(&msg, sizeof(MSG));

		while (true)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					Exit();
					break;
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				Render();
			}
		}
	}

	// Stop working.
	void Exit()
	{
		if (m_XeSS)
		{
			m_D3D12AppXeSS->Exit();
		}
		else 
		{
			m_D3D12AppDefault->Exit();
		}
	}

	// Rendering.
	void Render()
	{
		OutputDebugStringA("Rendering\n");
		
		if (m_XeSS)
		{
			m_D3D12AppXeSS->Update();
			m_D3D12AppXeSS->Render();
		}
		else 
		{
			m_D3D12AppDefault->Update();
			m_D3D12AppDefault->Render();
		}
	}
};
#endif