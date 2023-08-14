#include"SPGDI.h"
void  EvilLock_SPGDI::InitializeGDI(SPGDI mem)
{	//��ʼ�������豸������
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
	//��ʼ�����ĵ�λ��
	mem.Pos.x = w / 2;
	mem.Pos.y = h / 2;
	//��ʼ���Ƕ�
	mem.heading = 90;
	//��ʼ������״̬
	mem.state = DOWN;
}