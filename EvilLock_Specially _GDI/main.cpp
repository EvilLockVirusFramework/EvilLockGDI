#include"type.h"
void ApplyDesktopEffect()
{
    GDI hdc;

    // 在这里进行你想要的操作，例如绘制图形、绘制文本、填充颜色等等
    RECT rect;
    GetClientRect(GetDesktopWindow(), &rect);
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0));  // 创建一个红色刷子
    FillRect(hdc, &rect, hBrush);  // 填充整个桌面矩形
    DeleteObject(hBrush);  // 删除刷子对象

    ReleaseDC(NULL, hdc);  // 释放设备上下文
}

int main()
{
    ApplyDesktopEffect();

    return 0;
}
