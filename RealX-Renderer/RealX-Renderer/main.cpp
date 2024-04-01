#include "RealXRenderer.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	RealXRenderer rxr(true, PrimitiveType::Square);
	rxr.Init(hInstance);
	rxr.Run();
	return 0;
}