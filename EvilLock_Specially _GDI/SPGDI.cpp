#include"SPGDI.h"
void  EvilLock_SPGDI::InitializeGDI(SPGDI mem)
{	//初始化虚拟设备上下文
	mem.MemGDI.hdcMem = CreateCompatibleDC(mem.screen);
	INT w = GetSystemMetrics(0), h = GetSystemMetrics(1);
	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	HBITMAP hbmTemp = CreateDIBSection(mem.screen, &bmi, NULL, (void**)&mem.MemGDI.RGBScreen, NULL, NULL);
	SelectObject(mem.MemGDI.hdcMem, hbmTemp);
	//初始化中心点位置
	mem.Pos.x = w / 2;
	mem.Pos.y = h / 2;
	//初始化角度
	mem.heading = 90;
	//初始化画笔状态
	mem.state = DOWN;
}