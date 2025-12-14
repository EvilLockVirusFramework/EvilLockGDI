// layeredTextout.hpp
#pragma once

#include <windows.h>
#include <cmath>
#include <string>
#include <vector>
#include <random>
#include <memory>
#include <chrono>
#include <algorithm>

#define PI 3.14159265358979323846f

// 3D变换参数结构体
struct Transform3D {
    float rotationX = 0.0f;    // X轴旋转角度(弧度)
    float rotationY = 0.0f;    // Y轴旋转角度(弧度)  
    float rotationZ = 0.0f;    // Z轴旋转角度(弧度)
    float scaleX = 1.0f;       // X轴缩放
    float scaleY = 1.0f;       // Y轴缩放
    float scaleZ = 1.0f;       // Z轴缩放
    float translateX = 0.0f;   // X轴平移
    float translateY = 0.0f;   // Y轴平移
    float translateZ = 0.0f;   // Z轴平移
    float perspective = 0.001f; // 透视强度
};

// 文字样式结构体
struct TextStyle {
    int fontSize = 36;                    // 字体大小
    float widthScale = 1.0f;              // 宽度缩放比例（宽扁效果）
    float heightScale = 1.0f;             // 高度缩放比例
    float charSpacing = 1.0f;             // 字符间距
    bool enableStretch = false;           // 是否启用拉伸
    float stretchIntensity = 1.0f;        // 拉伸强度
    std::wstring fontFamily = L"微软雅黑"; // 字体家族
    int fontWeight = FW_BOLD;             // 字体粗细
};

// 动态渐变参数结构体
struct DynamicGradientParams {
    COLORREF startColor = RGB(255, 0, 0);     // 起始颜色
    COLORREF endColor = RGB(0, 0, 255);       // 结束颜色
    COLORREF solidColor = RGB(255, 255, 255); // 纯色模式颜色
    float gradientSpeed = 1.0f;               // 渐变速度
    bool useRainbow = true;                   // 是否使用彩虹色
    bool useSolidColor = false;               // 是否使用纯色
    float time = 0.0f;                        // 时间累积
};

// 滤镜效果参数
struct FilterEffects {
    // 扭曲效果
    bool enableFishEye = false;               // 鱼眼效果
    float fishEyeStrength = 0.5f;             // 鱼眼强度
    float fishEyeRadius = 200.0f;             // 鱼眼半径
    float fishEyeProgress = 0.0f;             // 鱼眼进度（0-1）

    bool enableTwirl = false;                 // 漩涡扭曲
    float twirlStrength = 1.0f;               // 漩涡强度
    float twirlRadius = 300.0f;               // 漩涡半径
    float twirlProgress = 0.0f;               // 漩涡进度

    bool enableWave = false;                  // 波浪扭曲
    float waveAmplitudeX = 10.0f;             // X轴波浪幅度
    float waveAmplitudeY = 5.0f;              // Y轴波浪幅度
    float waveFrequencyX = 0.05f;             // X轴波浪频率
    float waveFrequencyY = 0.03f;             // Y轴波浪频率

    // 像素效果
    bool enablePixelate = false;              // 像素化
    int pixelSize = 8;                        // 像素大小

    // 颜色效果
    bool enableInvert = false;                // 颜色反转
    bool enableGrayscale = false;             // 灰度化
    float contrast = 1.0f;                    // 对比度
    float brightness = 0.0f;                  // 亮度
};

// 动画时钟系统
struct AnimationClock {
    float globalTime = 0.0f;                  // 全局时间
    float speed = 1.0f;                       // 时间速度
    bool paused = false;                      // 是否暂停

    // 动画参数
    float waveTime = 0.0f;                    // 波浪动画时间
    float pulseTime = 0.0f;                   // 脉冲动画时间
    float rotationTime = 0.0f;                // 旋转动画时间
    float stretchTime = 0.0f;                 // 拉伸动画时间

    void Update(float deltaTime) {
        if (!paused) {
            float scaledDelta = deltaTime * speed;
            globalTime += scaledDelta;
            waveTime += scaledDelta;
            pulseTime += scaledDelta;
            rotationTime += scaledDelta;
            stretchTime += scaledDelta;
        }
    }

    void Reset() {
        globalTime = 0.0f;
        waveTime = 0.0f;
        pulseTime = 0.0f;
        rotationTime = 0.0f;
        stretchTime = 0.0f;
    }
};

