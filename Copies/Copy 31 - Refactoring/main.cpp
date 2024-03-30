#include "RealXRenderer.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	RealXRenderer rxr(false);
	rxr.Init(hInstance);
	rxr.Run();
	return 0;
}