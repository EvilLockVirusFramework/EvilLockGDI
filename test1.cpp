#include"draw.hpp"

int main() {
    Sleep(2000);
    IconDrawer drawer;
    drawer.setPenSpeed(5);
    
    // 正弦函数 y = A * sin(B * x)
   
    for (int i = 0; i < 8; i++)
    {
        drawer.turnRight(360 / 8);
        drawer.forward(200);
        drawer.drawCircle(100);
    }
    RECT rect;
    HDC hdcDesktop = GetDC(NULL);
    GetClientRect(WindowFromDC(hdcDesktop), &rect);
    int screenWidth = rect.right;
    int screenHeight = rect.bottom;
    float amplitude = 100.0f; // 振幅
    float frequency = 0.1f; // 频率
    for (int x = 0; x < screenWidth; ++x) {
        int y = static_cast<int>(screenHeight / 2 + amplitude * sin(frequency * x));
        drawer.DrawIcon(x, y);
    }
   //Y=a (x-h)²+k
    // 绘制爱心形状
    int centerX = screenWidth / 2;
    int centerY = screenHeight / 2;
    drawer.turnLeft(45);
    drawer.forward(300);
    drawer.drawArc(250, 180, 1);
    drawer.forward(620);
    drawer.gohome();
    drawer.turnRight(90);
    drawer.forward(250);
    drawer.drawArc(200, 180, -1);
    drawer.forward(650);  
    drawer.gotoPos(centerX-150, centerY +100);
    drawer.drawText("hope男神！爱你",100, RGB(255, 0, 0));  // 红色文字
    drawer.gotoPos(0, centerY);
    drawer.angle = 0;
    for (int i = 10; i <= 20; i++)
    {
        drawer.SetIconSize(i * 10, i * 10);
        drawer.forward(100);
    }
}