class LayeredTextOut {
private:
    HWND hwnd_ = nullptr;
    int width_ = 800;
    int height_ = 600;
    COLORREF bgColor_ = RGB(0, 0, 0);
    bool isVisible_ = false;
    int windowX_ = 0;
    int windowY_ = 0;
    BYTE alpha_ = 255;  // 透明度 (0-255)

    // 可调节参数
    Transform3D transform3D_;
    TextStyle textStyle_;
    DynamicGradientParams colorParams_;
    FilterEffects filterEffects_;
    AnimationClock animationClock_;
    std::wstring text_ = L"高性能文字效果";

    // 动画相关
    std::chrono::steady_clock::time_point lastUpdateTime_;
    bool useAnimation_ = true;

    // GDI资源
    HDC memDC_ = nullptr;
    HBITMAP memBmp_ = nullptr;
    HBITMAP oldBmp_ = nullptr;

public:
    LayeredTextOut() {
        lastUpdateTime_ = std::chrono::steady_clock::now();
    }

    ~LayeredTextOut() {
        Destroy();
        CleanupGDI();
    }

    HWND GetHWND() const { return hwnd_; }

    // === 文字样式控制 ===
    void SetFontSize(int size) {
        textStyle_.fontSize = size;
        UpdateWindowContent();
    }

    void SetWidthScale(float scale) {
        textStyle_.widthScale = scale;
        UpdateWindowContent();
    }

    void SetHeightScale(float scale) {
        textStyle_.heightScale = scale;
        UpdateWindowContent();
    }

    void SetCharSpacing(float spacing) {
        textStyle_.charSpacing = spacing;
        UpdateWindowContent();
    }

    void EnableStretch(bool enable, float intensity = 1.0f) {
        textStyle_.enableStretch = enable;
        textStyle_.stretchIntensity = intensity;
        UpdateWindowContent();
    }

    void SetStretchIntensity(float intensity) {
        textStyle_.stretchIntensity = intensity;
        if (textStyle_.enableStretch) {
            UpdateWindowContent();
        }
    }

    void SetFontFamily(const std::wstring& fontFamily) {
        textStyle_.fontFamily = fontFamily;
        UpdateWindowContent();
    }

    void SetFontWeight(int weight) {
        textStyle_.fontWeight = weight;
        UpdateWindowContent();
    }

    // 快速设置宽扁效果
    void SetWideFlatEffect(bool enable, float widthScale = 1.5f, float heightScale = 0.8f) {
        if (enable) {
            textStyle_.widthScale = widthScale;
            textStyle_.heightScale = heightScale;
            textStyle_.enableStretch = true;
        }
        else {
            textStyle_.widthScale = 1.0f;
            textStyle_.heightScale = 1.0f;
            textStyle_.enableStretch = false;
        }
        UpdateWindowContent();
    }

    // 动态宽扁动画
    void EnableDynamicStretch(bool enable, float speed = 2.0f) {
        if (enable) {
            textStyle_.enableStretch = true;
            animationClock_.speed = speed;
        }
        UpdateWindowContent();
    }

    void IncreaseWidthScale(float increment = 0.1f) {
        textStyle_.widthScale += increment;
        if (textStyle_.widthScale > 3.0f) textStyle_.widthScale = 3.0f;
        UpdateWindowContent();
    }

    void DecreaseWidthScale(float decrement = 0.1f) {
        textStyle_.widthScale -= decrement;
        if (textStyle_.widthScale < 0.1f) textStyle_.widthScale = 0.1f;
        UpdateWindowContent();
    }

    void IncreaseHeightScale(float increment = 0.1f) {
        textStyle_.heightScale += increment;
        if (textStyle_.heightScale > 3.0f) textStyle_.heightScale = 3.0f;
        UpdateWindowContent();
    }

    void DecreaseHeightScale(float decrement = 0.1f) {
        textStyle_.heightScale -= decrement;
        if (textStyle_.heightScale < 0.1f) textStyle_.heightScale = 0.1f;
        UpdateWindowContent();
    }

    // === 3D变换控制方法 ===
    void RotateX(float angle) { transform3D_.rotationX += angle; UpdateWindowContent(); }
    void RotateY(float angle) { transform3D_.rotationY += angle; UpdateWindowContent(); }
    void RotateZ(float angle) { transform3D_.rotationZ += angle; UpdateWindowContent(); }
    void SetRotationX(float angle) { transform3D_.rotationX = angle; UpdateWindowContent(); }
    void SetRotationY(float angle) { transform3D_.rotationY = angle; UpdateWindowContent(); }
    void SetRotationZ(float angle) { transform3D_.rotationZ = angle; UpdateWindowContent(); }

