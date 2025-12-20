#pragma once
#include "common.hpp"
#include "color.hpp"
#include "gdi_raii.hpp"

class LayeredWindowGDI {
public:
    HWND hWnd;													///< 分层窗口句柄
    HINSTANCE hInstance;										///< 程序实例句柄
    int xPos;													///< X 坐标
    int yPos;													///< Y 坐标
    int windowWidth;											///< 窗口宽
    int windowHeight;											///< 窗口高
    HDC hdcWindow;												///< 窗口上下文
    HDC hdcMem;													///< 内存设备上下文
    HBITMAP hbmTemp;											///< 临时位图
    HGDIOBJ oldTempBmp;                                         ///< hdcMem 原先选入的位图（析构时还原）
    PRGBQUAD rgbScreen;											///< 像素数组

    /**
     * @brief				初始化分层窗口类
     *
     * @param[in]			hInstance								程序实例句柄
     * @param[in]			x										X 坐标
     * @param[in]			y										Y 坐标
     * @param[in]			width									窗口宽度
     * @param[in]			height									窗口高度
     */
    LayeredWindowGDI(
        HINSTANCE hInstance,
        int x, int y, int width, int height
    )
        : hWnd(nullptr), hInstance(hInstance),
        xPos(x), yPos(y),
        windowWidth(width), windowHeight(height),
        hdcWindow(nullptr), hdcMem(nullptr),
        hbmTemp(nullptr), oldTempBmp(nullptr), rgbScreen(nullptr) {
    }

    /**
     * @brief				释放 GDI 资源
     * @details				noexcept
     *
     */
    ~LayeredWindowGDI() noexcept
    {
        // 重要：删除位图前先把 DC 里的“旧对象”还原出来
        if (hdcMem && oldTempBmp) {
            SelectObject(hdcMem, oldTempBmp);
        }
        if (hbmTemp) DeleteObject(hbmTemp);					///< 删除临时位图
        if (hdcMem) DeleteDC(hdcMem);							///< 删除内存设备上下文
        if (hdcWindow && hWnd) ReleaseDC(hWnd, hdcWindow);		///< 释放窗口设备上下文（必须配对同一个 hWnd）
    }

    /**
     * @brief				创建分层窗口
     *
     * @param[in]			className								窗口类名,默认为 "EvilLock"
     * @param[in]			windowTitle								窗口标题,默认为 "YunChenqwq"
     * @param[in]			keep									是否清空背景,默认为 true
     */
    void Create(
        const std::string className = "EvilLock",
        const std::string windowTitle = "YunChenqwq",
        bool keep = true
    )
    {
        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = className.c_str();

        RegisterClassA(&wc);

        hWnd = CreateWindowExA(
            WS_EX_LAYERED | WS_EX_TRANSPARENT |
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            className.c_str(), windowTitle.c_str(), WS_POPUP,
            xPos, yPos, windowWidth, windowHeight,
            nullptr, nullptr, hInstance, this
        );

        if (hWnd == nullptr) return;
        initialization();
        ShowWindow(hWnd, SW_SHOW);
        UpdateWindow(hWnd);

        if (keep)
        {
            evgdi::win::window_dc hdcScreen(nullptr);
            if (!hdcScreen) return;

            evgdi::win::unique_hdc tempDC(CreateCompatibleDC(hdcScreen.get()));
            if (!tempDC) return;

            evgdi::win::unique_hbitmap hBitmap(CreateCompatibleBitmap(hdcScreen.get(), windowWidth, windowHeight));
            if (!hBitmap) return;

            evgdi::win::select_object_guard bmpSel(tempDC.get(), hBitmap.get());
            BitBlt(hdcWindow, 0, 0, windowWidth, windowHeight, tempDC.get(), 0, 0, SRCCOPY);
        }
    }

