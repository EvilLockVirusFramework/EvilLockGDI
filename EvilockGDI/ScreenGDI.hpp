/*****************************************************************//**
 * @file				ScreenGDI.hpp
 * @brief				屏幕GDI操作
 * 
 * @author				EvilLockVirusFramework
 * @date				2024-01-06
 *********************************************************************/



#ifndef SCREEN_GDI_HPP
#define SCREEN_GDI_HPP



#pragma once
#include "common.hpp"
#include "color.hpp"



/**
 * @brief														: 屏幕GDI操作
 * @details														: 封装对屏幕的GDI操作
 */
class ScreenGDI {
public:
	HDC hdcDesktop;												///< 桌面设备上下文
	HDC hdcMem;													///< 内存设备上下文
	int width;													///< 桌面设备宽
	int height;													///< 桌面设备高
	HBITMAP hbmTemp;											///< 临时位图
	PRGBQUAD rgbScreen;											///< 像素数组



	/**
	 * @brief														: 初始化ScreenGDI类内部变量
	 * 
	 * @throw				std::runtime_error
	 *						1)	GetDC failed						: GDI 资源枯竭或桌面不可用
	 *						2)	CreateCompatibleDC failed			: 无法创建兼容内存 DC
	 *						3)	CreateDIBSection failed				: 内存不足或参数无效
	 */
	ScreenGDI() {
		hdcDesktop = GetDC(nullptr);							///< 获取桌面设备上下文
		if (!hdcDesktop)
			throw_runtime_error(
				"GetDC failed: "
				"GDI resource exhaustion "
				"or desktop unavailable."
			);

		hdcMem = CreateCompatibleDC(hdcDesktop);
		if (!hdcMem)
			throw_runtime_error(
				"CreateCompatibleDC failed: "
				"unable to create memory DC."
			);

		width = GetSystemMetrics(SM_CXSCREEN);					///< 获取屏幕宽度
		height = GetSystemMetrics(SM_CYSCREEN);					///< 获取屏幕高度
		
		rgbScreen = nullptr;
		BITMAPINFO bmi = { 0 };
		bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
		bmi.bmiHeader.biWidth = width;
		bmi.bmiHeader.biHeight = height;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		hbmTemp = CreateDIBSection(
			hdcMem, &bmi, DIB_RGB_COLORS,
			reinterpret_cast<void**>(&rgbScreen), nullptr, 0
		);														///< 创建临时位图
		if (!hbmTemp)
		{
			DeleteDC(hdcMem);
			ReleaseDC(nullptr, hdcDesktop);
			throw_runtime_error(
				"CreateDIBSection failed: "
				"out of memory or invalid parameters"
			);
		}
		SelectObject(hdcMem, hbmTemp);							///< 关联像素数组
	}



	/**
	 * @brief													: 释放 GDI 资源
	 * @details													: noexcept
	 */
	~ScreenGDI() noexcept
	{
		ReleaseDC(nullptr, hdcDesktop);							///< 释放桌面设备上下文
		DeleteDC(hdcMem);										///< 删除内存设备上下文
		DeleteObject(hbmTemp);									///< 删除临时位图
	}



	void DrawImageToBitmap(HBITMAP hBitmap) const;				///< 将传入位图绘制到内存设备上
	void LoadAndDrawImageFromResource(int resourceID) const;
	void AdjustRGB(
		int xStart, int yStart, int xEnd, int yEnd,
		int rIncrease, int gIncrease, int bIncrease
	);															///< 调整某个矩形区域RGB的相对值
	void SetRGB(
		int xStart, int yStart, int xEnd, int yEnd,
		BYTE newR, BYTE newG, BYTE newB
	);															///< 调整某个矩形区域RGB的绝对值
	void AdjustBrightness(float factor);						///< 调整亮度 范围为0.f～1.f
	void AdjustContrast(float factor);							///< 调整对比度 范围为0.f～1.f
	void AdjustSaturation(float factor);						///< 调整饱和度 范围为0.f～1.f
};



/** 
 * @brief														: 将传入位图绘制到内存设备上
 * 
 * @param[in]			hBitmap									: 传入位图
 * 
 * @throw				std::runtime_error
 *						1)	CreateCompatibleDC 失败				: 无法创建兼容内存 DC
 *						2)	SelectObject 失败					: hBitmap 无效或与 hdcBitmap 不兼容
 *						3)	GetObject 失败						: 无法获取位图属性
 *						4)	BitBlt 失败							: DC不兼容或绘制区域越界
 */