    void ScaleX(float factor) { transform3D_.scaleX *= factor; UpdateWindowContent(); }
    void ScaleY(float factor) { transform3D_.scaleY *= factor; UpdateWindowContent(); }
    void ScaleZ(float factor) { transform3D_.scaleZ *= factor; UpdateWindowContent(); }
    void ScaleUniform(float factor) {
        transform3D_.scaleX *= factor;
        transform3D_.scaleY *= factor;
        transform3D_.scaleZ *= factor;
        UpdateWindowContent();
    }
    void SetScaleX(float scale) { transform3D_.scaleX = scale; UpdateWindowContent(); }
    void SetScaleY(float scale) { transform3D_.scaleY = scale; UpdateWindowContent(); }
    void SetScaleZ(float scale) { transform3D_.scaleZ = scale; UpdateWindowContent(); }

    void TranslateX(float distance) { transform3D_.translateX += distance; UpdateWindowContent(); }
    void TranslateY(float distance) { transform3D_.translateY += distance; UpdateWindowContent(); }
    void TranslateZ(float distance) { transform3D_.translateZ += distance; UpdateWindowContent(); }
    void SetTranslateX(float x) { transform3D_.translateX = x; UpdateWindowContent(); }
    void SetTranslateY(float y) { transform3D_.translateY = y; UpdateWindowContent(); }
    void SetTranslateZ(float z) { transform3D_.translateZ = z; UpdateWindowContent(); }

    void SetPerspective(float strength) { transform3D_.perspective = strength; UpdateWindowContent(); }
    void ResetTransform() {
        transform3D_ = Transform3D();
        UpdateWindowContent();
    }

    // === 字体和文字控制 ===
    void IncreaseFontSize(int increment = 2) {
        textStyle_.fontSize += increment;
        if (textStyle_.fontSize > 200) textStyle_.fontSize = 200;
        UpdateWindowContent();
    }
    void DecreaseFontSize(int decrement = 2) {
        textStyle_.fontSize -= decrement;
        if (textStyle_.fontSize < 8) textStyle_.fontSize = 8;
        UpdateWindowContent();
    }

    void SetText(const std::wstring& text) {
        text_ = text;
        UpdateWindowContent();
    }

    void SetText(const wchar_t* text) {
        text_ = text;
        UpdateWindowContent();
    }

    // === 动态渐变控制 ===
    void SetDynamicGradientSpeed(float speed) { colorParams_.gradientSpeed = speed; }
    void SetRainbowMode(bool enable) {
        colorParams_.useRainbow = enable;
        colorParams_.useSolidColor = false;
        UpdateWindowContent();
    }
    void SetSolidColorMode(bool enable, COLORREF color = RGB(255, 255, 255)) {
        colorParams_.useSolidColor = enable;
        colorParams_.solidColor = color;
        colorParams_.useRainbow = false;
        UpdateWindowContent();
    }
    void SetSolidColor(COLORREF color) {
        colorParams_.solidColor = color;
        if (colorParams_.useSolidColor) {
            UpdateWindowContent();
        }
    }
    void SetGradientColors(COLORREF start, COLORREF end) {
        colorParams_.startColor = start;
        colorParams_.endColor = end;
        colorParams_.useRainbow = false;
        colorParams_.useSolidColor = false;
        UpdateWindowContent();
    }

    // === 透明度控制 ===
    void SetAlpha(BYTE alpha) {
        alpha_ = alpha;
        UpdateWindowContent();
    }
    void IncreaseAlpha(int increment = 10) {
        alpha_ = (std::min)(255, alpha_ + increment);
        UpdateWindowContent();
    }
    void DecreaseAlpha(int decrement = 10) {
        alpha_ = (std::max)(0, alpha_ - decrement);
        UpdateWindowContent();
    }

