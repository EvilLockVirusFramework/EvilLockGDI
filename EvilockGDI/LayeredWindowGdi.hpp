#pragma once
#include <windows.h>
#include <tchar.h>
#include"color.h"
#include<iostream>
#define BOUNCE 1//自动反弹
#define STOP 2// 停止移动
#define CONTINUE 3// 继续移动
class LayeredWindowGDI {
public:
    HWND hWnd;
    HINSTANCE hInstance;
    int xPos;
    int yPos;
    int windowWidth;
    int windowHeight;
    HDC hdcWindow;          //窗口上下文
    HDC hdcMem;              // 内存设备上下文
    HBITMAP hbmTemp;         // 临时位图
    PRGBQUAD rgbScreen;      // 像素数组

    LayeredWindowGDI(HINSTANCE hInstance, int x, int y, int width, int height)

        : hInstance(hInstance), xPos(x), yPos(y), windowWidth(width), windowHeight(height) {

    }
    ~LayeredWindowGDI()
    {
        ReleaseDC(NULL, hdcWindow);  // 释放桌面设备上下文
        DeleteDC(hdcMem);  // 删除内存设备上下文
        DeleteObject(hbmTemp);        // 删除临时位图
    }
    void Create(int keep=0) {
        const char CLASS_NAME[] = "EvilLock"; 

        WNDCLASSA wc = { 0 }; 
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;

        RegisterClassA(&wc);

        hWnd = CreateWindowExA(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            CLASS_NAME,
            "YunChenqwq", 
            WS_POPUP,
            xPos, yPos, windowWidth, windowHeight,
            NULL,
            NULL,
            hInstance,
            this
        );

        if (hWnd == NULL) {
            return;
        }
        initialization();
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);
        if (keep == 0)
        {
            // 因为创建时清空窗口窗口会被背景影响，通过此可以清空窗口内容，但是保留就可以实现一个整个窗口连带着背景旋转的效果，故此提供保留选项
            HDC hdcScreen = GetDC(NULL);
            HDC hdcMem = CreateCompatibleDC(hdcScreen);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, windowWidth, windowHeight);
            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

