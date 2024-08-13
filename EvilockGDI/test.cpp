#include "LayeredWindowGdi.hpp"
#include "resource.h"
#define PI acos(-1.0)

void RotateWindow(LayeredWindowGDI& window, float angle, POINT center = { -1, -1 }) {
    if (center.x == -1 && center.y == -1) {
        center.x = window.windowWidth / 2;
        center.y = window.windowHeight / 2;
    }

    POINT pt;
    pt.x = center.x;
    pt.y = center.y;

    SIZE sz;
    sz.cx = window.windowWidth;
    sz.cy = window.windowHeight;

    POINT ppt[3];
    angle = angle * (PI / 180);
    float sina = sin(angle);
    float cosa = cos(angle);
    ppt[0].x = pt.x + sina * pt.y - cosa * pt.x;
    ppt[0].y = pt.y - cosa * pt.y - sina * pt.x;
    ppt[1].x = ppt[0].x + cosa * sz.cx;
    ppt[1].y = ppt[0].y + sina * sz.cx;
    ppt[2].x = ppt[0].x - sina * sz.cy;
    ppt[2].y = ppt[0].y + cosa * sz.cy;

    window.MoveRight(10, 1);
    PlgBlt(window.hdcWindow, ppt, window.hdcWindow, 0, 0, window.windowWidth, window.windowHeight, 0, 0, 0);
}

int main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    LayeredWindowGDI p(hInstance, 100, 100, 800, 800);
   // p.Create();
    LayeredWindowGDI l(hInstance, 100, 100, 800, 800);
    l.Create();
    // 清空窗口l的内容
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, l.windowWidth, l.windowHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    BitBlt(l.hdcWindow, 0, 0, l.windowWidth, l.windowHeight, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    // 在这之后进行对窗口l的其他操作

    RECT rect;
    GetClipBox(l.hdcWindow, &rect);
    SetLayeredWindowAttributes(l.hWnd, RGB(0, 0, 0), 200, LWA_COLORKEY | LWA_ALPHA);
    l.LoadAndDrawImageFromFile("4.bmp");

    int angle = -10;
    for (int i = 0; i < 1000; ++i) {
        RotateWindow(l, angle); // 默认以窗口中心旋转
        RotateWindow(p, angle); // 默认以窗口中心旋转
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
