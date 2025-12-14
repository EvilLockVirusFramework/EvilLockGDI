/*****************************************************************//**
 * @file				Window.hpp
 * @brief				有边框窗口GDI操作
 *
 * @author				EvilLockVirusFramework
 * @date				2024-01-06
 *********************************************************************/



#ifndef BORDER_WINDOW_GDI_HPP
#define BORDER_WINDOW_GDI_HPP



#pragma once
#include "common.hpp"
#include "color.hpp"



/**
 * @brief														: 有边框窗口GDI操作
 * @details														: 封装对有边框窗口的GDI操作
 */
class BorderedWindowGDI {
public:
	HWND hWnd;													///< 窗口句柄
	HINSTANCE hInstance;										///< 程序实例句柄
	int xPos;													///< X 坐标
	int yPos;													///< Y 坐标
	int windowWidth;											///< 窗口宽
	int windowHeight;											///< 窗口高
	HDC hdcWindow;												///< 窗口上下文
	HDC hdcMem;													///< 内存设备上下文
	HBITMAP hbmTemp;											///< 临时位图
	PRGBQUAD rgbScreen;											///< 像素数组



	/**
	 * @brief													: 初始化窗口类
	 *
	 * @param[in]		hInstance								: 程序实例句柄
	 * @param[in]		x										: X 坐标
	 * @param[in]		y										: Y 坐标
	 * @param[in]		width									: 窗口宽度
	 * @param[in]		height									: 窗口高度
	 */
	BorderedWindowGDI(
		HINSTANCE hInstance,
		int x, int y, int width, int height
	)
		: hWnd(nullptr), hInstance(hInstance),
		xPos(x), yPos(y),
		windowWidth(width), windowHeight(height),
		hdcWindow(nullptr), hdcMem(nullptr),
		hbmTemp(nullptr), rgbScreen(nullptr) {}



	/**
	 * @brief													: 释放 GDI 资源
	 * @details													: noexcept
	 */
	~BorderedWindowGDI() noexcept
	{
		if (hdcWindow) ReleaseDC(hWnd, hdcWindow);				///< 释放窗口设备上下文
		if (hdcMem) DeleteDC(hdcMem);							///< 删除内存设备上下文
		if (hbmTemp) DeleteObject(hbmTemp);						///< 删除临时位图
	}



