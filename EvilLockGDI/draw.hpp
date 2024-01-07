#pragma once
#include <windows.h>
#include <math.h>
#include <iostream>
constexpr float CCW = -1;//逆时针
constexpr float CW = 1;//顺时针 
constexpr float PI = 3.1415926;

class IconDrawer {
private:
    HDC hdc;
    HICON icon;
    bool penState;//画笔状态
    int sensitivity;//灵敏度（图标间隔）
    int penSpeed;//画笔速度
    POINT center;//画布中心

public:
    POINT position;
    float angle;
    IconDrawer(HDC hdc, HICON icon) : hdc(hdc), icon(icon), penState(true), sensitivity(10), penSpeed(10) {
        RECT rect;
        GetClientRect(WindowFromDC(hdc), &rect);
        center.x = rect.right / 2;
        center.y = rect.bottom / 2;
        position = center;
        angle = 0;
    }

    void forward(int distance) {
        int steps = distance / sensitivity;
        float stepX = distance * cos(angle) / steps;
        float stepY = distance * sin(angle) / steps;

        for (int i = 0; i < steps; i++) {
            position.x += stepX;
            position.y += stepY;
            if (penState) {
                DrawIcon(position.x, position.y);
            }
        }

        position.x += (distance - steps * sensitivity) * cos(angle);
        position.y += (distance - steps * sensitivity) * sin(angle);
        if (penState) {
            DrawIcon(position.x, position.y);
        }
    }

    void gotoPos(int newX, int newY) {
        position.x = newX;
        position.y = newY;
    }

    void rotate(float degrees) {
        angle += degrees * (PI / 180.0);
    }

    void turnLeft(float degrees) {
        angle -= degrees * (PI / 180.0);
    }

    void turnRight(float degrees) {
        angle += degrees * (PI / 180.0);
    }

    void backward(int distance) {
        rotate(180);
        forward(distance);
        rotate(180);
    }

    void setSensitivity(int newSensitivity) {
        sensitivity = newSensitivity;
    }

    void setPenSpeed(int newPenSpeed) {
        penSpeed = newPenSpeed;
    }

    void penUp() {
        penState = false;
    }

    void penDown() {
        penState = true;
    }

    void changeIcon(HICON newIcon) {
        icon = newIcon;
    }

    int getCenterX() const {
        return center.x;
    }

    int getCenterY() const {
        return center.y;
    }

    void drawCircle(int radius) {
        int centerX = position.x;
        int centerY = position.y;

        float angleIncrement = sensitivity / static_cast<float>(radius);

        for (float angle = 0.0; angle <= 2 * PI; angle += angleIncrement) {
            int xx = static_cast<int>(centerX + radius * cos(angle));
            int yy = static_cast<int>(centerY + radius * sin(angle));
            DrawIcon(xx, yy);
        }
    }

    void drawArc(int radius, float angle, float direction) {
        float arcLength = 2 * PI * radius * (angle / 360.0);
        int numSegments = static_cast<int>(arcLength / sensitivity);
        angle = direction * angle;
        float angleIncrement = angle / numSegments;

        for (int i = 0; i < numSegments; i++) {
            forward(sensitivity);
            rotate(angleIncrement);
        }
    }
    void drawText(const std::string& text, int fontSize, COLORREF textColor) {
        HFONT font = CreateFont(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
        HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, font));
        SetTextColor(hdc, textColor);
        RECT rect = { position.x, position.y, position.x + 100, position.y + 100 };
        DrawTextA(hdc, text.c_str(), -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

    void DrawIcon(int x, int y) {
       // std::cout << "X:" << x << " Y:" << y << std::endl;
        Sleep(penSpeed);
        DrawIconEx(hdc, x, y, icon, 0, 0, 0, NULL, DI_NORMAL);
    }
    void clearCanvas() {
        RECT rect;
        GetClientRect(WindowFromDC(hdc), &rect);
        int width = rect.right;
        int height = rect.bottom;

        HDC hdcDesktop = GetDC(NULL);
        BitBlt(hdc, 0, 0, width, height, hdcDesktop, 0, 0, SRCCOPY);
        ReleaseDC(NULL, hdcDesktop);
    }

};
HICON loadCustomIcon(HINSTANCE hInstance, LPCTSTR resourceName, int iconWidth, int iconHeight) {
    HICON hCustomIcon = LoadIcon(hInstance, resourceName);
    if (hCustomIcon == NULL) {
        return 0;
    }
    return hCustomIcon;
}