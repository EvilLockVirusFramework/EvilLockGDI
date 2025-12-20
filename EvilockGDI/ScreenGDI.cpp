#include "ScreenGDI.hpp"
#include "gdi_raii.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace {
	[[nodiscard]] BYTE ClampByte(int v) noexcept {
		return static_cast<BYTE>(std::clamp(v, 0, 255));
	}
}

void ScreenGDI::Reset_() noexcept
{
	hdcDesktop = nullptr;
	hdcMem = nullptr;
	width = 0;
	height = 0;
	hbmTemp = nullptr;
	rgbScreen = nullptr;
	releaseWnd_ = nullptr;
	ownsDesktopDC_ = false;
	oldTempBmp_ = nullptr;
}

ScreenGDI::ScreenGDI(EmptyTag) noexcept
{
	Reset_();
}

void ScreenGDI::Init_(HDC targetDC, int targetWidth, int targetHeight, bool ownsDc, HWND releaseWnd)
{
	Reset_();

	if (!targetDC)
		throw_runtime_error("ScreenGDI init failed: targetDC is null.");
	if (targetWidth <= 0 || targetHeight <= 0)
		throw_runtime_error("ScreenGDI init failed: invalid width/height.");

	HDC memDC = CreateCompatibleDC(targetDC);
	if (!memDC)
	{
		if (ownsDc) ReleaseDC(releaseWnd, targetDC);
		throw_runtime_error("CreateCompatibleDC failed: unable to create memory DC.");
	}

	PRGBQUAD pixels = nullptr;
	BITMAPINFO bmi{};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = targetWidth;
	bmi.bmiHeader.biHeight = targetHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	HBITMAP dib = CreateDIBSection(
		memDC, &bmi, DIB_RGB_COLORS,
		reinterpret_cast<void**>(&pixels), nullptr, 0
	);
	if (!dib)
	{
		DeleteDC(memDC);
		if (ownsDc) ReleaseDC(releaseWnd, targetDC);
		throw_runtime_error("CreateDIBSection failed: out of memory or invalid parameters.");
	}

	HGDIOBJ oldBmp = SelectObject(memDC, dib);
	if (oldBmp == nullptr || oldBmp == HGDI_ERROR)
	{
		DeleteObject(dib);
		DeleteDC(memDC);
		if (ownsDc) ReleaseDC(releaseWnd, targetDC);
		throw_runtime_error("SelectObject failed: unable to select DIBSection into memory DC.");
	}

	hdcDesktop = targetDC;
	hdcMem = memDC;
	width = targetWidth;
	height = targetHeight;
	hbmTemp = dib;
	rgbScreen = pixels;
	releaseWnd_ = releaseWnd;
	ownsDesktopDC_ = ownsDc;
	oldTempBmp_ = oldBmp;
}

ScreenGDI::ScreenGDI()
{
	HDC desktop = GetDC(nullptr);
	if (!desktop)
		throw_runtime_error("GetDC failed: GDI resource exhaustion or desktop unavailable.");

	const int w = GetSystemMetrics(SM_CXSCREEN);
	const int h = GetSystemMetrics(SM_CYSCREEN);
	Init_(desktop, w, h, true, nullptr);
}

ScreenGDI::ScreenGDI(HDC targetDC)
{
	if (!targetDC)
		throw_runtime_error("ScreenGDI ctor failed: targetDC is null.");

	RECT clip{};
	int w = 0;
	int h = 0;

	const int clipResult = GetClipBox(targetDC, &clip);
	if (clipResult != ERROR)
	{
		w = clip.right - clip.left;
		h = clip.bottom - clip.top;
	}

	if (w <= 0 || h <= 0)
	{
		w = GetDeviceCaps(targetDC, HORZRES);
		h = GetDeviceCaps(targetDC, VERTRES);
	}

	if (w <= 0 || h <= 0)
		throw_runtime_error("ScreenGDI ctor failed: unable to infer target size.");

	Init_(targetDC, w, h, false, nullptr);
}