	/**
	 * @brief													: 创建带边框的窗口
	 *
	 * @param[in]		className								: 窗口类名,默认为 "hopejieshuo"
	 * @param[in]		windowTitle								: 窗口标题,默认为 "EvilLock"
	 * @param[in]		style									: 窗口样式,默认为标准重叠窗口
	 */
	void Create(
		const std::string className = "hopejieshuo",
		const std::string windowTitle = "EvilLock",
		DWORD style = WS_OVERLAPPEDWINDOW
	)
	{
		WNDCLASSA wc = { 0 };
		wc.lpfnWndProc = WndProc;
		wc.hInstance = hInstance;
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszClassName = className.c_str();
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

		RegisterClassA(&wc);

		RECT rect = { 0, 0, windowWidth, windowHeight };		///< 调整窗口大小以适应边框
		AdjustWindowRect(&rect, style, FALSE);
		int adjustedWidth = rect.right - rect.left;
		int adjustedHeight = rect.bottom - rect.top;

		hWnd = CreateWindowExA(
			0,													///< 无扩展样式
			className.c_str(),
			windowTitle.c_str(),
			style,
			xPos, yPos, adjustedWidth, adjustedHeight,
			nullptr, nullptr, hInstance, this
		);

		if (hWnd == nullptr) return;

		initialization();										///< 初始化 GDI 资源
		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);
	}



	/**
	 * @brief													: 判断窗口是否碰到屏幕边缘
	 * @return													: 返回值含义:0 不碰到边缘,1 碰到边缘反弹,2 碰到边缘停止
	 *
	 * @param[in]		deltaX									: 窗口将要移动的X轴距离
	 * @param[in]		deltaY									: 窗口将要移动的Y轴距离
	 * @param[in]		mode									: 碰到边缘时的处理方式
	 */
	int IsAtEdge(int deltaX, int deltaY, int mode) const
	{
		AutoUpdate();
		RECT rect;
		GetClientRect(GetDesktopWindow(), &rect);

		RECT windowRect;										///<　获取窗口实际大小
		GetWindowRect(hWnd, &windowRect);
		int actualWidth = windowRect.right - windowRect.left;
		int actualHeight = windowRect.bottom - windowRect.top;

		if (xPos + deltaX <= 0 ||
			xPos + deltaX + actualWidth > rect.right - rect.left ||
			yPos + deltaY <= 0 ||
			yPos + deltaY + actualHeight > rect.bottom - rect.top
			)
		{
			if (mode == BOUNCE) return 1;
			if (mode == STOP) return 2;
		}
		return 0;
	}



	/**
	 * @brief													: 移动窗口
	 *
	 * @param[in]		deltaX									: 窗口将要移动的X轴距离
	 * @param[in]		deltaY									: 窗口将要移动的Y轴距离
	 * @param[in]		mode									: 碰到边缘时的处理方式
	 */
	void Move(int deltaX, int deltaY, int mode)
	{
		AutoUpdate();
		RECT rect;
		GetClientRect(GetDesktopWindow(), &rect);

		RECT windowRect;										///<　获取窗口实际大小
		GetWindowRect(hWnd, &windowRect);
		int actualWidth = windowRect.right - windowRect.left;
		int actualHeight = windowRect.bottom - windowRect.top;

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
		SetWindowPos(hWnd, nullptr, xPos, yPos, 0, 0,
			SWP_NOSIZE | SWP_NOZORDER);
	}



	/**
	 * @brief													: 向上移动窗口
	 * 
	 * @param[in]		dt										: 移动距离
	 * @param[in]		mode									: 碰到边缘时的处理方式
	 */
	void MoveUp(int dt, int mode) {
		AutoUpdate();
		Move(0, -dt, mode);
	}



	/**
	 * @brief													: 向下移动窗口
	 * 
	 * @param[in]		dt										: 移动距离
	 * @param[in]		mode									: 碰到边缘时的处理方式
	 */
	void MoveDown(int dt, int mode) {
		AutoUpdate();
		Move(0, dt, mode);
	}



	/**
	 * @brief													: 向左移动窗口
	 * 
	 * @param[in]		dt										: 移动距离
	 * @param[in]		mode									: 碰到边缘时的处理方式
	 */
	void MoveLeft(int dt, int mode) {
		AutoUpdate();
		Move(-dt, 0, mode);
	}



	/**
	 * @brief													: 向右移动窗口
	 * 
	 * @param[in]		dt										: 移动距离
	 * @param[in]		mode									: 碰到边缘时的处理方式
	 */
	void MoveRight(int dt, int mode) {
		AutoUpdate();
		Move(dt, 0, mode);
	}



	/**
	 * @brief													: 窗口晃动
	 * 
	 * @param[in]		shakeCount								: 晃动次数,默认8次
	 * @param[in]		maxIntensity							: 最大晃动强度,默认15像素
	 */
	void Shake(int shakeCount = 8, int maxIntensity = 15) {
		AutoUpdate();

		int originalX = xPos, originalY = yPos;

		for (int i = 0; i < shakeCount; i++) {
			float decay = static_cast<float>(shakeCount - i) / shakeCount;
			int currentIntensity = static_cast<int>(
				maxIntensity * decay);							///< 计算衰减的强度

			if (currentIntensity < 1) currentIntensity = 1;

			SetWindowPos(hWnd, nullptr,
				originalX + currentIntensity, originalY,
				0, 0, SWP_NOSIZE | SWP_NOZORDER);
			Sleep(30);

			SetWindowPos(hWnd, nullptr,
				originalX - currentIntensity, originalY,
				0, 0, SWP_NOSIZE | SWP_NOZORDER);
			Sleep(30);											///< 左右晃动

			SetWindowPos(hWnd, nullptr,
				originalX, originalY + currentIntensity,
				0, 0, SWP_NOSIZE | SWP_NOZORDER);
			Sleep(30);

			SetWindowPos(hWnd, nullptr,
				originalX, originalY - currentIntensity,
				0, 0, SWP_NOSIZE | SWP_NOZORDER);
			Sleep(30);											///< 上下晃动
		}

		SetWindowPos(hWnd, nullptr,
			originalX, originalY,
			0, 0, SWP_NOSIZE | SWP_NOZORDER);					///< 恢复原始位置
	}



	/**
	 * @brief													: 自动更新窗口内容
	 * @details													: 如果窗口内容被修改,则更新内存设备上下文和像素数组
	 * 
	 * @param[in]		angle									: 旋转角度,正值为顺时针,负值为逆时针
	 * @param[in]		zoomX									: X轴缩放比例,默认1
	 * @param[in]		zoomY									: Y轴缩放比例,默认1
	 * @param[in]		offsetX									: X轴偏移量,默认0
	 * @param[in]		offsetY									: Y轴偏移量,默认0
	 * @param[in]		center									: 旋转中心,默认窗口中心
	 */
	void Rotate(
		float angle, float zoomX = 1, float zoomY = 1,
		int offsetX = 0, int offsetY = 0,
		POINT center = { -1, -1 }
	) const
	{
		AutoUpdate();
		if (center.x == -1 && center.y == -1) {
			center.x = windowWidth / 2;
			center.y = windowHeight / 2;
		}

		POINT pt{center.x, center.y};

		SIZE sz{
			static_cast<LONG>(windowWidth),
			static_cast<LONG>(windowHeight)
		};

		POINT ppt[3]{};
		angle = angle * (PI / 180.0f);
		float sina = sinf(angle), cosa = cosf(angle);
		ppt[0].x = static_cast<LONG>(
			pt.x + sina * pt.y - cosa * pt.x * zoomX + offsetX);
		ppt[0].y = static_cast<LONG>(
			pt.y - cosa * pt.y - sina * pt.x * zoomY + offsetY);
		ppt[1].x = static_cast<LONG>(
			ppt[0].x + cosa * sz.cx * zoomX + offsetX);
		ppt[1].y = static_cast<LONG>(
			ppt[0].y + sina * sz.cx * zoomY + offsetY);
		ppt[2].x = static_cast<LONG>(
			ppt[0].x - sina * sz.cy * zoomX + offsetX);
		ppt[2].y = static_cast<LONG>(
			ppt[0].y + cosa * sz.cy * zoomY + offsetY);
		PlgBlt(hdcWindow, ppt,
			hdcWindow, 0, 0, windowWidth, windowHeight,
			0, 0, 0);
	}



	/**
	 * @brief													: 向左旋转窗口内容
	 * 
	 * @param[in]		angle									: 旋转角度
	 */
	void turnLeft(float angle) const {
		AutoUpdate();
		Rotate(-angle, 1, 1, 0, 0, { -1, -1 });
	}



	/**
	 * @brief													: 向右旋转窗口内容
	 * 
	 * @param[in]		angle									: 旋转角度
	 */
	void turnRight(float angle) const {
		AutoUpdate();
		Rotate(angle, 1, 1, 0, 0, { -1, -1 });
	}



	/**
	 * @brief													: 绘制图像到位图
	 * 
	 * @param[in]		hBitmap									: 位图句柄
	 */
	void DrawImageToBitmap(HBITMAP hBitmap) const {
		AutoUpdate();
		HDC hdcBitmap = CreateCompatibleDC(hdcMem);
		SelectObject(hdcBitmap, hBitmap);

		BITMAP bmpInfo{};
		GetObject(hBitmap, sizeof(BITMAP), &bmpInfo);
		int bmpWidth = bmpInfo.bmWidth;
		int bmpHeight = bmpInfo.bmHeight;

		RECT clientRect;										///< 获取客户区大小
		GetClientRect(hWnd, &clientRect);
		int clientWidth = clientRect.right - clientRect.left;
		int clientHeight = clientRect.bottom - clientRect.top;

		int startX = (clientWidth - bmpWidth) / 2;
		int startY = (clientHeight - bmpHeight) / 2;			///< 计算居中位置

		BitBlt(hdcMem, startX, startY, bmpWidth, bmpHeight,
			hdcBitmap, 0, 0, SRCCOPY);							///< 绘制图像到内存设备上下文

		DeleteDC(hdcBitmap);
		InvalidateRect(hWnd, nullptr, TRUE);					///< 触发重绘
	}



	/**
	 * @brief													: 从资源加载并绘制图像
	 * 
	 * @param[in]		resourceID								: 资源ID
	 * 
	 * @throw			std::runtime_error
	 *					1)	LoadImageA 失败						: 资源未找到
	 */
	void LoadAndDrawImageFromResource(int resourceID) const {
		AutoUpdate();
		HBITMAP hBitmap = static_cast<HBITMAP>(LoadImageA(
			hInstance,
			MAKEINTRESOURCEA(resourceID),
			IMAGE_BITMAP,
			0, 0,
			LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS
		));

		if (hBitmap == NULL) {
			throw_runtime_error(
				"LoadImageA failed: "
				"Resource not found."
			);
			return;
		}

		DrawImageToBitmap(hBitmap);
		DeleteObject(hBitmap);
	}



	/**
	 * @brief													: 从文件加载并绘制图像
	 * 
	 * @param[in]		filePath								: 文件路径
	 * 
	 * @throw			std::runtime_error
	 *					1)	LoadImageA 失败						: 文件未找到
	 */
	void LoadAndDrawImageFromFile(const char* filePath) const {
		AutoUpdate();
		HBITMAP hBitmap = static_cast<HBITMAP>(LoadImageA(
			NULL,
			filePath,
			IMAGE_BITMAP,
			0, 0,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION
		));

		if (hBitmap == NULL) {
			throw_runtime_error(
				"LoadImageA failed: "
				"File not found."
			);
			return;
		}

		DrawImageToBitmap(hBitmap);
		DeleteObject(hBitmap);
	}



	/**
	 * @brief													: 调整亮度
	 * 
	 * @param[in]		factor									: 亮度因子,大于1变亮,小于1变暗
	 */
	void AdjustBrightness(float factor) {
		AutoUpdate();

		RECT clientRect;										///< 获取客户区大小
		GetClientRect(hWnd, &clientRect);
		int clientWidth = clientRect.right - clientRect.left;
		int clientHeight = clientRect.bottom - clientRect.top;

		BitBlt(hdcMem, 0, 0, clientWidth, clientHeight,
			hdcWindow, 0, 0, SRCCOPY);							///< 备份窗口内容

		for (int i = 0; i < clientWidth * clientHeight; i++) {
			HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
			hsl.l *= factor;
			hsl.l = (std::max)(0.0f, (std::min)(1.0f, hsl.l));	///< 保持亮度在合理范围内
			_RGBQUAD rgb = HSLToRGB(hsl);
			rgbScreen[i].r = rgb.r;
			rgbScreen[i].g = rgb.g;
			rgbScreen[i].b = rgb.b;
		}														///< 调整亮度

		BitBlt(hdcWindow, 0, 0, clientWidth, clientHeight,
			hdcMem, 0, 0, SRCCOPY);								///< 应用修改
		InvalidateRect(hWnd, nullptr, TRUE);
	}



	/**
	 * @brief													: 调整对比度
	 * 
	 * @param[in]		factor									: 对比度因子,大于1鲜艳,小于1暗淡
	 */
	void AdjustContrast(float factor) {
		AutoUpdate();
		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		int clientWidth = clientRect.right - clientRect.left;
		int clientHeight = clientRect.bottom - clientRect.top;

		BitBlt(hdcMem, 0, 0, clientWidth, clientHeight,
			hdcWindow, 0, 0, SRCCOPY);

		for (int i = 0; i < clientWidth * clientHeight; i++) {
			HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
			hsl.l = 0.5f + (hsl.l - 0.5f) * factor;
	   
			_RGBQUAD rgb = HSLToRGB(hsl);
			rgbScreen[i].r = rgb.r;
			rgbScreen[i].g = rgb.g;
			rgbScreen[i].b = rgb.b;
		}

		BitBlt(hdcWindow, 0, 0, clientWidth, clientHeight, hdcMem, 0, 0, SRCCOPY);
		InvalidateRect(hWnd, nullptr, TRUE);
	}



	/**
	 * @brief													: 调整饱和度
	 * 
	 * @param[in]		factor									: 饱和度因子,大于1鲜艳,小于1灰暗
	 */
	void AdjustSaturation(float factor) {
		AutoUpdate();
		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		int clientWidth = clientRect.right - clientRect.left;
		int clientHeight = clientRect.bottom - clientRect.top;

		BitBlt(hdcMem, 0, 0, clientWidth, clientHeight,
			hdcWindow, 0, 0, SRCCOPY);

		for (int i = 0; i < clientWidth * clientHeight; i++) {
			HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
			hsl.s *= factor;
			hsl.s = (std::max)(0.0f, (std::min)(1.0f, hsl.s));
			_RGBQUAD rgb = HSLToRGB(hsl);
			rgbScreen[i].r = rgb.r;
			rgbScreen[i].g = rgb.g;
			rgbScreen[i].b = rgb.b;
		}

		BitBlt(hdcWindow, 0, 0, clientWidth, clientHeight,
			hdcMem, 0, 0, SRCCOPY);
		InvalidateRect(hWnd, nullptr, TRUE);
	}



	/**
	 * @brief													: 设置窗口标题
	 * 
	 * @param[in]		title									: 新标题
	 */
	void SetWindowTitle(const std::string& title) const{
		SetWindowTextA(hWnd, title.c_str());
	}



	/**
	 * @brief													: 获取窗口客户区大小
	 * @return													: 客户区大小
	 */
	SIZE GetClientSize() const {
		RECT rect;
		GetClientRect(hWnd, &rect);
		return { rect.right - rect.left,
			rect.bottom - rect.top };
	}

