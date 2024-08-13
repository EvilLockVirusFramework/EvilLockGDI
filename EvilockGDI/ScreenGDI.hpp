#pragma once
#include <iostream>
#include <Windows.h>
#include"color.h"
class ScreenGDI {
public:
    HDC hdcDesktop;          //桌面设备上下文
    HDC hdcMem;              // 内存设备上下文
    int width;               // 宽度
    int height;              // 高度
    HBITMAP hbmTemp;         // 临时位图
    PRGBQUAD rgbScreen;      // 像素数组

    ScreenGDI() {
        hdcDesktop = GetDC(NULL);  // 获取桌面设备上下文
        hdcMem = CreateCompatibleDC(hdcDesktop);
        width = GetSystemMetrics(SM_CXSCREEN);   // 获取屏幕宽度
        height = GetSystemMetrics(SM_CYSCREEN);  // 获取屏幕高度
      //  std::cout << "Width: " << width << std::endl;
       // std::cout << "Height: " << height << std::endl;

        // 创建临时位图并关联像素数组
        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = height;
        hbmTemp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (void**)&rgbScreen, NULL, 0);
        SelectObject(hdcMem, hbmTemp);
        //std::cout << "Temp Bitmap: " << hbmTemp << std::endl;
       // std::cout << "Pixel Array: " << (void*)rgbScreen << std::endl;
    }

    ~ScreenGDI() {
        ReleaseDC(NULL, hdcDesktop);  // 释放桌面设备上下文
        DeleteDC(hdcMem);  // 删除内存设备上下文
        DeleteObject(hbmTemp);        // 删除临时位图
    }
    //调整亮度 范围为0.f～1.f
    void AdjustBrightness(float factor);
    //调整对比度 范围为0.f～1.f
    void AdjustContrast(float factor);
    //调整饱和度 范围为0.f～1.f
    void AdjustSaturation(float factor);
    //---------------------------------------------
    //调整某个矩形区域RGB的值（增加）减少就用负数
    void AdjustRGB(int xStart, int yStart, int xEnd, int yEnd, int rIncrease, int gIncrease, int bIncrease); 
    //直接设定某个矩形区域RGB的值
    void SetRGB(int xStart, int yStart, int xEnd, int yEnd, BYTE newR, BYTE newG, BYTE newB);
    void DrawImageToBitmap(HBITMAP hBitmap);
    void LoadAndDrawImageFromResource(int resourceID);
};
void ScreenGDI::DrawImageToBitmap(HBITMAP hBitmap) {
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
void ScreenGDI::AdjustRGB(int xStart, int yStart, int xEnd, int yEnd, int rIncrease, int gIncrease, int bIncrease) //直接增加某个矩形区域RGB的值
{
    BitBlt(hdcMem, 0, 0, width, height, hdcDesktop, 0, 0, SRCCOPY);
    int startX = max(0, min(xStart, width - 1));    // 确保xStart在有效范围内
    int startY = max(0, min(yStart, height - 1));   // 确保yStart在有效范围内
    int endX = max(0, min(xEnd, width - 1));        // 确保xEnd在有效范围内
    int endY = max(0, min(yEnd, height - 1));       // 确保yEnd在有效范围内

    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            int index = y * width + x;
            BYTE newR = min(255, max(0, rgbScreen[index].r + rIncrease));
            BYTE newG = min(255, max(0, rgbScreen[index].g + gIncrease));
            BYTE newB = min(255, max(0, rgbScreen[index].b + bIncrease));
            rgbScreen[index].r = newR;
            rgbScreen[index].g = newG;
            rgbScreen[index].b = newB;
        }
    }
    BitBlt(hdcDesktop, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
}
void ScreenGDI::SetRGB(int xStart, int yStart, int xEnd, int yEnd, BYTE newR, BYTE newG, BYTE newB) //直接设定某个矩形区域RGB的值
{
    BitBlt(hdcMem, 0, 0, width, height, hdcDesktop, 0, 0, SRCCOPY);
    int startX = max(0, min(xStart, width - 1));    // 确保xStart在有效范围内
    int startY = max(0, min(yStart, height - 1));   // 确保yStart在有效范围内
    int endX = max(0, min(xEnd, width - 1));        // 确保xEnd在有效范围内
    int endY = max(0, min(yEnd, height - 1));       // 确保yEnd在有效范围内

    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            int index = y * width + x;
            rgbScreen[index].r = newR;
            rgbScreen[index].g = newG;
            rgbScreen[index].b = newB;
        }
    }
    BitBlt(hdcDesktop, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
}
void ScreenGDI::AdjustBrightness(float factor) {
    BitBlt(hdcMem, 0, 0, width, height, hdcDesktop, 0, 0, SRCCOPY);
    for (int i = 0; i < width * height; i++) {
        HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
        hsl.l *= factor;
        _RGBQUAD rgb = HSLToRGB(hsl);
        rgbScreen[i].r = rgb.r;
        rgbScreen[i].g = rgb.g;
        rgbScreen[i].b = rgb.b;
    }

    BitBlt(hdcDesktop, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
}

void ScreenGDI::AdjustContrast(float factor) {
    BitBlt(hdcMem, 0, 0, width, height, hdcDesktop, 0, 0, SRCCOPY);
    for (int i = 0; i < width * height; i++) {
        HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
        hsl.l = 0.5f + (hsl.l - 0.5f) * factor;
        _RGBQUAD rgb = HSLToRGB(hsl);
        rgbScreen[i].r = rgb.r;
        rgbScreen[i].g = rgb.g;
        rgbScreen[i].b = rgb.b;
    }

    BitBlt(hdcDesktop, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
}

void ScreenGDI::AdjustSaturation(float factor) {
    BitBlt(hdcMem, 0, 0, width, height, hdcDesktop, 0, 0, SRCCOPY);
    for (int i = 0; i < width * height; i++) {
        HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
        hsl.s *= factor;
        _RGBQUAD rgb = HSLToRGB(hsl);
        rgbScreen[i].r = rgb.r;
        rgbScreen[i].g = rgb.g;
        rgbScreen[i].b = rgb.b;
    }

    BitBlt(hdcDesktop, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
}