            BitBlt(hdcWindow, 0, 0, windowWidth, windowHeight, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hdcMem);
            ReleaseDC(NULL, hdcScreen);
        }
    }

    int IsAtEdge(int deltaX, int deltaY, int mode) {
        RECT rect;
        GetClientRect(GetDesktopWindow(), &rect);

        if (xPos + deltaX <= 0 || xPos + deltaX + windowWidth > rect.right || yPos + deltaY <= 0 || yPos + deltaY + windowHeight > rect.bottom) { 
            if (mode == BOUNCE) {
                return 1; // 碰到边缘，自动反弹
            }
            if (mode == STOP) {
                return 2; // 碰到边缘，自动停止移动
              
            }
        }

        return 0; // 不碰到边缘
    }

    void Move(int deltaX, int deltaY, int mode) {
        RECT rect;
        GetClientRect(GetDesktopWindow(), &rect);

        if (hasCollided) {
            deltaX = -deltaX; 
            deltaY = -deltaY;
            int edge = IsAtEdge(deltaX, deltaY, mode);
          //  std::cout << edge;
            if (edge == 1) {
                hasCollided = false; // 重置碰撞标志
            }
            if (edge == 2) {
                return;
            }

        }
        else {
            int edge = IsAtEdge(deltaX, deltaY, mode);
            if (edge == 1) 
            {
                if (mode == BOUNCE) {
                    hasCollided = true; // 设置碰撞标志
                }
            }
            else if (edge == 2) {
                return;
            }

          
        }

        xPos += deltaX;
        yPos += deltaY;
        MoveWindow(hWnd, xPos, yPos, windowWidth, windowHeight, TRUE);
    }


    void MoveUp(int distance, int mode) {
        Move(0, -distance, mode);
    }

    void MoveDown(int distance, int mode) {
        Move(0, distance, mode);
    }

    void MoveLeft(int distance, int mode) {
        Move(-distance, 0, mode);
    }

    void MoveRight(int distance, int mode) {
        Move(distance, 0, mode);
    }
    void Rotate(float angle, float zoomX=1, float zoomY=1, int offsetX=0, int offsetY=0, POINT center = { -1, -1 }) {
        if (center.x == -1 && center.y == -1) {
            center.x = windowWidth / 2;
            center.y = windowHeight / 2;
        }

        POINT pt;
        pt.x = center.x;
        pt.y = center.y;

        SIZE sz;
        sz.cx = windowWidth;
        sz.cy = windowHeight;

        POINT ppt[3];
        angle = angle * (PI / 180);
        float sina = sin(angle);
        float cosa = cos(angle);
        ppt[0].x = pt.x + sina * pt.y - cosa * pt.x * zoomX + offsetX;
        ppt[0].y = pt.y - cosa * pt.y - sina * pt.x * zoomY + offsetY;
        ppt[1].x = ppt[0].x + cosa * sz.cx * zoomX + offsetX;
        ppt[1].y = ppt[0].y + sina * sz.cx * zoomY + offsetY;
        ppt[2].x = ppt[0].x - sina * sz.cy * zoomX + offsetX;
        ppt[2].y = ppt[0].y + cosa * sz.cy * zoomY + offsetY;
        PlgBlt(hdcWindow, ppt, hdcWindow, 0, 0, windowWidth, windowHeight, 0, 0, 0);
    }
    void turnLeft(float angle) {
        Rotate(-angle, 1, 1, 0, 0, { -1, -1 });
    }

    void turnRight(float angle) {
        Rotate(angle, 1, 1, 0, 0, { -1, -1 });
    }

    void DrawImageToBitmap(HBITMAP hBitmap) {
        // 创建一个与传入位图兼容的临时内存设备上下文
        HDC hdcBitmap = CreateCompatibleDC(hdcMem);

        // 将传入的位图选入临时内存设备上下文
        SelectObject(hdcBitmap, hBitmap);

        // 获取传入位图的宽度和高度
        BITMAP bmpInfo;
        GetObject(hBitmap, sizeof(BITMAP), &bmpInfo);
        int bmpWidth = bmpInfo.bmWidth;
        int bmpHeight = bmpInfo.bmHeight;

        // 将传入位图绘制到临时位图中
        BitBlt(hdcMem, 0, 0, bmpWidth, bmpHeight, hdcBitmap, 0, 0, SRCCOPY);

        // 释放临时内存设备上下文
        DeleteDC(hdcBitmap);
    }
    void LoadAndDrawImageFromResource(int resourceID) {
        // 加载资源图片
        HBITMAP hBitmap = reinterpret_cast<HBITMAP>(LoadImage(NULL, MAKEINTRESOURCE(resourceID), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));

        if (hBitmap == NULL) {
            DWORD dwError = GetLastError();
          //  std::cerr << "Failed to load image. Error code: " << dwError << std::endl;
            return;
        }

        // 将图片绘制到位图上
        DrawImageToBitmap(hBitmap);

        // 释放位图资源
        DeleteObject(hBitmap);
    }

    void LoadAndDrawImageFromFile(const char* filePath) {
        // 加载位图文件
        HBITMAP hBitmap = (HBITMAP)LoadImageA(NULL, filePath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

        if (hBitmap == NULL) {
            // 加载图像失败
            DWORD dwError = GetLastError();
            // 输出错误信息
            std::cerr << "Failed to load image. Error code: " << dwError << std::endl;
            return;
        }

        // 获取窗口客户区域大小
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int clientWidth = clientRect.right - clientRect.left;
        int clientHeight = clientRect.bottom - clientRect.top;

        // 获取位图大小
        BITMAP bmpInfo;
        GetObject(hBitmap, sizeof(BITMAP), &bmpInfo);
        int bmpWidth = bmpInfo.bmWidth;
        int bmpHeight = bmpInfo.bmHeight;

        // 计算居中位置
        int startX = (clientWidth - bmpWidth) / 2;
        int startY = (clientHeight - bmpHeight) / 2;
        std::cout << startX << " " << startY << std::endl;
        // 创建设备上下文
        HDC hdc = GetDC(hWnd);
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

        // 将图片绘制到位图上
        BitBlt(hdc, startX, startY, bmpWidth, bmpHeight, hdcMem, 0, 0, SRCCOPY);

        // 释放资源
        SelectObject(hdcMem, hOldBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(hWnd, hdc);
        DeleteObject(hBitmap);
    }
    void AdjustBrightness(float factor) {
        // 备份窗口内容
        BitBlt(hdcMem, 0, 0, windowWidth, windowHeight, hdcWindow, 0, 0, SRCCOPY);

        // 调整亮度
        for (int i = 0; i < windowWidth * windowHeight; i++) {
            HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
            hsl.l *= factor;
            _RGBQUAD rgb = HSLToRGB(hsl);
            rgbScreen[i].r = rgb.r;
            rgbScreen[i].g = rgb.g;
            rgbScreen[i].b = rgb.b;
        }

        // 将修改后的内容应用到窗口
        BitBlt(hdcWindow, 0, 0, windowWidth, windowHeight, hdcMem, 0, 0, SRCCOPY);
    }

    void AdjustContrast(float factor) {
        // 备份窗口内容
        BitBlt(hdcMem, 0, 0, windowWidth, windowHeight, hdcWindow, 0, 0, SRCCOPY);

        // 调整对比度
        for (int i = 0; i < windowWidth * windowHeight; i++) {
            HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
            hsl.l = 0.5f + (hsl.l - 0.5f) * factor;
            _RGBQUAD rgb = HSLToRGB(hsl);
            rgbScreen[i].r = rgb.r;
            rgbScreen[i].g = rgb.g;
            rgbScreen[i].b = rgb.b;
        }

        // 将修改后的内容应用到窗口
        BitBlt(hdcWindow, 0, 0, windowWidth, windowHeight, hdcMem, 0, 0, SRCCOPY);
    }

    void AdjustSaturation(float factor) {
        // 备份窗口内容
        BitBlt(hdcMem, 0, 0, windowWidth, windowHeight, hdcWindow, 0, 0, SRCCOPY);

        // 调整饱和度
        for (int i = 0; i < windowWidth * windowHeight; i++) {
            HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
            hsl.s *= factor;
            _RGBQUAD rgb = HSLToRGB(hsl);
            rgbScreen[i].r = rgb.r;
            rgbScreen[i].g = rgb.g;
            rgbScreen[i].b = rgb.b;
        }

        // 将修改后的内容应用到窗口
        BitBlt(hdcWindow, 0, 0, windowWidth, windowHeight, hdcMem, 0, 0, SRCCOPY);
    }



private:
    bool hasCollided = false; // 标志是否已经发生碰撞
    void initialization()
    {
        hdcWindow = GetDC(hWnd);  // 获取桌面设备上下文
        hdcMem = CreateCompatibleDC(hdcWindow);
        //  std::cout << "Width: " << width << std::endl;
         // std::cout << "Height: " << height << std::endl;

          // 创建临时位图并关联像素数组
        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biWidth = windowWidth;
        bmi.bmiHeader.biHeight = windowHeight;
        hbmTemp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (void**)&rgbScreen, NULL, 0);
        SelectObject(hdcMem, hbmTemp);
    }

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        LayeredWindowGDI* window = reinterpret_cast<LayeredWindowGDI*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

        switch (msg) {
        case WM_CREATE: {
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
            window = reinterpret_cast<LayeredWindowGDI*>(((CREATESTRUCT*)lParam)->lpCreateParams);

            SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
            SetLayeredWindowAttributes(hWnd, 0, 200, LWA_ALPHA);

            COLORREF colorKey = RGB(0, 0, 0);
            SetLayeredWindowAttributes(hWnd, colorKey, 0, LWA_COLORKEY);

            break;
        }
        case WM_TIMER: {

            break;
        }
        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        return 0;
    }
};