ScreenGDI::ScreenGDI(HDC targetDC, int targetWidth, int targetHeight)
{
	Init_(targetDC, targetWidth, targetHeight, false, nullptr);
}

ScreenGDI ScreenGDI::FromWindow(HWND hWnd)
{
	if (!hWnd)
		throw_runtime_error("ScreenGDI::FromWindow failed: hWnd is null.");

	RECT rect{};
	if (!GetClientRect(hWnd, &rect))
		throw_runtime_error("ScreenGDI::FromWindow failed: GetClientRect failed.");

	const int w = rect.right - rect.left;
	const int h = rect.bottom - rect.top;

	HDC hdc = GetDC(hWnd);
	if (!hdc)
		throw_runtime_error("ScreenGDI::FromWindow failed: GetDC failed.");

	ScreenGDI out(EmptyTag{});
	out.Init_(hdc, w, h, true, hWnd);
	return out;
}

ScreenGDI::ScreenGDI(ScreenGDI&& other) noexcept
{
	hdcDesktop = std::exchange(other.hdcDesktop, nullptr);
	hdcMem = std::exchange(other.hdcMem, nullptr);
	width = std::exchange(other.width, 0);
	height = std::exchange(other.height, 0);
	hbmTemp = std::exchange(other.hbmTemp, nullptr);
	rgbScreen = std::exchange(other.rgbScreen, nullptr);
	releaseWnd_ = std::exchange(other.releaseWnd_, nullptr);
	ownsDesktopDC_ = std::exchange(other.ownsDesktopDC_, false);
	oldTempBmp_ = std::exchange(other.oldTempBmp_, nullptr);
}

ScreenGDI& ScreenGDI::operator=(ScreenGDI&& other) noexcept
{
	if (this == &other) return *this;

	this->~ScreenGDI();
	new (this) ScreenGDI(std::move(other));
	return *this;
}

ScreenGDI::~ScreenGDI() noexcept
{
	if (hdcMem && oldTempBmp_)
	{
		SelectObject(hdcMem, oldTempBmp_);
	}
	if (hbmTemp)
	{
		DeleteObject(hbmTemp);
	}
	if (hdcMem)
	{
		DeleteDC(hdcMem);
	}
	if (ownsDesktopDC_ && hdcDesktop)
	{
		ReleaseDC(releaseWnd_, hdcDesktop);
	}

	Reset_();
}

void ScreenGDI::EnsurePixels_() const
{
	if (rgbScreen == nullptr)
		throw_runtime_error("ScreenGDI failed: rgbScreen invalid.");
}

ScreenGDI::RectI ScreenGDI::ClampRect_(int xStart, int yStart, int xEnd, int yEnd) const
{
	const int startX = std::clamp(xStart, 0, width - 1);
	const int startY = std::clamp(yStart, 0, height - 1);
	const int endX = std::clamp(xEnd, 0, width - 1);
	const int endY = std::clamp(yEnd, 0, height - 1);

	if (startX > endX || startY > endY)
		throw_runtime_error("ScreenGDI failed: try to use an invalid rectangle.");

	return RectI{ startX, startY, endX, endY };
}

void ScreenGDI::Capture_() const
{
	if (!BitBlt(hdcMem, 0, 0, width, height, hdcDesktop, 0, 0, SRCCOPY))
		throw_runtime_error("BitBlt(hdcMem) failed: incompatible DC or rectangle out of bounds.");
}

void ScreenGDI::Present_() const
{
	if (!BitBlt(hdcDesktop, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY))
		throw_runtime_error("BitBlt(hdcDesktop) failed: incompatible DC or rectangle out of bounds.");
}