    /**
     * @brief				判断窗口是否碰到屏幕边缘
     *
     * @param				deltaX									窗口将要移动的X轴距离
     * @param				deltaY									窗口将要移动的Y轴距离
     * @param				mode									碰到边缘时的处理方式
     * @return														返回值含义:0 不碰到边缘,1 碰到边缘反弹,2 碰到边缘停止
     */
    int IsAtEdge(int deltaX, int deltaY, int mode) const
    {
        AutoUpdate();
        RECT rect;
        GetClientRect(GetDesktopWindow(), &rect);

        if (xPos + deltaX <= 0 ||
            xPos + deltaX + windowWidth > rect.right - rect.left ||
            yPos + deltaY <= 0 ||
            yPos + deltaY + windowHeight > rect.bottom - rect.top
            )
        {
            if (mode == BOUNCE) return 1;
            if (mode == STOP) return 2;
        }
        return 0;
    }

    /**
     * @brief				移动窗口
     *
     * @param				deltaX									窗口将要移动的X轴距离
     * @param				deltaY									窗口将要移动的Y轴距离
     * @param				mode									碰到边缘时的处理方式
     */
    void Move(int deltaX, int deltaY, int mode)
    {
        AutoUpdate();
        RECT rect;
        GetClientRect(GetDesktopWindow(), &rect);

        if (hasCollided)
        {
            deltaX = -deltaX, deltaY = -deltaY;
            int edge = IsAtEdge(deltaX, deltaY, mode);
            if (edge == 1) hasCollided = false;
            if (edge == 2) return;
        }
        else
        {
            int edge = IsAtEdge(deltaX, deltaY, mode);
            if (edge == 1)
            {
                if (mode == BOUNCE) hasCollided = true;
            }
            else if (edge == 2)
            {
                return;
            }
        }

        xPos += deltaX, yPos += deltaY;
        MoveWindow(hWnd, xPos, yPos, windowWidth, windowHeight, TRUE);
    }

    /**
     * @brief				向上移动窗口
     * @param				dt										移动距离
     * @param				mode									碰到边缘时的处理方式
     */
    void MoveUp(int dt, int mode) {
        AutoUpdate();
        Move(0, -dt, mode);
    }

    /**
     * @brief				向下移动窗口
     * @param				dt										移动距离
     * @param				mode									碰到边缘时的处理方式
     */
    void MoveDown(int dt, int mode) {
        AutoUpdate();
        Move(0, dt, mode);
    }

    /**
     * @brief				向左移动窗口
     * @param				dt										移动距离
     * @param				mode									碰到边缘时的处理方式
     */
    void MoveLeft(int dt, int mode) {
        AutoUpdate();
        Move(-dt, 0, mode);
    }

