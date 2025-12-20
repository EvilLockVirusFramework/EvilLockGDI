#include <Windows.h>
#include <iostream>
#include "draw.hpp"

// 窗口过程函数
LRESULT CALLBACK MyWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // 创建绘图器对象
        Pen drawer(hdc, NULL);

        // 设置绘制参数
        drawer.setSensitivity(5);
        drawer.setPenSpeed(3);

        // 加载错误图标
        HICON errorIcon = LoadIcon(NULL, IDI_ERROR);

        if (errorIcon) {
            // 设置使用错误图标
            drawer.changeIcon(errorIcon);

            // 抬起画笔移动到中心
            drawer.setPenStatus(ICON_PEN_UP);
            drawer.setStartPos(400, 300);
            drawer.setAngle(180);  // 设置角度让爱心正着 至于原因是为什么，那是因为海龟的坐标系和我在Windows依据窗口创建的坐标系不同

            // 落下画笔开始画爱心
            drawer.setPenStatus(ICON_PEN_DOWN);

            // 画爱心轮廓
            drawer.left(50);
            drawer.forward(133);
            drawer.drawArc(50, 200, CW);
            drawer.right(140);
            drawer.drawArc(50, 200, CW);
            drawer.forward(133);

            // 抬起画笔准备填充
            drawer.setPenStatus(ICON_PEN_UP);
            drawer.setStartPos(400, 300);
            drawer.setAngle(180);
            drawer.setPenStatus(ICON_PEN_DOWN);

            // 开始填充爱心
            drawer.beginFill(errorIcon, 15);

            // 再画一遍爱心形状用于填充
            drawer.left(50);
            drawer.forward(133);
            drawer.drawArc(50, 200, CW);
            drawer.right(140);
            drawer.drawArc(50, 200, CW);
            drawer.forward(133);

            // 结束填充
            drawer.endFill();

            // 清除画布

            // 直接绘制HOPE文字（不拆分字母）
            drawer.drawTextWithIcons(L"HOPE", 200, 250, 2.0f, 20, 6, L"Arial", 100);
        }

        EndPaint(hWnd, &ps);
    }
    else if (msg == WM_DESTROY) {
        PostQuitMessage(0);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main() {
    // 1. 创建窗口
    HINSTANCE hInstance = GetModuleHandle(NULL);
    const char* className = "MyWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = MyWindowProc;  // 使用普通函数，不用lambda
    wc.hInstance = hInstance;
    wc.lpszClassName = className;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    // 创建大窗口（全屏大小减去边距）
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HWND hWnd = CreateWindow(className, "初学者绘图演示",
        WS_OVERLAPPEDWINDOW, 50, 50,
        screenWidth - 100, screenHeight - 100,  // 更大的窗口
        NULL, NULL, hInstance, NULL);

    if (hWnd) {
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);

        // 强制重绘
        InvalidateRect(hWnd, NULL, TRUE);

        std::cout << "程序开始运行！" << std::endl;
        std::cout << "会依次显示：" << std::endl;
        std::cout << "1. 爱心轮廓" << std::endl;
        std::cout << "2. 爱心填充" << std::endl;
        std::cout << "3. HOPE文字" << std::endl;
        std::cout << "关闭窗口退出程序" << std::endl;

        // 消息循环
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}
