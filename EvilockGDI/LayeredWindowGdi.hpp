#pragma once
#include <windows.h>
#include <tchar.h>
#include"color.h"
#include<iostream>
#define BOUNCE 1//�Զ�����
#define STOP 2// ֹͣ�ƶ�
#define CONTINUE 3// �����ƶ�
class LayeredWindowGDI {
public:
    HWND hWnd;
    HINSTANCE hInstance;
    int xPos;
    int yPos;
    int windowWidth;
    int windowHeight;
    HDC hdcWindow;          //����������
    HDC hdcMem;              // �ڴ��豸������
    HBITMAP hbmTemp;         // ��ʱλͼ
    PRGBQUAD rgbScreen;      // ��������

    LayeredWindowGDI(HINSTANCE hInstance, int x, int y, int width, int height)

        : hInstance(hInstance), xPos(x), yPos(y), windowWidth(width), windowHeight(height) {

    }
    ~LayeredWindowGDI()
    {
        ReleaseDC(NULL, hdcWindow);  // �ͷ������豸������
        DeleteDC(hdcMem);  // ɾ���ڴ��豸������
        DeleteObject(hbmTemp);        // ɾ����ʱλͼ
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
            // ��Ϊ����ʱ��մ��ڴ��ڻᱻ����Ӱ�죬ͨ���˿�����մ������ݣ����Ǳ����Ϳ���ʵ��һ���������������ű�����ת��Ч�����ʴ��ṩ����ѡ��
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
                return 1; // ������Ե���Զ�����
            }
            if (mode == STOP) {
                return 2; // ������Ե���Զ�ֹͣ�ƶ�
              
            }
        }

        return 0; // ��������Ե
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
                hasCollided = false; // ������ײ��־
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
                    hasCollided = true; // ������ײ��־
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
    void LoadAndDrawImageFromResource(int resourceID) {
        // ������ԴͼƬ
        HBITMAP hBitmap = reinterpret_cast<HBITMAP>(LoadImage(NULL, MAKEINTRESOURCE(resourceID), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));

        if (hBitmap == NULL) {
            DWORD dwError = GetLastError();
          //  std::cerr << "Failed to load image. Error code: " << dwError << std::endl;
            return;
        }

        // ��ͼƬ���Ƶ�λͼ��
        DrawImageToBitmap(hBitmap);

        // �ͷ�λͼ��Դ
        DeleteObject(hBitmap);
    }

    void LoadAndDrawImageFromFile(const char* filePath) {
        // ����λͼ�ļ�
        HBITMAP hBitmap = (HBITMAP)LoadImageA(NULL, filePath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

        if (hBitmap == NULL) {
            // ����ͼ��ʧ��
            DWORD dwError = GetLastError();
            // ���������Ϣ
            std::cerr << "Failed to load image. Error code: " << dwError << std::endl;
            return;
        }

        // ��ȡ���ڿͻ������С
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int clientWidth = clientRect.right - clientRect.left;
        int clientHeight = clientRect.bottom - clientRect.top;

        // ��ȡλͼ��С
        BITMAP bmpInfo;
        GetObject(hBitmap, sizeof(BITMAP), &bmpInfo);
        int bmpWidth = bmpInfo.bmWidth;
        int bmpHeight = bmpInfo.bmHeight;

        // �������λ��
        int startX = (clientWidth - bmpWidth) / 2;
        int startY = (clientHeight - bmpHeight) / 2;
        std::cout << startX << " " << startY << std::endl;
        // �����豸������
        HDC hdc = GetDC(hWnd);
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

        // ��ͼƬ���Ƶ�λͼ��
        BitBlt(hdc, startX, startY, bmpWidth, bmpHeight, hdcMem, 0, 0, SRCCOPY);

        // �ͷ���Դ
        SelectObject(hdcMem, hOldBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(hWnd, hdc);
        DeleteObject(hBitmap);
    }
    void AdjustBrightness(float factor) {
        // ���ݴ�������
        BitBlt(hdcMem, 0, 0, windowWidth, windowHeight, hdcWindow, 0, 0, SRCCOPY);

        // ��������
        for (int i = 0; i < windowWidth * windowHeight; i++) {
            HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
            hsl.l *= factor;
            _RGBQUAD rgb = HSLToRGB(hsl);
            rgbScreen[i].r = rgb.r;
            rgbScreen[i].g = rgb.g;
            rgbScreen[i].b = rgb.b;
        }

        // ���޸ĺ������Ӧ�õ�����
        BitBlt(hdcWindow, 0, 0, windowWidth, windowHeight, hdcMem, 0, 0, SRCCOPY);
    }

    void AdjustContrast(float factor) {
        // ���ݴ�������
        BitBlt(hdcMem, 0, 0, windowWidth, windowHeight, hdcWindow, 0, 0, SRCCOPY);

        // �����Աȶ�
        for (int i = 0; i < windowWidth * windowHeight; i++) {
            HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
            hsl.l = 0.5f + (hsl.l - 0.5f) * factor;
            _RGBQUAD rgb = HSLToRGB(hsl);
            rgbScreen[i].r = rgb.r;
            rgbScreen[i].g = rgb.g;
            rgbScreen[i].b = rgb.b;
        }

        // ���޸ĺ������Ӧ�õ�����
        BitBlt(hdcWindow, 0, 0, windowWidth, windowHeight, hdcMem, 0, 0, SRCCOPY);
    }

    void AdjustSaturation(float factor) {
        // ���ݴ�������
        BitBlt(hdcMem, 0, 0, windowWidth, windowHeight, hdcWindow, 0, 0, SRCCOPY);

        // �������Ͷ�
        for (int i = 0; i < windowWidth * windowHeight; i++) {
            HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
            hsl.s *= factor;
            _RGBQUAD rgb = HSLToRGB(hsl);
            rgbScreen[i].r = rgb.r;
            rgbScreen[i].g = rgb.g;
            rgbScreen[i].b = rgb.b;
        }

        // ���޸ĺ������Ӧ�õ�����
        BitBlt(hdcWindow, 0, 0, windowWidth, windowHeight, hdcMem, 0, 0, SRCCOPY);
    }



private:
    bool hasCollided = false; // ��־�Ƿ��Ѿ�������ײ
    void initialization()
    {
        hdcWindow = GetDC(hWnd);  // ��ȡ�����豸������
        hdcMem = CreateCompatibleDC(hdcWindow);
        //  std::cout << "Width: " << width << std::endl;
         // std::cout << "Height: " << height << std::endl;

          // ������ʱλͼ��������������
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