void ScreenGDI::DrawImageToBitmap(HBITMAP hBitmap) const
{
	HDC hdcBitmap = CreateCompatibleDC(hdcMem);
	if (!hdcBitmap)
		throw_runtime_error(
			"CreateCompatibleDC failed: "
			"unable to create memory DC."
		);

	HGDIOBJ hOldBmp = SelectObject(hdcBitmap, hBitmap);
	if (hOldBmp == nullptr || hOldBmp == HGDI_ERROR)
	{
		DeleteDC(hdcBitmap);
		throw_runtime_error(
			"SelectObject failed: "
			"hBitmap invalid or incompatible."
		);
	}

	BITMAP bmp{};
	if (!GetObject(hBitmap, sizeof(BITMAP), &bmp))
	{
		SelectObject(hdcBitmap, hOldBmp);						///< 还原成原HBITMAP
		DeleteDC(hdcBitmap);
		throw_runtime_error(
			"GetObject failed: "
			"hBitmap is not a valid HBITMAP."
		);
	}

	if (!BitBlt(hdcMem,
		0, 0, bmp.bmWidth, bmp.bmHeight,
		hdcBitmap, 0, 0, SRCCOPY))
	{
		SelectObject(hdcBitmap, hOldBmp);						///< 还原成原HBITMAP
		DeleteDC(hdcBitmap);
		throw_runtime_error(
			"BitBlt failed: "
			"incompatible DC or rectangle out of bounds."
		);
	}

	SelectObject(hdcBitmap, hOldBmp);
	DeleteDC(hdcBitmap);										///< 还原成原HBITMAP并删去兼容内存DC
}



/**
 * @brief														: 调整某个矩形区域RGB的相对值
 * @details														: 基于每像素原有RGB进行调整（正数是增加,负数是减少）
 * 
 * @param[in]			xStart									: 矩形左上角X坐标
 * @param[in]			yStart									: 矩形左上角Y坐标
 * @param[in]			xEnd									: 矩形右下角X坐标
 * @param[in]			yEnd									: 矩形右下角Y坐标
 * @param[in]			rIncrease								: 每像素Red增长值
 * @param[in]			gIncrease								: 每像素Green增长值
 * @param[in]			bIncrease								: 每像素Blue增长值
 * 
 * @throw				std::runtime_error
 *						1)	BitBlt 失败							: DC不兼容或绘制区域越界
 *						2)	AdjustRGB 失败						: 尝试使用一个无效矩形区域
 *						3)	AdjustRGB 失败						: rgbScreen 未初始化
 */
void ScreenGDI::AdjustRGB(
	int xStart, int yStart, int xEnd, int yEnd,
	int rIncrease, int gIncrease, int bIncrease
)
{
	if (!BitBlt(
		hdcMem, 0, 0, width, height,
		hdcDesktop, 0, 0, SRCCOPY
	))
		throw_runtime_error(
			"BitBlt(hdcMem) failed: "
			"incompatible DC or rectangle out of bounds."
		);

	int startX = std::clamp(xStart, 0, width - 1);
	int startY = std::clamp(yStart, 0, height - 1);
	int endX = std::clamp(xEnd, 0, width - 1);
	int endY = std::clamp(yEnd, 0, height - 1);
	if (startX > endX || startY > endY)
		throw_runtime_error(
			"AdjustRGB failed: "
			"try to use an invalid rectangle."
		);
	if (rgbScreen == nullptr)
		throw_runtime_error(
			"AdjustRGB failed: "
			"rgbScreen invalid."
		);

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
	}															///< 填充目标区域
	
	if(!BitBlt(
		hdcDesktop, 0, 0, width, height, 
		hdcMem, 0, 0, SRCCOPY
	))
		throw_runtime_error(
			"BitBlt(hdcDesktop) failed: "
			"incompatible DC or rectangle out of bounds."
		);
}



/**
 * @brief														: 调整某个矩形区域RGB的绝对值
 * @details														: 直接修改某个矩形区域内的RGB
 * 
 * @param[in]			xStart									: 矩形左上角X坐标
 * @param[in]			yStart									: 矩形左上角Y坐标
 * @param[in]			xEnd									: 矩形右下角X坐标
 * @param[in]			yEnd									: 矩形右下角Y坐标
 * @param[in]			newR									: 每像素的Red值
 * @param[in]			newG									: 每像素的Green值
 * @param[in]			newB									: 每像素的Blue值
 * 
 * @throw				std::runtime_error
 *						1)	BitBlt 失败							: DC不兼容或绘制区域越界
 *						2)	AdjustRGB 失败						: 尝试使用一个无效矩形区域
 *						3)	AdjustRGB 失败						: rgbScreen 未初始化
 */