private:
	bool hasCollided = false;									///< 标志是否已经发生碰撞



	/**
	 * @brief													: 初始化 GDI 资源
	 */
	void initialization()
	{
		hdcWindow = GetDC(hWnd);
		hdcMem = CreateCompatibleDC(hdcWindow);

		RECT clientRect;										///< 获取客户区大小
		GetClientRect(hWnd, &clientRect);
		int clientWidth = clientRect.right - clientRect.left;
		int clientHeight = clientRect.bottom - clientRect.top;

		BITMAPINFO bmi = { 0 };
		bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biWidth = clientWidth;
		bmi.bmiHeader.biHeight = -clientHeight;					///< 负值表示从上到下的位图
		hbmTemp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS,
			(void**)&rgbScreen, NULL, 0);
		if (!hbmTemp)
		{
			DeleteDC(hdcMem);
			ReleaseDC(nullptr, hdcWindow);
			throw_runtime_error(
				"CreateDIBSection failed: "
				"out of memory or invalid parameters"
			);
		}
		SelectObject(hdcMem, hbmTemp);
	}

	

	/**
	 * @brief													: 自动处理窗口消息
	 */
	void AutoUpdate() const {
		MSG msg;
		while (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}



	/**
	 * @brief													: 窗口过程函数
	 * @return													: 消息处理结果,0表示消息已处理,非0表示未处理
	 * 
	 * @param[in]		hWnd									: 窗口句柄
	 * @param[in]		msg										: 消息代码
	 * @param[in]		wParam									: 附加消息参数
	 * @param[in]		lParam									: 附加消息参数
	 */
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		BorderedWindowGDI* window = reinterpret_cast<BorderedWindowGDI*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		switch (msg) {
		case WM_CREATE: {
			SetWindowLongPtr(hWnd, GWLP_USERDATA,
				(LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
			break;
		}
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);

			if (window) {
				RECT clientRect;
				GetClientRect(hWnd, &clientRect);				///< 获取客户区大小

				BitBlt(hdc, 0, 0,
					clientRect.right, clientRect.bottom,
					window->hdcMem, 0, 0, SRCCOPY);				///< 绘制内存DC到窗口
			}

			EndPaint(hWnd, &ps);
			break;
		}
		case WM_ERASEBKGND:
			return 1;											///< 防止闪烁

		case WM_SIZE: {
			if (window) {
				RECT clientRect;								///< 获取客户区大小
				GetClientRect(hWnd, &clientRect);
				int clientWidth = clientRect.right - clientRect.left;
				int clientHeight = clientRect.bottom - clientRect.top;

				if (window->hbmTemp)
					DeleteObject(window->hbmTemp);
				if (window->hdcMem)
					DeleteDC(window->hdcMem);					///< 释放旧的DC和位图

				window->hdcMem = CreateCompatibleDC(window->hdcWindow);
				BITMAPINFO bmi = { 0 };
				bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
				bmi.bmiHeader.biBitCount = 32;
				bmi.bmiHeader.biPlanes = 1;
				bmi.bmiHeader.biWidth = clientWidth;
				bmi.bmiHeader.biHeight = -clientHeight;
				window->hbmTemp = CreateDIBSection(window->hdcMem,
					&bmi, DIB_RGB_COLORS,
					(void**)&window->rgbScreen, NULL, 0);		///< 创建新的DC和位图
				if (!window->hbmTemp)
				{
					DeleteDC(window->hdcMem);
					ReleaseDC(nullptr, window->hdcWindow);
					throw_runtime_error(
						"CreateDIBSection failed: "
						"out of memory or invalid parameters"
					);
				}
				SelectObject(window->hdcMem, window->hbmTemp);

				RECT rect = { 0, 0, clientWidth, clientHeight };
				HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
				FillRect(window->hdcMem, &rect, hBrush);		///< 用白色填充新位图
				DeleteObject(hBrush);

				InvalidateRect(hWnd, nullptr, TRUE);
			}
			break;
		}
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE) {}				///< 窗口失去焦点时不作处理
			break;
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
#endif															///< !BORDER_WINDOW_GDI_HPP