    // === 窗口位置控制 ===
    void SetWindowPosition(int x, int y) {
        windowX_ = x;
        windowY_ = y;
        if (hwnd_) {
            SetWindowPos(hwnd_, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }
    void MoveWindow(int deltaX, int deltaY) {
        windowX_ += deltaX;
        windowY_ += deltaY;
        if (hwnd_) {
            SetWindowPos(hwnd_, NULL, windowX_, windowY_, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }
    void CenterWindow() {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        SetWindowPosition((screenWidth - width_) / 2, (screenHeight - height_) / 2);
    }

    // === 滤镜效果控制 ===
    // 扭曲效果 - 使用PlgBlt实现高性能
    void EnableFishEye(bool enable, float strength = 0.5f, bool instant = false) {
        filterEffects_.enableFishEye = enable;
        filterEffects_.fishEyeStrength = strength;
        if (instant) {
            filterEffects_.fishEyeProgress = enable ? 1.0f : 0.0f;
        }
        UpdateWindowContent();
    }

    void SetFishEyeProgress(float progress) {
        filterEffects_.fishEyeProgress = (std::max)(0.0f, (std::min)(1.0f, progress));
        UpdateWindowContent();
    }

    void EnableTwirl(bool enable, float strength = 1.0f, bool instant = false) {
        filterEffects_.enableTwirl = enable;
        filterEffects_.twirlStrength = strength;
        if (instant) {
            filterEffects_.twirlProgress = enable ? 1.0f : 0.0f;
        }
        UpdateWindowContent();
    }

    void SetTwirlProgress(float progress) {
        filterEffects_.twirlProgress = (std::max)(0.0f, (std::min)(1.0f, progress));
        UpdateWindowContent();
    }

    void EnableWave(bool enable, float ampX = 10.0f, float ampY = 5.0f, float freqX = 0.05f, float freqY = 0.03f) {
        filterEffects_.enableWave = enable;
        filterEffects_.waveAmplitudeX = ampX;
        filterEffects_.waveAmplitudeY = ampY;
        filterEffects_.waveFrequencyX = freqX;
        filterEffects_.waveFrequencyY = freqY;
        UpdateWindowContent();
    }

    // 像素效果
    void EnablePixelate(bool enable, int size = 8) {
        filterEffects_.enablePixelate = enable;
        filterEffects_.pixelSize = size;
        UpdateWindowContent();
    }

    // 颜色效果
    void EnableInvert(bool enable) {
        filterEffects_.enableInvert = enable;
        UpdateWindowContent();
    }

    void EnableGrayscale(bool enable) {
        filterEffects_.enableGrayscale = enable;
        UpdateWindowContent();
    }

    void SetContrast(float contrast) {
        filterEffects_.contrast = contrast;
        UpdateWindowContent();
    }

    void SetBrightness(float brightness) {
        filterEffects_.brightness = brightness;
        UpdateWindowContent();
    }

    // 重置所有滤镜
    void ResetFilters() {
        filterEffects_ = FilterEffects();
        UpdateWindowContent();
    }

    // === 动画时钟控制 ===
    void SetAnimationSpeed(float speed) { animationClock_.speed = speed; }
    void PauseAnimation(bool pause) { animationClock_.paused = pause; }
    void ResetAnimationClock() { animationClock_.Reset(); }
    float GetGlobalTime() const { return animationClock_.globalTime; }

    void EnableAnimation(bool enable) {
        useAnimation_ = enable;
        if (enable) {
            lastUpdateTime_ = std::chrono::steady_clock::now();
        }
    }

    void ResetAnimationTime() {
        colorParams_.time = 0.0f;
        animationClock_.Reset();
        lastUpdateTime_ = std::chrono::steady_clock::now();
    }

    // === 获取当前参数 ===
    Transform3D GetTransform3D() const { return transform3D_; }
    TextStyle GetTextStyle() const { return textStyle_; }
    DynamicGradientParams GetColorParams() const { return colorParams_; }
    FilterEffects GetFilterEffects() const { return filterEffects_; }
    AnimationClock GetAnimationClock() const { return animationClock_; }
    BYTE GetAlpha() const { return alpha_; }
    std::wstring GetText() const { return text_; }

    // 原有基础方法
    bool Create(int width = 800, int height = 600) {
        width_ = width;
        height_ = height;

        WNDCLASSW wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"LayeredTextOutWindow";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

        if (!RegisterClassW(&wc)) return false;

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        windowX_ = (screenWidth - width_) / 2;
        windowY_ = (screenHeight - height_) / 2;

        hwnd_ = CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            L"LayeredTextOutWindow", L"High Performance Text Effects", WS_POPUP,
            windowX_, windowY_, width_, height_, NULL, NULL, GetModuleHandle(NULL), this
        );

        // 初始化GDI资源
        if (hwnd_) {
            HDC screenDC = GetDC(NULL);
            memDC_ = CreateCompatibleDC(screenDC);
            memBmp_ = CreateCompatibleBitmap(screenDC, width_, height_);
            oldBmp_ = (HBITMAP)SelectObject(memDC_, memBmp_);
            ReleaseDC(NULL, screenDC);
            return true;
        }

        return false;
    }

    void Show() {
        if (hwnd_ && !isVisible_) {
            ShowWindow(hwnd_, SW_SHOW);
            isVisible_ = true;
            UpdateWindowContent();
        }
    }

    void Hide() {
        if (hwnd_ && isVisible_) {
            ShowWindow(hwnd_, SW_HIDE);
            isVisible_ = false;
        }
    }

    void Destroy() {
        if (hwnd_) {
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
        }
    }

    void SetBackgroundColor(COLORREF color) {
        bgColor_ = color;
        UpdateWindowContent();
    }

    void SetSize(int width, int height) {
        if (hwnd_) {
            width_ = width;
            height_ = height;
            SetWindowPos(hwnd_, NULL, 0, 0, width_, height_, SWP_NOMOVE | SWP_NOZORDER);

            // 重新创建GDI资源
            CleanupGDI();
            HDC screenDC = GetDC(NULL);
            memDC_ = CreateCompatibleDC(screenDC);
            memBmp_ = CreateCompatibleBitmap(screenDC, width_, height_);
            oldBmp_ = (HBITMAP)SelectObject(memDC_, memBmp_);
            ReleaseDC(NULL, screenDC);

            UpdateWindowContent();
        }
    }

    void UpdateWindowContent() {
        if (!hwnd_ || !isVisible_ || !memDC_) return;

        // 更新时间系统
        if (useAnimation_) {
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                currentTime - lastUpdateTime_);
            float deltaTime = elapsed.count() * 0.001f;

            // 更新所有时间轴
            colorParams_.time += deltaTime * colorParams_.gradientSpeed;
            animationClock_.Update(deltaTime);

            // 动态拉伸效果
            if (textStyle_.enableStretch) {
                // 根据时间动态调整宽扁效果，增强3D感
                float stretchWave = sin(animationClock_.stretchTime * 2.0f) * 0.2f + 1.0f;
                textStyle_.widthScale = 1.2f * stretchWave * textStyle_.stretchIntensity;
                textStyle_.heightScale = 0.9f / stretchWave * textStyle_.stretchIntensity;
            }

            // 渐进式滤镜效果
            if (filterEffects_.enableFishEye) {
                filterEffects_.fishEyeProgress = (std::min)(1.0f, filterEffects_.fishEyeProgress + deltaTime * 2.0f);
            }
            else {
                filterEffects_.fishEyeProgress = (std::max)(0.0f, filterEffects_.fishEyeProgress - deltaTime * 2.0f);
            }

            if (filterEffects_.enableTwirl) {
                filterEffects_.twirlProgress = (std::min)(1.0f, filterEffects_.twirlProgress + deltaTime * 1.5f);
            }
            else {
                filterEffects_.twirlProgress = (std::max)(0.0f, filterEffects_.twirlProgress - deltaTime * 1.5f);
            }

            lastUpdateTime_ = currentTime;
        }

        // 绘制基础内容
        RECT rect = { 0, 0, width_, height_ };
        HBRUSH bgBrush = CreateSolidBrush(bgColor_);
        FillRect(memDC_, &rect, bgBrush);
        Draw3DText(memDC_);

        // 应用高性能滤镜效果
        ApplyHighPerformanceEffects();

        // 更新到分层窗口
        UpdateLayeredWindowContent();

        DeleteObject(bgBrush);
    }

private:
    void CleanupGDI() {
        if (memDC_ && oldBmp_) {
            SelectObject(memDC_, oldBmp_);
            oldBmp_ = nullptr;
        }
        if (memBmp_) {
            DeleteObject(memBmp_);
            memBmp_ = nullptr;
        }
        if (memDC_) {
            DeleteDC(memDC_);
            memDC_ = nullptr;
        }
    }

    void UpdateLayeredWindowContent() {
        if (!hwnd_ || !memDC_) return;

        HDC screenDC = GetDC(NULL);
        POINT ptSrc = { 0, 0 };
        SIZE size = { width_, height_ };
        BLENDFUNCTION blend = { AC_SRC_OVER, 0, alpha_, AC_SRC_ALPHA };
        POINT dstPoint = { windowX_, windowY_ };

        UpdateLayeredWindow(hwnd_, screenDC, &dstPoint, &size, memDC_, &ptSrc,
            RGB(0, 0, 0), &blend, ULW_COLORKEY);

        ReleaseDC(NULL, screenDC);
    }

    // 3D变换计算
    void TransformPoint(float& x, float& y, float& z) const {
        // 应用缩放
        x *= transform3D_.scaleX;
        y *= transform3D_.scaleY;
        z *= transform3D_.scaleZ;

        // 应用旋转
        float cosX = cos(transform3D_.rotationX);
        float sinX = sin(transform3D_.rotationX);
        float cosY = cos(transform3D_.rotationY);
        float sinY = sin(transform3D_.rotationY);
        float cosZ = cos(transform3D_.rotationZ);
        float sinZ = sin(transform3D_.rotationZ);

        // Z轴旋转
        float x1 = x * cosZ - y * sinZ;
        float y1 = x * sinZ + y * cosZ;
        float z1 = z;

        // Y轴旋转
        float x2 = x1 * cosY + z1 * sinY;
        float y2 = y1;
        float z2 = -x1 * sinY + z1 * cosY;

        // X轴旋转
        float x3 = x2;
        float y3 = y2 * cosX - z2 * sinX;
        float z3 = y2 * sinX + z2 * cosX;

        x = x3 + transform3D_.translateX;
        y = y3 + transform3D_.translateY;
        z = z3 + transform3D_.translateZ;
    }

    // 透视投影
    void ProjectPoint(float x, float y, float z, int& screenX, int& screenY) const {
        float factor = 1.0f / (1.0f + z * transform3D_.perspective);
        screenX = static_cast<int>(x * factor + width_ / 2);
        screenY = static_cast<int>(y * factor + height_ / 2);
    }

    // 动态渐变色计算
    COLORREF CalculateDynamicGradient(float charIndex, float totalChars) const {
        if (colorParams_.useSolidColor) {
            return colorParams_.solidColor;
        }
        else if (colorParams_.useRainbow) {
            // 彩虹色动态渐变 - 基于时间和字符位置
            float hue = colorParams_.time + charIndex / totalChars * 0.3f;
            hue = fmod(hue, 1.0f) * 6.28318f;

            int r = static_cast<int>((sin(hue + 0) + 1) * 127.5f);
            int g = static_cast<int>((sin(hue + 2.094f) + 1) * 127.5f);
            int b = static_cast<int>((sin(hue + 4.188f) + 1) * 127.5f);
            return RGB(r, g, b);
        }
        else {
            // 自定义颜色动态渐变
            float t = fmod(colorParams_.time + charIndex / totalChars * 0.2f, 1.0f);
            float wave = (sin(t * 6.28318f) + 1) * 0.5f;

            int r = LerpColor(GetRValue(colorParams_.startColor), GetRValue(colorParams_.endColor), wave);
            int g = LerpColor(GetGValue(colorParams_.startColor), GetGValue(colorParams_.endColor), wave);
            int b = LerpColor(GetBValue(colorParams_.startColor), GetBValue(colorParams_.endColor), wave);

            return RGB(r, g, b);
        }
    }

    // 颜色插值辅助函数
    int LerpColor(int start, int end, float t) const {
        return start + static_cast<int>(t * (end - start));
    }

    // 创建带拉伸效果的字体
    HFONT CreateStretchedFont() const {
        // 计算拉伸后的字体尺寸
        int stretchedWidth = static_cast<int>(textStyle_.fontSize * textStyle_.widthScale);
        int stretchedHeight = static_cast<int>(textStyle_.fontSize * textStyle_.heightScale);

        return CreateFontW(
            stretchedHeight,                    // 字体高度
            stretchedWidth,                     // 字体宽度（控制宽扁效果）
            0,                                  // 旋转角度
            0,                                  // 方向
            textStyle_.fontWeight,              // 字体粗细
            FALSE,                              // 斜体
            FALSE,                              // 下划线
            FALSE,                              // 删除线
            DEFAULT_CHARSET,                    // 字符集
            OUT_DEFAULT_PRECIS,                 // 输出精度
            CLIP_DEFAULT_PRECIS,                // 裁剪精度
            DEFAULT_QUALITY,                    // 输出质量
            DEFAULT_PITCH | FF_DONTCARE,        // 字体系列
            textStyle_.fontFamily.c_str()       // 字体名称
        );
    }

    void Draw3DText(HDC hdc) {
        SetBkMode(hdc, TRANSPARENT);

        // 创建带拉伸效果的字体
        HFONT hFont = CreateStretchedFont();
        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

        const wchar_t* text = text_.c_str();
        size_t textLen = text_.length();

        for (size_t i = 0; i < textLen; i++) {
            // 3D位置计算 - 考虑字符间距
            float x = (i - textLen / 2.0f) * (textStyle_.fontSize * 0.8f * textStyle_.charSpacing);
            float y = 0;
            float z = 0;
            TransformPoint(x, y, z);

            int screenX, screenY;
            ProjectPoint(x, y, z, screenX, screenY);

            // 使用动态渐变色
            COLORREF color = CalculateDynamicGradient(static_cast<float>(i), static_cast<float>(textLen));
            SetTextColor(hdc, color);

            wchar_t ch[2] = { text[i], L'\0' };

            // 根据3D深度调整绘制参数，增强透视效果
            float depthFactor = 1.0f / (1.0f + z * transform3D_.perspective * 2.0f);
            if (depthFactor < 0.3f) depthFactor = 0.3f; // 限制最小可见度

            // 应用深度感知的字符间距
            float charSpacing = textStyle_.charSpacing * depthFactor;
            screenX = static_cast<int>((i - textLen / 2.0f) * (textStyle_.fontSize * 0.8f * charSpacing) + width_ / 2);

            TextOutW(hdc, screenX, screenY, ch, 1);
        }

        SelectObject(hdc, oldFont);
        DeleteObject(hFont);
    }

    // === 高性能滤镜效果系统 ===
    void ApplyHighPerformanceEffects() {
        if (!AnyFilterEnabled()) return;

        // 使用PlgBlt实现高性能变换
        if (filterEffects_.fishEyeProgress > 0.01f) {
            ApplyFishEyeEffect();
        }
        if (filterEffects_.twirlProgress > 0.01f) {
            ApplyTwirlEffect();
        }
        if (filterEffects_.enableWave) {
            ApplyWaveEffect();
        }
        if (filterEffects_.enablePixelate) {
            ApplyPixelateEffect();
        }
        if (filterEffects_.enableInvert || filterEffects_.enableGrayscale ||
            filterEffects_.contrast != 1.0f || filterEffects_.brightness != 0.0f) {
            ApplyColorEffects();
        }
    }

    bool AnyFilterEnabled() const {
        return filterEffects_.fishEyeProgress > 0.01f || filterEffects_.twirlProgress > 0.01f ||
            filterEffects_.enableWave || filterEffects_.enablePixelate ||
            filterEffects_.enableInvert || filterEffects_.enableGrayscale ||
            filterEffects_.contrast != 1.0f || filterEffects_.brightness != 0.0f;
    }

    // 鱼眼效果 - 使用PlgBlt
    void ApplyFishEyeEffect() {
        float progress = filterEffects_.fishEyeProgress;
        float strength = filterEffects_.fishEyeStrength * progress;

        POINT center = { width_ / 2, height_ / 2 };
        POINT ppt[3];

        // 创建鱼眼变形
        float distortion = 1.0f - strength * 0.3f;
        ppt[0].x = center.x - center.x * distortion;
        ppt[0].y = center.y - center.y * distortion;
        ppt[1].x = center.x + (width_ - center.x) * distortion;
        ppt[1].y = center.y - center.y * distortion;
        ppt[2].x = center.x - center.x * distortion;
        ppt[2].y = center.y + (height_ - center.y) * distortion;

        ApplyPlgBltTransform(ppt);
    }

    // 漩涡效果 - 使用PlgBlt
    void ApplyTwirlEffect() {
        float progress = filterEffects_.twirlProgress;
        float strength = filterEffects_.twirlStrength * progress * 0.5f;

        POINT center = { width_ / 2, height_ / 2 };
        POINT ppt[3];

        // 创建漩涡变形
        float angle = strength;
        float cosa = cos(angle);
        float sina = sin(angle);

        ppt[0].x = center.x - center.x * cosa + center.y * sina;
        ppt[0].y = center.y - center.x * sina - center.y * cosa;
        ppt[1].x = center.x + (width_ - center.x) * cosa + center.y * sina;
        ppt[1].y = center.y + (width_ - center.x) * sina - center.y * cosa;
        ppt[2].x = center.x - center.x * cosa + (height_ - center.y) * sina;
        ppt[2].y = center.y - center.x * sina + (height_ - center.y) * cosa;

        ApplyPlgBltTransform(ppt);
    }

    // 波浪效果 - 使用PlgBlt
    void ApplyWaveEffect() {
        float time = animationClock_.waveTime;
        float waveX = sin(time) * filterEffects_.waveAmplitudeX;
        float waveY = cos(time * 0.7f) * filterEffects_.waveAmplitudeY;

        POINT ppt[3];
        ppt[0].x = waveX;
        ppt[0].y = waveY;
        ppt[1].x = width_ + waveX;
        ppt[1].y = waveY;
        ppt[2].x = waveX;
        ppt[2].y = height_ + waveY;

        ApplyPlgBltTransform(ppt);
    }

    // 应用PlgBlt变换
    void ApplyPlgBltTransform(POINT ppt[3]) {
        HDC screenDC = GetDC(NULL);
        HDC tempDC = CreateCompatibleDC(screenDC);
        HBITMAP tempBmp = CreateCompatibleBitmap(screenDC, width_, height_);
        HBITMAP oldTempBmp = (HBITMAP)SelectObject(tempDC, tempBmp);

        // 复制内容到临时DC
        BitBlt(tempDC, 0, 0, width_, height_, memDC_, 0, 0, SRCCOPY);

        // 清除原始内容
        RECT rect = { 0, 0, width_, height_ };
        HBRUSH bgBrush = CreateSolidBrush(bgColor_);
        FillRect(memDC_, &rect, bgBrush);

        // 应用PlgBlt变换
        PlgBlt(memDC_, ppt, tempDC, 0, 0, width_, height_, 0, 0, 0);

        // 清理临时资源
        SelectObject(tempDC, oldTempBmp);
        DeleteObject(tempBmp);
        DeleteDC(tempDC);
        DeleteObject(bgBrush);
        ReleaseDC(NULL, screenDC);
    }

    // 像素化效果
    void ApplyPixelateEffect() {
        int size = filterEffects_.pixelSize;
        if (size <= 1) return;

        for (int y = 0; y < height_; y += size) {
            for (int x = 0; x < width_; x += size) {
                COLORREF color = GetPixel(memDC_, x + size / 2, y + size / 2);

                for (int py = 0; py < size && y + py < height_; py++) {
                    for (int px = 0; px < size && x + px < width_; px++) {
                        SetPixel(memDC_, x + px, y + py, color);
                    }
                }
            }
        }
    }

    // 颜色效果
    void ApplyColorEffects() {
        for (int y = 0; y < height_; y++) {
            for (int x = 0; x < width_; x++) {
                COLORREF color = GetPixel(memDC_, x, y);
                int r = GetRValue(color);
                int g = GetGValue(color);
                int b = GetBValue(color);

                // 颜色反转
                if (filterEffects_.enableInvert) {
                    r = 255 - r;
                    g = 255 - g;
                    b = 255 - b;
                }

                // 灰度化
                if (filterEffects_.enableGrayscale) {
                    int gray = (r + g + b) / 3;
                    r = g = b = gray;
                }

                // 对比度
                r = static_cast<int>((r - 127) * filterEffects_.contrast + 127 + filterEffects_.brightness);
                g = static_cast<int>((g - 127) * filterEffects_.contrast + 127 + filterEffects_.brightness);
                b = static_cast<int>((b - 127) * filterEffects_.contrast + 127 + filterEffects_.brightness);

                // 限制范围
                r = (std::max)(0, (std::min)(255, r));
                g = (std::max)(0, (std::min)(255, g));
                b = (std::max)(0, (std::min)(255, b));

                SetPixel(memDC_, x, y, RGB(r, g, b));
            }
        }
    }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        LayeredTextOut* pThis = nullptr;

        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
            pThis = reinterpret_cast<LayeredTextOut*>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        }
        else {
            pThis = reinterpret_cast<LayeredTextOut*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (pThis) {
            switch (uMsg) {
            case WM_DESTROY:
                pThis->hwnd_ = nullptr;
                return 0;
            }
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
};