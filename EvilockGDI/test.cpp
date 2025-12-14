#include "LayeredWindowGDI.hpp"




int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow
)
{
	LayeredWindowGDI l(hInstance, 100, 100, 800, 800);
	l.Create();

	HDC hdcScreen = GetDC(NULL);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(
		hdcScreen, l.windowWidth, l.windowHeight);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

	BitBlt(l.hdcWindow, 0, 0, l.windowWidth, l.windowHeight,
		hdcMem, 0, 0, SRCCOPY);

	SelectObject(hdcMem, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);

	RECT rect;
	GetClipBox(l.hdcWindow, &rect);
	SetLayeredWindowAttributes(l.hWnd, RGB(0, 0, 0), 200,
		LWA_COLORKEY | LWA_ALPHA);
	l.LoadAndDrawImageFromFile("4.bmp");

	int angle = -10;
	for (int i = 0; i < 1000; ++i) {
		l.Shake();
		l.Rotate(35);
		Sleep(100);
	}

	return 0;
}

/*
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	LayeredWindowGDI l(hInstance, 100, 100, 500, 500);
	LayeredWindowGDI l2(hInstance, 100, 100, 500, 500);
	l.Create();
	l2.Create();
	for (int execution = 0; execution < 10000; execution++) {
		BitBlt(l.hdcMem, 0, 0, l.windowWidth, l.windowHeight, l.hdcWindow, 0, 0, SRCCOPY);
		for (int i = 0; i < l.windowWidth * l.windowHeight; i++) {
			int x = i % l.windowWidth;
			int y = i / l.windowHeight;
			l.rgbScreen[i].rgb *= x ^ y;
		}
		BitBlt(l.hdcWindow, 0, 0, l.windowWidth, l.windowHeight, l.hdcMem, 0, 0, SRCCOPY);
		l.MoveDown(1, 2);
	   l.MoveRight(1,1);
	   BitBlt(l2.hdcMem, 0, 0, l2.windowWidth, l2.windowHeight, l2.hdcWindow, 0, 0, SRCCOPY);
	   for (int i = 0; i < l2.windowWidth * l2.windowHeight; i++) {
		   int x = i % l2.windowWidth;
		   int y = i / l2.windowHeight;
		   l2.rgbScreen[i].rgb *= x ^ y;
	   }
	   BitBlt(l2.hdcWindow, 0, 0, l2.windowWidth, l2.windowHeight, l2.hdcMem, 0, 0, SRCCOPY);
	   l2.MoveUp(10, 2);
	   l2.MoveRight(10, 1);
	}
	return 0;
}
*/
/*
void HuaPing1(int executionTimes) {
	EvilLockGDI l;

	for (int execution = 0; execution < executionTimes; execution++) {
		BitBlt(l.hdcMem, 0, 0, l.width, l.height, l.hdcDesktop, 0, 0, SRCCOPY);
		for (int i = 0; i < l.width * l.height; i++) {
			int x = i % l.width;
			int y = i / l.width;
			l.rgbScreen[i].rgb *= x ^ y;
		}
		BitBlt(l.hdcDesktop, 0, 0, l.width, l.height, l.hdcMem, 0, 0, SRCCOPY);
	}
}
*/
