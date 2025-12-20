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
 * @details														: 封装对“某个 DC（默认桌面 DC）”的 GDI 操作（兼容旧 API）
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
	 * @brief														: 初始化 ScreenGDI（兼容旧行为：直接操作桌面 DC）
	 * 
	 * @throw				std::runtime_error
	 *						1)	GetDC failed						: GDI 资源枯竭或桌面不可用
	 *						2)	CreateCompatibleDC failed			: 无法创建兼容内存 DC
	 *						3)	CreateDIBSection failed				: 内存不足或参数无效
	 */
	ScreenGDI();

	/**
	 * @brief														: 初始化 ScreenGDI（新接口：传入要操作的 DC）
	 * @details														: 适用于“窗口 DC / 内存 DC / 打印机 DC”等；不接管外部 DC 的释放
	 *
	 * @param[in]			targetDC								: 目标设备上下文（由调用者管理生命周期）
	 */
	explicit ScreenGDI(HDC targetDC);

	/**
	 * @brief														: 初始化 ScreenGDI（新接口：传入 DC + 显式尺寸）
	 * @details														: 当 DC 的裁剪区域尺寸不可靠时使用
	 */
	ScreenGDI(HDC targetDC, int targetWidth, int targetHeight);

	/**
	 * @brief														: 工厂函数：从窗口创建 ScreenGDI（内部会 GetDC/ReleaseDC）
	 */
	static ScreenGDI FromWindow(HWND hWnd);



	/**
	 * @brief													: 释放 GDI 资源
	 * @details													: noexcept
	 */
	~ScreenGDI() noexcept;

	ScreenGDI(const ScreenGDI&) = delete;
	ScreenGDI& operator=(const ScreenGDI&) = delete;
	ScreenGDI(ScreenGDI&& other) noexcept;
	ScreenGDI& operator=(ScreenGDI&& other) noexcept;



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

private:
	struct RectI {
		int startX;
		int startY;
		int endX;
		int endY;
	};

	struct EmptyTag { explicit EmptyTag() = default; };
	explicit ScreenGDI(EmptyTag) noexcept;

	HWND releaseWnd_;											///< 仅当内部持有窗口 DC 时使用（ReleaseDC 需要 HWND）
	bool ownsDesktopDC_;										///< 是否接管 hdcDesktop 的释放
	HGDIOBJ oldTempBmp_;										///< hdcMem 原先选入的位图（析构时还原）

	void Reset_() noexcept;
	void Init_(HDC targetDC, int targetWidth, int targetHeight, bool ownsDc, HWND releaseWnd);
	void Capture_() const;										///< 目标 DC -> 内存 DIB
	void Present_() const;										///< 内存 DIB -> 目标 DC
	[[nodiscard]] RectI ClampRect_(int xStart, int yStart, int xEnd, int yEnd) const;
	void EnsurePixels_() const;
};


#endif															///< !SCREEN_GDI_HPP
