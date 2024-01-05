#pragma once
#include <windows.h>
#include <tchar.h>
#include"color.h"
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
    void Create() {
        const wchar_t CLASS_NAME[] = L"EvilLock";

        WNDCLASS wc = { 0 };
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;

        RegisterClass(&wc);

        hWnd = CreateWindowEx(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            CLASS_NAME,
            L"YunChenqwq",
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
    }
    bool IsAtEdge(int deltaX, int deltaY) {
        RECT rect;
        GetClientRect(GetDesktopWindow(), &rect);

        if (xPos + deltaX <= 0 || xPos + deltaX + windowWidth > rect.right) {
            return true;
        }

        if (yPos + deltaY <= 0 || yPos + deltaY + windowHeight > rect.bottom) {
            return true;
        }

        return false;
    }

    void Move(int deltaX, int deltaY, bool isBounce) {
        if (isBounce && IsAtEdge(deltaX, deltaY)) {
            deltaX = -deltaX;
            deltaY = -deltaY;
        }

        xPos += deltaX;
        yPos += deltaY;
        MoveWindow(hWnd, xPos, yPos, windowWidth, windowHeight, TRUE);
    }


    void MoveUp(int distance, bool isBounce) {
        Move(0, -distance, isBounce);
    }

    void MoveDown(int distance, bool isBounce) {
        Move(0, distance, isBounce);
    }

    void MoveLeft(int distance, bool isBounce) {
        Move(-distance, 0, isBounce);
    }

    void MoveRight(int distance, bool isBounce) {
        Move(distance, 0, isBounce);
    }
    void StartFlicker(int executionTimes) {
        //  flickerExecutionTimes = executionTimes;
        SetTimer(hWnd, 1, 50, NULL);
    }

    void StopFlicker() {
        KillTimer(hWnd, 1);
    }

private:
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
    void ScreenFlicker() {
        HDC hdc = GetDC(hWnd);
        RECT rect;
        GetClientRect(hWnd, &rect);

        HBRUSH brush = CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255));
        SelectObject(hdc, brush);
        PatBlt(hdc, 0, 0, rect.right, rect.bottom, PATINVERT);
        DeleteObject(brush);

        ReleaseDC(hWnd, hdc);
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
            window->ScreenFlicker();

            break;
        }
        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;
        case WM_DESTROY:
            window->StopFlicker();
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        return 0;
    }
};
/*
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    LayeredWindowGDI window(hInstance, 100, 100, 500, 500);
    window.Create();
    window.StartFlicker(10);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        // ����Ϣѭ��֮��ִ���ƶ�����
        window.MoveRight(10, true);
        //   window.StopFlicker();
    }

    return 0;
}
*/
