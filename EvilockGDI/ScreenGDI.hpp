#pragma once
#include <iostream>
#include <Windows.h>
#include"color.h"
class ScreenGDI {
public:
    HDC hdcDesktop;          //�����豸������
    HDC hdcMem;              // �ڴ��豸������
    int width;               // ���
    int height;              // �߶�
    HBITMAP hbmTemp;         // ��ʱλͼ
    PRGBQUAD rgbScreen;      // ��������

    ScreenGDI() {
        hdcDesktop = GetDC(NULL);  // ��ȡ�����豸������
        hdcMem = CreateCompatibleDC(hdcDesktop);
        width = GetSystemMetrics(SM_CXSCREEN);   // ��ȡ��Ļ���
        height = GetSystemMetrics(SM_CYSCREEN);  // ��ȡ��Ļ�߶�
      //  std::cout << "Width: " << width << std::endl;
       // std::cout << "Height: " << height << std::endl;

        // ������ʱλͼ��������������
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
        ReleaseDC(NULL, hdcDesktop);  // �ͷ������豸������
        DeleteDC(hdcMem);  // ɾ���ڴ��豸������
        DeleteObject(hbmTemp);        // ɾ����ʱλͼ
    }
    //�������� ��ΧΪ0.f��1.f
    void AdjustBrightness(float factor);
    //�����Աȶ� ��ΧΪ0.f��1.f
    void AdjustContrast(float factor);
    //�������Ͷ� ��ΧΪ0.f��1.f
    void AdjustSaturation(float factor);
    //---------------------------------------------
    //����ĳ����������RGB��ֵ�����ӣ����پ��ø���
    void AdjustRGB(int xStart, int yStart, int xEnd, int yEnd, int rIncrease, int gIncrease, int bIncrease); 
    //ֱ���趨ĳ����������RGB��ֵ
    void SetRGB(int xStart, int yStart, int xEnd, int yEnd, BYTE newR, BYTE newG, BYTE newB);
    void DrawImageToBitmap(HBITMAP hBitmap);
    void LoadAndDrawImageFromResource(int resourceID);
};
void ScreenGDI::DrawImageToBitmap(HBITMAP hBitmap) {
    // ����һ���봫��λͼ���ݵ���ʱ�ڴ��豸������
    HDC hdcBitmap = CreateCompatibleDC(hdcMem);

    // �������λͼѡ����ʱ�ڴ��豸������
    SelectObject(hdcBitmap, hBitmap);

    // ��ȡ����λͼ�Ŀ�Ⱥ͸߶�
    BITMAP bmpInfo;
    GetObject(hBitmap, sizeof(BITMAP), &bmpInfo);
    int bmpWidth = bmpInfo.bmWidth;
    int bmpHeight = bmpInfo.bmHeight;

    // ������λͼ���Ƶ���ʱλͼ��
    BitBlt(hdcMem, 0, 0, bmpWidth, bmpHeight, hdcBitmap, 0, 0, SRCCOPY);

    // �ͷ���ʱ�ڴ��豸������
    DeleteDC(hdcBitmap);
}
void ScreenGDI::AdjustRGB(int xStart, int yStart, int xEnd, int yEnd, int rIncrease, int gIncrease, int bIncrease) //ֱ������ĳ����������RGB��ֵ
{
    BitBlt(hdcMem, 0, 0, width, height, hdcDesktop, 0, 0, SRCCOPY);
    int startX = max(0, min(xStart, width - 1));    // ȷ��xStart����Ч��Χ��
    int startY = max(0, min(yStart, height - 1));   // ȷ��yStart����Ч��Χ��
    int endX = max(0, min(xEnd, width - 1));        // ȷ��xEnd����Ч��Χ��
    int endY = max(0, min(yEnd, height - 1));       // ȷ��yEnd����Ч��Χ��

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
void ScreenGDI::SetRGB(int xStart, int yStart, int xEnd, int yEnd, BYTE newR, BYTE newG, BYTE newB) //ֱ���趨ĳ����������RGB��ֵ
{
    BitBlt(hdcMem, 0, 0, width, height, hdcDesktop, 0, 0, SRCCOPY);
    int startX = max(0, min(xStart, width - 1));    // ȷ��xStart����Ч��Χ��
    int startY = max(0, min(yStart, height - 1));   // ȷ��yStart����Ч��Χ��
    int endX = max(0, min(xEnd, width - 1));        // ȷ��xEnd����Ч��Χ��
    int endY = max(0, min(yEnd, height - 1));       // ȷ��yEnd����Ч��Χ��

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

