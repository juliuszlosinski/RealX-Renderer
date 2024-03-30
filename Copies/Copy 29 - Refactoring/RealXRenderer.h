#ifndef REALXRENDERER_H
#define REALXRENDERER_H

#include "Window.h"
#include "D3D12AppXeSS.h"
#include "D3D12AppDefault.h"

class RealXRenderer
{
	Window* m_Window{nullptr};
	D3D12AppXeSS* m_D3D12AppXeSS{nullptr};
	D3D12AppDefault* m_D3D12AppDefault{ nullptr };
	int m_ViewPortWidth{ 800 };
	int m_ViewPortHeight{ 600 };

public:
	RealXRenderer()
	{
		m_Window = new Window{m_ViewPortWidth, m_ViewPortHeight};
		m_D3D12AppXeSS = new D3D12AppXeSS{static_cast<UINT>(m_ViewPortWidth), static_cast<UINT>(m_ViewPortHeight)};
		m_D3D12AppDefault = new D3D12AppDefault{ m_ViewPortWidth, m_ViewPortHeight };
	}

	RealXRenderer(int width, int height)
	{
		m_ViewPortWidth = width;
		m_ViewPortHeight = height;
		m_Window = new Window{m_ViewPortWidth, m_ViewPortHeight};
		m_D3D12AppXeSS = new D3D12AppXeSS{static_cast<UINT>(m_ViewPortWidth), static_cast<UINT>(m_ViewPortHeight)};
		m_D3D12AppDefault = new D3D12AppDefault{ m_ViewPortWidth, m_ViewPortHeight };
	}

	int Init(HINSTANCE hInstance)
	{
		if (!m_Window->Init(hInstance))
		{
			MessageBox(0, L"Window Initialization - Failed", L"Error", MB_OK);
				return false;
		}
		/*
		if (!m_D3D12AppXeSS->Init(*m_Window->getHandleToTheWindow()))
		{
			MessageBox(0, L"D3D12 Initialization - Failed", L"Error", MB_OK);
				return false;
		}
		*/
		if (!m_D3D12AppDefault->Init(*m_Window->getHandleToTheWindow()))
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

	void Exit()
	{
		m_D3D12AppXeSS->Exit();
		m_D3D12AppDefault->Exit();
	}

	void Render()
	{
		OutputDebugStringA("Rendering\n");
		/*
		m_D3D12AppXeSS->Update();
		m_D3D12AppXeSS->Render();
		*/
		m_D3D12AppDefault->Update();
		m_D3D12AppDefault->Render();
	}
};
#endif