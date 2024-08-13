#include"LayeredWindowGdi.hpp"
#include"ScreenGDI.hpp"
int main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    /*
    LayeredWindowGDI l(hInstance, 100, 100, 500, 500);
    l.Create(2);
    l.LoadAndDrawImageFromFile("4.bmp");
    l.MoveRight(1000,1);
    int angle = 45;
    for (int i = 0; i < 1000; ++i) {
        l.Rotate(10);
        l.AdjustContrast(1.001);
        Sleep(100);
    }
    return 0;
    */
    ScreenGDI s;
    while (1)
        s.AdjustSaturation(1.01);
}