void ScreenGDI::DrawImageToBitmap(HBITMAP hBitmap) const
{
	if (!hBitmap)
		throw_runtime_error("DrawImageToBitmap failed: hBitmap is null.");

	evgdi::win::unique_hdc hdcBitmap(CreateCompatibleDC(hdcMem));
	if (!hdcBitmap)
		throw_runtime_error("CreateCompatibleDC failed: unable to create memory DC.");

	evgdi::win::select_object_guard bmpSel(hdcBitmap.get(), hBitmap);

	BITMAP bmp{};
	if (!GetObject(hBitmap, sizeof(BITMAP), &bmp))
	{
		throw_runtime_error("GetObject failed: hBitmap is not a valid HBITMAP.");
	}

	if (!BitBlt(hdcMem, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcBitmap.get(), 0, 0, SRCCOPY))
	{
		throw_runtime_error("BitBlt failed: incompatible DC or rectangle out of bounds.");
	}
}

void ScreenGDI::LoadAndDrawImageFromResource(int resourceID) const
{
	const HINSTANCE hInst = GetModuleHandleW(nullptr);
	evgdi::win::unique_hbitmap hBitmap(static_cast<HBITMAP>(LoadImageW(
		hInst,
		MAKEINTRESOURCEW(resourceID),
		IMAGE_BITMAP,
		0, 0,
		LR_CREATEDIBSECTION
	)));

	if (!hBitmap)
		throw_runtime_error("LoadAndDrawImageFromResource failed: LoadImageW failed.");

	DrawImageToBitmap(hBitmap.get());
	Present_();
}

void ScreenGDI::AdjustRGB(
	int xStart, int yStart, int xEnd, int yEnd,
	int rIncrease, int gIncrease, int bIncrease
)
{
	EnsurePixels_();
	Capture_();
	const auto rect = ClampRect_(xStart, yStart, xEnd, yEnd);

	for (int y = rect.startY; y <= rect.endY; ++y)
	{
		PRGBQUAD row = rgbScreen + (static_cast<std::size_t>(y) * static_cast<std::size_t>(width));
		for (int x = rect.startX; x <= rect.endX; ++x)
		{
			auto& px = row[x];
			px.r = ClampByte(static_cast<int>(px.r) + rIncrease);
			px.g = ClampByte(static_cast<int>(px.g) + gIncrease);
			px.b = ClampByte(static_cast<int>(px.b) + bIncrease);
		}
	}

	Present_();
}

void ScreenGDI::SetRGB(
	int xStart, int yStart, int xEnd, int yEnd,
	BYTE newR, BYTE newG, BYTE newB
)
{
	EnsurePixels_();
	Capture_();
	const auto rect = ClampRect_(xStart, yStart, xEnd, yEnd);

	for (int y = rect.startY; y <= rect.endY; ++y)
	{
		PRGBQUAD row = rgbScreen + (static_cast<std::size_t>(y) * static_cast<std::size_t>(width));
		for (int x = rect.startX; x <= rect.endX; ++x)
		{
			auto& px = row[x];
			px.r = newR;
			px.g = newG;
			px.b = newB;
		}
	}

	Present_();
}

void ScreenGDI::AdjustBrightness(float factor)
{
	EnsurePixels_();
	Capture_();

	const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
	for (std::size_t i = 0; i < pixelCount; ++i)
	{
		HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
		hsl.l *= factor;
		hsl.l = std::clamp(hsl.l, 0.0f, 1.0f);
		rgbScreen[i] = HSLToRGB(hsl);
	}

	Present_();
}

void ScreenGDI::AdjustContrast(float factor)
{
	EnsurePixels_();
	Capture_();

	const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
	for (std::size_t i = 0; i < pixelCount; ++i)
	{
		HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
		hsl.l = 0.5f + (hsl.l - 0.5f) * factor;
		hsl.l = std::clamp(hsl.l, 0.0f, 1.0f);
		rgbScreen[i] = HSLToRGB(hsl);
	}

	Present_();
}

void ScreenGDI::AdjustSaturation(float factor)
{
	EnsurePixels_();
	Capture_();

	const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
	for (std::size_t i = 0; i < pixelCount; ++i)
	{
		HSLQUAD hsl = RGBToHSL(rgbScreen[i]);
		hsl.s *= factor;
		hsl.s = std::clamp(hsl.s, 0.0f, 1.0f);
		rgbScreen[i] = HSLToRGB(hsl);
	}

	Present_();
}