    /**
     * @brief				向右移动窗口
     * @param				dt										移动距离
     * @param				mode									碰到边缘时的处理方式
     */
    void MoveRight(int dt, int mode) {
        AutoUpdate();
        Move(dt, 0, mode);
    }
    /**
 * @brief                  窗口晃动效果
 * @param duration         晃动持续时间(毫秒)
 * @param intensity        晃动强度(像素)
 * @param frequency        晃动频率(次数)
 */
 /**
* @brief                  弹簧衰减晃动效果
* @param shakeCount       晃动次数
* @param maxIntensity     最大晃动强度
*/
    void Shake(int shakeCount = 8, int maxIntensity = 15) {
        AutoUpdate();

        int originalX = xPos;
        int originalY = yPos;

        for (int i = 0; i < shakeCount; i++) {
            // 计算衰减的强度
            float decay = static_cast<float>(shakeCount - i) / shakeCount;
            int currentIntensity = static_cast<int>(maxIntensity * decay);

            if (currentIntensity < 1) currentIntensity = 1;

            // 右→左晃动
            SetWindowPos(hWnd, nullptr, originalX + currentIntensity, originalY,
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
            Sleep(30);

            SetWindowPos(hWnd, nullptr, originalX - currentIntensity, originalY,
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
            Sleep(30);

            // 下→上晃动
            SetWindowPos(hWnd, nullptr, originalX, originalY + currentIntensity,
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
            Sleep(30);

            SetWindowPos(hWnd, nullptr, originalX, originalY - currentIntensity,
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
            Sleep(30);
        }

        // 回到精确位置
        SetWindowPos(hWnd, nullptr, originalX, originalY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    /**
     * @brief
     * @param angle
     * @param zoomX
     * @param zoomY
     * @param offsetX
     * @param offsetY
     * @param center
     */
    void Rotate(
        float angle, float zoomX = 1, float zoomY = 1,
        int offsetX = 0, int offsetY = 0, POINT center = { -1, -1 }
    ) const
    {
        AutoUpdate();
        auto ToLongRound = [](float v) -> LONG {
            // 明确“我们就是要把浮点坐标转成像素坐标”，避免 MSVC C4244 警告刷屏
            return static_cast<LONG>(std::lround(v));
        };

        if (center.x == -1 && center.y == -1) {
            center.x = windowWidth / 2;
            center.y = windowHeight / 2;
        }

        POINT pt{};
        pt.x = center.x;
        pt.y = center.y;

        SIZE sz{};
        sz.cx = windowWidth;
        sz.cy = windowHeight;

        POINT ppt[3]{};
        angle = angle * (PI / 180);
        float sina = sin(angle);
        float cosa = cos(angle);
        const float x0 = static_cast<float>(pt.x) + sina * static_cast<float>(pt.y) - cosa * static_cast<float>(pt.x) * zoomX + static_cast<float>(offsetX);
        const float y0 = static_cast<float>(pt.y) - cosa * static_cast<float>(pt.y) - sina * static_cast<float>(pt.x) * zoomY + static_cast<float>(offsetY);

        ppt[0].x = ToLongRound(x0);
        ppt[0].y = ToLongRound(y0);
        ppt[1].x = ToLongRound(x0 + cosa * static_cast<float>(sz.cx) * zoomX + static_cast<float>(offsetX));
        ppt[1].y = ToLongRound(y0 + sina * static_cast<float>(sz.cx) * zoomY + static_cast<float>(offsetY));
        ppt[2].x = ToLongRound(x0 - sina * static_cast<float>(sz.cy) * zoomX + static_cast<float>(offsetX));
        ppt[2].y = ToLongRound(y0 + cosa * static_cast<float>(sz.cy) * zoomY + static_cast<float>(offsetY));
        PlgBlt(hdcWindow, ppt, hdcWindow, 0, 0, windowWidth, windowHeight, 0, 0, 0);
    }

    void turnLeft(float angle) const {
        AutoUpdate();
        Rotate(-angle, 1, 1, 0, 0, { -1, -1 });
    }

    void turnRight(float angle) const {
        AutoUpdate();
        Rotate(angle, 1, 1, 0, 0, { -1, -1 });
    }

    void DrawImageToBitmap(HBITMAP hBitmap) const {
        AutoUpdate();
        // 创建一个与传入位图兼容的临时内存设备上下文
        evgdi::win::unique_hdc hdcBitmap(CreateCompatibleDC(hdcMem));
        if (!hdcBitmap) return;

        // 将传入的位图选入临时内存设备上下文
        evgdi::win::select_object_guard bmpSel(hdcBitmap.get(), hBitmap);

        // 获取传入位图的宽度和高度
        BITMAP bmpInfo{};
        GetObject(hBitmap, sizeof(BITMAP), &bmpInfo);
        int bmpWidth = bmpInfo.bmWidth;
        int bmpHeight = bmpInfo.bmHeight;

        // 将传入位图绘制到临时位图中
        BitBlt(hdcMem, 0, 0, bmpWidth, bmpHeight, hdcBitmap.get(), 0, 0, SRCCOPY);
    }

    void LoadAndDrawImageFromResource(int resourceID) const {
        AutoUpdate();
        // 加载资源图片
        evgdi::win::unique_hbitmap hBitmap(reinterpret_cast<HBITMAP>(
            LoadImageA(NULL, MAKEINTRESOURCEA(resourceID), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS)
        ));
        if (!hBitmap) return;

        // 将图片绘制到位图上
        DrawImageToBitmap(hBitmap.get());
    }

    void LoadAndDrawImageFromFile(const char* filePath) const {
        AutoUpdate();
        // 加载位图文件
        evgdi::win::unique_hbitmap hBitmap((HBITMAP)LoadImageA(NULL, filePath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION));
        if (!hBitmap) return;

        // 获取窗口客户区域大小
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int clientWidth = clientRect.right - clientRect.left;
        int clientHeight = clientRect.bottom - clientRect.top;

        // 获取位图大小
        BITMAP bmpInfo{};
        GetObject(hBitmap.get(), sizeof(BITMAP), &bmpInfo);
        int bmpWidth = bmpInfo.bmWidth;
        int bmpHeight = bmpInfo.bmHeight;

        // 计算居中位置
        int startX = (clientWidth - bmpWidth) / 2;
        int startY = (clientHeight - bmpHeight) / 2;

        // 创建设备上下文（RAII）
        evgdi::win::window_dc hdc(hWnd);
        if (!hdc) return;
        evgdi::win::unique_hdc hdcMem(CreateCompatibleDC(hdc.get()));
        if (!hdcMem) return;
        evgdi::win::select_object_guard sel(hdcMem.get(), hBitmap.get());

        // 将图片绘制到位图上
        BitBlt(hdc.get(), startX, startY, bmpWidth, bmpHeight, hdcMem.get(), 0, 0, SRCCOPY);
    }

    void AdjustBrightness(float factor) {
        AutoUpdate();
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
        AutoUpdate();
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
        AutoUpdate();
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
        hdcWindow = GetDC(hWnd);  // 获取窗口设备上下文
        if (!hdcWindow) {
            throw_runtime_error("LayeredWindowGDI init failed: GetDC(hWnd) failed.");
        }

        hdcMem = CreateCompatibleDC(hdcWindow);
        if (!hdcMem) {
            ReleaseDC(hWnd, hdcWindow);
            hdcWindow = nullptr;
            throw_runtime_error("LayeredWindowGDI init failed: CreateCompatibleDC failed.");
        }

        // 创建临时位图并关联像素数组
        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biWidth = windowWidth;
        bmi.bmiHeader.biHeight = windowHeight;
        hbmTemp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, (void**)&rgbScreen, NULL, 0);
        if (!hbmTemp) {
            DeleteDC(hdcMem);
            hdcMem = nullptr;
            ReleaseDC(hWnd, hdcWindow);
            hdcWindow = nullptr;
            throw_runtime_error("LayeredWindowGDI init failed: CreateDIBSection failed.");
        }

        oldTempBmp = SelectObject(hdcMem, hbmTemp);
        if (oldTempBmp == nullptr || oldTempBmp == HGDI_ERROR) {
            DeleteObject(hbmTemp);
            hbmTemp = nullptr;
            DeleteDC(hdcMem);
            hdcMem = nullptr;
            ReleaseDC(hWnd, hdcWindow);
            hdcWindow = nullptr;
            throw_runtime_error("LayeredWindowGDI init failed: SelectObject failed.");
        }
    }

    // 自动更新消息处理
    void AutoUpdate() const {
        MSG msg;
        while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
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
        case WM_ACTIVATE:
            // 处理窗口激活状态变化
            if (LOWORD(wParam) == WA_INACTIVE) {
                // 窗口失去焦点时的处理
                // CHULI NIUMO FUCK MIRCOSOFT
            }
            break;
        case WM_TIMER: {
            break;
        }
        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;
        case WM_DESTROY:
            // 说明：作为“库里的窗口组件”，这里不要 PostQuitMessage。
            // 否则当用户关闭这个窗口时，会把整个线程的消息循环直接退出，影响别的窗口/功能。
            break;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        return 0;
    }
};