void ScreenGDI::SetRGB(
	int xStart, int yStart, int xEnd, int yEnd,
	BYTE newR, BYTE newG, BYTE newB
)
{
	if (!BitBlt(
		hdcMem, 0, 0, width, height,
		hdcDesktop, 0, 0, SRCCOPY
	))
		throw_runtime_error(
			"BitBlt(hdcMem) failed: "
			"incompatible DC or rectangle out of bounds."
		);

	int startX = std::clamp(xStart, 0, width - 1);
	int startY = std::clamp(yStart, 0, height - 1);
	int endX = std::clamp(xEnd, 0, width - 1);
	int endY = std::clamp(yEnd, 0, height - 1);
	if (startX > endX || startY > endY)
		throw_runtime_error(
			"SetRGB failed: "
			"try to use an invalid rectangle."
		);
	if (rgbScreen == nullptr)
		throw_runtime_error(
			"SetRGB failed: "
			"rgbScreen invalid."
		);

	for (int y = startY; y <= endY; y++) {
		for (int x = startX; x <= endX; x++) {
			int index = y * width + x;
			rgbScreen[index].r = newR;
			rgbScreen[index].g = newG;
			rgbScreen[index].b = newB;
		}
	}

	if (!BitBlt(
		hdcDesktop, 0, 0, width, height,
		hdcMem, 0, 0, SRCCOPY
	))
		throw_runtime_error(
			"BitBlt(hdcDesktop) failed: "
			"incompatible DC or rectangle out of bounds."
		);
}



/**
 * @brief														: 调整桌面设备的亮度
 * @details														: 当 factor 小于等于 0 时为全黑
 * 
 * @param[in]			factor									: 亮度系数
 * 
 * @throw				std::runtime_error
 *						1)	AdjustBrightness 失败				: rgbScreen 未初始化
 *						2)	BitBlt 失败							: DC不兼容或绘制区域越界
 */
void ScreenGDI::AdjustBrightness(float factor) {
	if (rgbScreen == nullptr)
		throw_runtime_error(
			"AdjustBrightness failed: "
			"rgbScreen invalid."
		);

	if (!BitBlt(
		hdcMem, 0, 0, width, height,
		hdcDesktop, 0, 0, SRCCOPY
	))
		throw_runtime_error(
			"BitBlt(hdcMem) failed: "
			"incompatible DC or rectangle out of bounds."
		);

	for (int i = 0; i < width * height; i++) {
		HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
		hsl.l *= factor;
		hsl.l = std::clamp(hsl.l, 0.0f, 1.0f);
		rgbScreen[i] = HSLToRGB(hsl);
	}

	if (!BitBlt(
		hdcDesktop, 0, 0, width, height,
		hdcMem, 0, 0, SRCCOPY
	))
		throw_runtime_error(
			"BitBlt(hdcDesktop) failed: "
			"incompatible DC or rectangle out of bounds."
		);
}




/**
 * @brief														: 调整桌面设备的对比度
 *
 * @param[in]			factor									: 对比度系数
 * 
 * @throw				std::runtime_error
 *						1)	AdjustContrast 失败					: rgbScreen 未初始化
 *						2)	BitBlt 失败							: DC不兼容或绘制区域越界
 */
void ScreenGDI::AdjustContrast(float factor) {
	if (rgbScreen == nullptr)
		throw_runtime_error(
			"AdjustContrast failed: "
			"rgbScreen invalid."
		);

	if (!BitBlt(
		hdcMem, 0, 0, width, height,
		hdcDesktop, 0, 0, SRCCOPY
	))
		throw_runtime_error(
			"BitBlt(hdcMem) failed: "
			"incompatible DC or rectangle out of bounds."
		);

	for (int i = 0; i < width * height; i++) {
		HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
		hsl.l = 0.5f + (hsl.l - 0.5f) * factor;
		hsl.l = std::clamp(hsl.l, 0.0f, 1.0f);
		rgbScreen[i] = HSLToRGB(hsl);
	}

	if (!BitBlt(
		hdcDesktop, 0, 0, width, height,
		hdcMem, 0, 0, SRCCOPY
	))
		throw_runtime_error(
			"BitBlt(hdcDesktop) failed: "
			"incompatible DC or rectangle out of bounds."
		);
}



/**
 * @brief														: 调整桌面设备的饱和度
 *
 * @param[in]			factor									: 饱和度系数
 * 
 * @throw				std::runtime_error
 *						1)	AdjustSaturation 失败				: rgbScreen 未初始化
 *						2)	BitBlt 失败							: DC不兼容或绘制区域越界
 */
void ScreenGDI::AdjustSaturation(float factor) {
	if (rgbScreen == nullptr)
		throw_runtime_error(
			"AdjustSaturation failed: "
			"rgbScreen invalid."
		);

	if (!BitBlt(
		hdcMem, 0, 0, width, height,
		hdcDesktop, 0, 0, SRCCOPY
	))
		throw_runtime_error(
			"BitBlt(hdcMem) failed: "
			"incompatible DC or rectangle out of bounds."
		);

	for (int i = 0; i < width * height; i++) {
		HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
		hsl.s *= factor;
		hsl.s = std::clamp(hsl.s, 0.0f, 1.0f);
		rgbScreen[i] = HSLToRGB(hsl);
	}

	if (!BitBlt(
		hdcDesktop, 0, 0, width, height,
		hdcMem, 0, 0, SRCCOPY
	))
		throw_runtime_error(
			"BitBlt(hdcDesktop) failed: "
			"incompatible DC or rectangle out of bounds."
		);
}



#endif															///< !SCREEN_GDI_HPP
