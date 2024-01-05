/*
#include"LayeredWindowGdi.hpp"
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    LayeredWindowGDI l(hInstance, 100, 100, 500, 500);
 
    l.Create();
    for (int execution = 0; execution < 10000; execution++) {
        BitBlt(l.hdcMem, 0, 0, l.windowWidth, l.windowHeight, l.hdcWindow, 0, 0, SRCCOPY);
        for (int i = 0; i < l.windowWidth * l.windowHeight; i++) {
            int x = i % l.windowWidth;
            int y = i / l.windowHeight;
            l.rgbScreen[i].rgb *= x ^ y;
        }
        BitBlt(l.hdcWindow, 0, 0, l.windowWidth, l.windowHeight, l.hdcMem, 0, 0, SRCCOPY);
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
