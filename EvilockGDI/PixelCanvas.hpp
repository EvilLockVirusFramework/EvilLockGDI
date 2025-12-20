/*****************************************************************//**
 * @file                PixelCanvas.hpp
 * @brief               像素画板（GDI，Header-Only）
 *
 * @details
 * 你给的 HuaPing3 代码做的事情其实很简单：
 * 1) 从某个 DC 抓一帧到 32bpp 的 DIB 像素缓冲
 * 2) 直接改内存里的像素
 * 3) 再把缓冲贴回去
 *
 * 这个头文件把上面流程封装成一个更“像库”的小类：
 * - 默认操作屏幕 DC（GetDC(nullptr)）
 * - 也可以传入任意 HDC（窗口 DC / 内存 DC / 打印机 DC...）
 * - 全程 RAII：创建的 DC/Bitmap 会自动释放，避免泄漏
 *
 * @date                2025-12-20
 *********************************************************************/

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#if defined(min)
#undef min
#endif
#if defined(max)
#undef max
#endif

#include "color.hpp"

#include <gdiplus.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <source_location>
#include <stdexcept>
#include <string>
#include <utility>

#if defined(_MSC_VER)
#pragma comment(lib, "gdiplus.lib")
#endif

namespace evgdi {

	/**
	 * @brief 32bpp 像素（Windows DIB 通常是 BGRA 顺序）
	 *
	 * @details
	 * - `rgb` 只是一个 32 位“打包值”（并不等同于 COLORREF 的 0x00BBGGRR 含义），
	 *   用它做简单的加/乘/异或等特效很方便。
	 * - 需要精确控制颜色时，可以用 b/g/r/a 分量。
	 */
	struct Pixel32 {
		union {
			std::uint32_t rgb = 0;
			struct {
				BYTE b;
				BYTE g;
				BYTE r;
				BYTE a;
			};
		};
	};

	using Pixel32Ptr = Pixel32*;

	// 给外部更好用的别名（避免和 Windows 的 RGB(...) 宏冲突，所以不用 RGB 这个名字）
	using Rgb = ::_RGBQUAD;
	using Hsl = ::HSLQUAD;
	using Hsv = ::HSVQUAD;


	namespace detail {
		[[noreturn]] inline void ThrowWin32(const char* msg, const std::source_location& loc = std::source_location::current())
		{
			throw std::runtime_error(
				std::string(loc.file_name()) + ":" + std::to_string(loc.line()) + " " + msg
			);
		}

		template <class Handle, auto DestroyFn>
		class unique_handle {
		public:
			unique_handle() noexcept = default;
			explicit unique_handle(Handle h) noexcept : h_(h) {}
			unique_handle(const unique_handle&) = delete;
			unique_handle& operator=(const unique_handle&) = delete;
			unique_handle(unique_handle&& other) noexcept : h_(std::exchange(other.h_, nullptr)) {}
			unique_handle& operator=(unique_handle&& other) noexcept {
				if (this == &other) return *this;
				reset(std::exchange(other.h_, nullptr));
				return *this;
			}
			~unique_handle() noexcept { reset(); }

			[[nodiscard]] Handle get() const noexcept { return h_; }
			[[nodiscard]] explicit operator bool() const noexcept { return h_ != nullptr; }

			void reset(Handle h = nullptr) noexcept {
				if (h_ != nullptr) {
					DestroyFn(h_);
				}
				h_ = h;
			}

		private:
			Handle h_ = nullptr;
		};

		using unique_hdc = unique_handle<HDC, DeleteDC>;            // CreateCompatibleDC -> DeleteDC
		using unique_hbitmap = unique_handle<HBITMAP, DeleteObject>; // CreateDIBSection/CreateCompatibleBitmap -> DeleteObject

		class window_dc {
		public:
			window_dc() = default;
			explicit window_dc(HWND hwnd) : hwnd_(hwnd), hdc_(GetDC(hwnd)) {}
			window_dc(const window_dc&) = delete;
			window_dc& operator=(const window_dc&) = delete;
			window_dc(window_dc&& other) noexcept
				: hwnd_(std::exchange(other.hwnd_, nullptr)), hdc_(std::exchange(other.hdc_, nullptr)) {}
			window_dc& operator=(window_dc&& other) noexcept {
				if (this == &other) return *this;
				reset();
				hwnd_ = std::exchange(other.hwnd_, nullptr);
				hdc_ = std::exchange(other.hdc_, nullptr);
				return *this;
			}
			~window_dc() noexcept { reset(); }

			[[nodiscard]] HDC get() const noexcept { return hdc_; }
			[[nodiscard]] explicit operator bool() const noexcept { return hdc_ != nullptr; }

			void reset() noexcept {
				if (hdc_) {
					ReleaseDC(hwnd_, hdc_);
				}
				hwnd_ = nullptr;
				hdc_ = nullptr;
			}

		private:
			HWND hwnd_ = nullptr;
			HDC hdc_ = nullptr;
		};

		class select_object_guard {
		public:
			select_object_guard() = default;
			select_object_guard(HDC dc, HGDIOBJ obj) : dc_(dc)
			{
				if (!dc_ || !obj) return;
				old_ = SelectObject(dc_, obj);
			}
			select_object_guard(const select_object_guard&) = delete;
			select_object_guard& operator=(const select_object_guard&) = delete;
			select_object_guard(select_object_guard&& other) noexcept
				: dc_(std::exchange(other.dc_, nullptr)), old_(std::exchange(other.old_, nullptr)) {}
			select_object_guard& operator=(select_object_guard&& other) noexcept {
				if (this == &other) return *this;
				reset();
				dc_ = std::exchange(other.dc_, nullptr);
				old_ = std::exchange(other.old_, nullptr);
				return *this;
			}
			~select_object_guard() noexcept { reset(); }

			void reset() noexcept {
				if (dc_ && old_ && old_ != HGDI_ERROR) {
					SelectObject(dc_, old_);
				}
				dc_ = nullptr;
				old_ = nullptr;
			}

		private:
			HDC dc_ = nullptr;
			HGDIOBJ old_ = nullptr;
		};

		// 让每个进程只启动一次 GDI+（给 PixelCanvas 的“旋转/缩放/裁剪”等子方法用）
		class gdiplus_session {
		public:
			gdiplus_session()
			{
				Gdiplus::GdiplusStartupInput input{};
				const auto st = Gdiplus::GdiplusStartup(&token_, &input, nullptr);
				if (st != Gdiplus::Ok) {
					ThrowWin32("PixelCanvas: GDI+ startup failed.");
				}
			}
			gdiplus_session(const gdiplus_session&) = delete;
			gdiplus_session& operator=(const gdiplus_session&) = delete;

			~gdiplus_session() noexcept
			{
				if (token_ != 0) {
					Gdiplus::GdiplusShutdown(token_);
				}
			}

		private:
			ULONG_PTR token_ = 0;
		};

		inline void EnsureGdiPlusStarted()
		{
			static gdiplus_session s;
			(void)s;
		}
	} // namespace detail


	/**
	 * @brief 像素画板：抓取 DC -> 改像素 -> 贴回 DC
	 *
	 * @details
	 * - 默认构造：使用屏幕 DC
	 * - 传入 HDC：使用你给的 DC（不会接管它的释放）
	 * - 内部缓冲是 32bpp DIB（可直接用 `pixels()` 访问）
	 */
	class PixelCanvas {
	public:
		/**
		 * @brief 变换参数（旋转 / 缩放 / 平移 / 倾斜 / 裁剪）
		 *
		 * @details
		 * - 这些属于 2D 仿射变换（Affine）。
		 * - 你说的“自由 3D 变化”如果指“带透视的 3D 投影”，纯 GDI+ 做不到真正透视；这里提供的是“伪 3D”近似效果。
		 */
		struct TransformParams {
			float scale = 1.0f;            ///< 缩放：1=原大小，>1 放大，<1 缩小
			float rotationDeg = 0.0f;      ///< 旋转角度（度）
			float shearX = 0.0f;           ///< x 方向倾斜（建议：-0.3 ~ 0.3）
			float shearY = 0.0f;           ///< y 方向倾斜
			float offsetX = 0.0f;          ///< 平移（像素）
			float offsetY = 0.0f;          ///< 平移（像素）

			// ===== 透视（真正的投影变换）=====
			bool enablePerspective = false; ///< 是否启用“真透视投影”（启用后不走 GDI+ 仿射矩阵）
			float rotationXDeg = 0.0f;      ///< 绕 X 轴旋转（度）
			float rotationYDeg = 0.0f;      ///< 绕 Y 轴旋转（度）
			float translateZ = 0.0f;        ///< Z 方向平移（像素单位的“深度”）
			float perspective = 0.001f;     ///< 透视强度（越大透视越强）

			// ===== 鱼眼（径向畸变）=====
			bool enableFishEye = false;      ///< 是否启用鱼眼
			float fishEyeStrength = 0.55f;   ///< 鱼眼强度（0~1 左右；越大越夸张）
			float fishEyeRadius = 220.0f;    ///< 鱼眼半径（像素）
			bool fishEyeUseCenter = false;   ///< 是否使用自定义中心
			float fishEyeCenterX = 0.0f;     ///< 鱼眼中心 X（像素；fishEyeUseCenter=true 生效）
			float fishEyeCenterY = 0.0f;     ///< 鱼眼中心 Y（像素；fishEyeUseCenter=true 生效）

			bool enableClip = false;       ///< 是否启用裁剪（在“变换坐标系”里裁剪）
			RECT clipRect{};               ///< 裁剪矩形（enableClip=true 时生效）

			bool enableSrcCrop = false;    ///< 是否先“从源图裁一块”再做变换（更像剪照片）
			RECT srcRect{};                ///< 源裁剪矩形（enableSrcCrop=true 时生效）

			bool fast = false;             ///< true=更快但更糊；false=更清晰但更慢
		};

		/**
		 * @brief 默认：屏幕 DC（GetDC(nullptr)）
		 */
		PixelCanvas()
		{
			InitScreen_();
		}

		/**
		 * @brief 使用外部 DC（不会 ReleaseDC）
		 */
		explicit PixelCanvas(HDC target)
		{
			if (!target) {
				InitScreen_();
				return;
			}
			InitFromDC_(target, 0, 0, false, nullptr);
		}

		/**
		 * @brief 使用外部 DC + 显式尺寸（推荐：更稳定，不依赖裁剪区）
		 */
		PixelCanvas(HDC target, int w, int h)
		{
			if (!target) {
				InitScreen_();
				return;
			}
			if (w <= 0 || h <= 0) detail::ThrowWin32("PixelCanvas: invalid width/height.");
			InitFromDC_(target, w, h, false, nullptr);
		}

		/**
		 * @brief 从窗口创建（内部 GetDC/ReleaseDC）
		 */
		static PixelCanvas FromWindow(HWND hwnd)
		{
			if (!hwnd) detail::ThrowWin32("PixelCanvas::FromWindow: hwnd is null.");
			RECT r{};
			if (!GetClientRect(hwnd, &r)) detail::ThrowWin32("PixelCanvas::FromWindow: GetClientRect failed.");
			const int w = r.right - r.left;
			const int h = r.bottom - r.top;
			if (w <= 0 || h <= 0) detail::ThrowWin32("PixelCanvas::FromWindow: invalid client size.");

			detail::window_dc dc(hwnd);
			if (!dc) detail::ThrowWin32("PixelCanvas::FromWindow: GetDC failed.");

			PixelCanvas out;
			out.Reset_();
			out.InitFromDC_(dc.get(), w, h, true, hwnd);
			return out;
		}

		~PixelCanvas() noexcept
		{
			Reset_();
		}

		PixelCanvas(const PixelCanvas&) = delete;
		PixelCanvas& operator=(const PixelCanvas&) = delete;

		PixelCanvas(PixelCanvas&& other) noexcept
		{
			MoveFrom_(other);
		}
		PixelCanvas& operator=(PixelCanvas&& other) noexcept
		{
			if (this == &other) return *this;
			Reset_();
			MoveFrom_(other);
			return *this;
		}

		[[nodiscard]] int width() const noexcept { return w_; }
		[[nodiscard]] int height() const noexcept { return h_; }
		[[nodiscard]] Pixel32Ptr pixels() noexcept { return pixels_; }
		[[nodiscard]] const Pixel32* pixels() const noexcept { return pixels_; }

		// ====================== 颜色模型（RGB/HSL/HSV）======================
		// PixelCanvas 内部像素是 BGRA（Pixel32）。下面这些方法让你能直接用 color.hpp 里的模型读写像素。

		[[nodiscard]] static Rgb ToRgb(Pixel32 p) noexcept
		{
			Rgb out{};
			out.r = p.r;
			out.g = p.g;
			out.b = p.b;
			out.unused = 0;
			return out;
		}

		[[nodiscard]] static Pixel32 FromRgb(Rgb c, BYTE a = 255) noexcept
		{
			Pixel32 out{};
			out.r = c.r;
			out.g = c.g;
			out.b = c.b;
			out.a = a;
			return out;
		}

		[[nodiscard]] static Hsl ToHsl(Pixel32 p) noexcept
		{
			return RGBToHSL(ToRgb(p));
		}

		[[nodiscard]] static Pixel32 FromHsl(Hsl hsl, BYTE a = 255) noexcept
		{
			return FromRgb(HSLToRGB(hsl), a);
		}

		[[nodiscard]] static Hsv ToHsv(Pixel32 p) noexcept
		{
			return RGBToHSV(ToRgb(p));
		}

		[[nodiscard]] static Pixel32 FromHsv(Hsv hsv, BYTE a = 255) noexcept
		{
			return FromRgb(HSVToRGB(hsv), a);
		}

		[[nodiscard]] bool InBounds(int x, int y) const noexcept
		{
			return x >= 0 && y >= 0 && x < w_ && y < h_;
		}

		// 读取/写入：RGB
		[[nodiscard]] Rgb GetRgb(int x, int y) const noexcept
		{
			if (!pixels_ || !InBounds(x, y)) return {};
			return ToRgb(pixels_[y * w_ + x]);
		}

		void SetRgb(int x, int y, Rgb c, BYTE a = 255) noexcept
		{
			if (!pixels_ || !InBounds(x, y)) return;
			pixels_[y * w_ + x] = FromRgb(c, a);
		}

		// 读取/写入：HSL
		[[nodiscard]] Hsl GetHsl(int x, int y) const noexcept
		{
			if (!pixels_ || !InBounds(x, y)) return {};
			return ToHsl(pixels_[y * w_ + x]);
		}

		void SetHsl(int x, int y, Hsl hsl, BYTE a = 255) noexcept
		{
			if (!pixels_ || !InBounds(x, y)) return;
			pixels_[y * w_ + x] = FromHsl(hsl, a);
		}

		// 读取/写入：HSV
		[[nodiscard]] Hsv GetHsv(int x, int y) const noexcept
		{
			if (!pixels_ || !InBounds(x, y)) return {};
			return ToHsv(pixels_[y * w_ + x]);
		}

		void SetHsv(int x, int y, Hsv hsv, BYTE a = 255) noexcept
		{
			if (!pixels_ || !InBounds(x, y)) return;
			pixels_[y * w_ + x] = FromHsv(hsv, a);
		}
		[[nodiscard]] HBITMAP hbitmap() const noexcept { return dibOwner_.get(); } ///< 方便给 GDI+ 等接口直接用

		/**
		 * @brief 把当前画布以“变形后的样子”绘制到目标 DC（旋转/缩放/平移/倾斜/裁剪）
		 *
		 * @details
		 * 常用流程：先 `Capture()` 抓一帧到内部位图，再用这些方法画到窗口/屏幕。
		 */
		void PresentTransformed(const TransformParams& p, HDC hdcTarget = nullptr, int dstWidth = 0, int dstHeight = 0) const
		{
			// 如果启用了鱼眼：需要先渲染到缓冲，再做一次径向畸变，再贴到目标 DC
			if (p.enableFishEye) {
				PresentWithFishEye_(p, hdcTarget, dstWidth, dstHeight);
				return;
			}

			if (p.enablePerspective) {
				PresentPerspective_(p, hdcTarget, dstWidth, dstHeight);
				return;
			}

			detail::EnsureGdiPlusStarted();

			HDC dc = hdcTarget ? hdcTarget : targetDC_;
			if (!dc) return;

			const int w = (dstWidth > 0) ? dstWidth : w_;
			const int h = (dstHeight > 0) ? dstHeight : h_;
			if (w <= 0 || h <= 0) return;

			HBITMAP hbmp = hbitmap();
			if (!hbmp) return;

			Gdiplus::Graphics g(dc);
			if (p.fast) {
				g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
				g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
				g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
				g.SetCompositingQuality(Gdiplus::CompositingQualityHighSpeed);
			}
			else {
				g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
				g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
				g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
				g.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
			}

			Gdiplus::Bitmap bmp(hbmp, nullptr);
			if (bmp.GetLastStatus() != Gdiplus::Ok) return;

			const int bmpW = static_cast<int>(bmp.GetWidth());
			const int bmpH = static_cast<int>(bmp.GetHeight());
			if (bmpW <= 0 || bmpH <= 0) return;

			Gdiplus::Rect srcRect(0, 0, bmpW, bmpH);
			if (p.enableSrcCrop) {
				const int x0 = std::clamp<int>(static_cast<int>(p.srcRect.left), 0, bmpW);
				const int y0 = std::clamp<int>(static_cast<int>(p.srcRect.top), 0, bmpH);
				const int x1 = std::clamp<int>(static_cast<int>(p.srcRect.right), 0, bmpW);
				const int y1 = std::clamp<int>(static_cast<int>(p.srcRect.bottom), 0, bmpH);
				const int sw = (x1 > x0) ? (x1 - x0) : 0;
				const int sh = (y1 > y0) ? (y1 - y0) : 0;
				if (sw > 0 && sh > 0) {
					srcRect = Gdiplus::Rect(x0, y0, sw, sh);
				}
			}

			const float centerX = pivotEnabled_ ? pivotX_ : (w * 0.5f);
			const float centerY = pivotEnabled_ ? pivotY_ : (h * 0.5f);

			Gdiplus::Matrix m;
			m.Translate(centerX, centerY);     // 移到中心
			m.Scale(p.scale, p.scale);         // 缩放
			m.Rotate(p.rotationDeg);           // 旋转
			m.Shear(p.shearX, p.shearY);       // 倾斜
			m.Translate(-centerX, -centerY);   // 移回
			m.Translate(p.offsetX, p.offsetY); // 平移

			const auto state = g.Save();
			g.SetTransform(&m);

			if (p.enableClip) {
				const int cw = static_cast<int>(p.clipRect.right - p.clipRect.left);
				const int ch = static_cast<int>(p.clipRect.bottom - p.clipRect.top);
				if (cw > 0 && ch > 0) {
					g.SetClip(Gdiplus::Rect(
						static_cast<int>(p.clipRect.left),
						static_cast<int>(p.clipRect.top),
						cw,
						ch
					));
				}
			}

			if (p.enableSrcCrop) {
				g.DrawImage(&bmp,
					Gdiplus::Rect(0, 0, w, h),
					srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height,
					Gdiplus::UnitPixel
				);
			}
			else {
				g.DrawImage(&bmp, 0, 0, w, h);
			}

			g.Restore(state);
		}

		/**
		 * @brief 状态机制：用内部“当前变换状态”进行绘制
		 *
		 * @details
		 * 你可以先不停调用 `Move/Rotate/Scale/...` 去改状态，然后每帧只调用一次 `PresentTransformed()`。
		 */
		void PresentTransformed(HDC hdcTarget = nullptr, int dstWidth = 0, int dstHeight = 0) const
		{
			if (!stateEnabled_) {
				PresentTo_(hdcTarget, 0, 0);
				return;
			}
			PresentTransformed(transform_, hdcTarget, dstWidth, dstHeight);
		}

		// ====================== 状态机制（像 Python 一样：先改状态，再 Present） ======================
		[[nodiscard]] const TransformParams& transform() const noexcept { return transform_; }
		[[nodiscard]] bool StateEnabled() const noexcept { return stateEnabled_; }

		PixelCanvas& EnableState(bool on = true) noexcept
		{
			stateEnabled_ = on;
			return *this;
		}

		PixelCanvas& ResetTransform() noexcept
		{
			transform_ = TransformParams{};
			pivotEnabled_ = false;
			pivotX_ = 0.0f;
			pivotY_ = 0.0f;
			return *this;
		}

		PixelCanvas& SetTransform(const TransformParams& p) noexcept
		{
			transform_ = p;
			return *this;
		}

		PixelCanvas& Fast(bool on = true) noexcept
		{
			transform_.fast = on;
			return *this;
		}

		PixelCanvas& EnablePerspective(bool on = true) noexcept
		{
			transform_.enablePerspective = on;
			return *this;
		}

		PixelCanvas& SetPerspective(float strength) noexcept
		{
			transform_.perspective = strength;
			return *this;
		}

		PixelCanvas& EnableFishEye(bool on = true) noexcept
		{
			transform_.enableFishEye = on;
			return *this;
		}

		PixelCanvas& SetFishEye(float strength = 0.55f, float radius = 220.0f) noexcept
		{
			transform_.enableFishEye = true;
			transform_.fishEyeStrength = strength;
			transform_.fishEyeRadius = radius;
			transform_.fishEyeUseCenter = false;
			return *this;
		}

		PixelCanvas& SetFishEyeCenter(float cx, float cy) noexcept
		{
			transform_.fishEyeUseCenter = true;
			transform_.fishEyeCenterX = cx;
			transform_.fishEyeCenterY = cy;
			return *this;
		}

		PixelCanvas& FishEye(float cx, float cy, float strength = 0.55f, float radius = 220.0f) noexcept
		{
			transform_.enableFishEye = true;
			transform_.fishEyeStrength = strength;
			transform_.fishEyeRadius = radius;
			transform_.fishEyeUseCenter = true;
			transform_.fishEyeCenterX = cx;
			transform_.fishEyeCenterY = cy;
			return *this;
		}

		PixelCanvas& DisableFishEye() noexcept
		{
			transform_.enableFishEye = false;
			return *this;
		}

		PixelCanvas& Move(float dx, float dy) noexcept
		{
			transform_.offsetX += dx;
			transform_.offsetY += dy;
			return *this;
		}

		/**
		 * @brief 设置旋转/缩放中心（单位：像素，目标区域坐标）
		 * @details
		 * - 默认中心：目标区域中心（w/2, h/2）
		 * - 你可以把中心设到任意点：例如鼠标位置、某个角、某个按钮上
		 *
		 * 常见用法：
		 *   canvas.Pivot(0, 0).Rotate(5);            // 围绕左上角转
		 *   canvas.Pivot(w/2, h/2).Rotate(5);        // 围绕中心转（默认）
		 */
		PixelCanvas& Pivot(float x, float y) noexcept
		{
			pivotEnabled_ = true;
			pivotX_ = x;
			pivotY_ = y;
			return *this;
		}

		/**
		 * @brief 恢复默认中心（目标区域中心）
		 */
		PixelCanvas& PivotCenter() noexcept
		{
			pivotEnabled_ = false;
			return *this;
		}

		/**
		 * @brief 方便写法：指定中心旋转
		 */
		PixelCanvas& RotateAt(float deg, float x, float y) noexcept
		{
			Pivot(x, y);
			return Rotate(deg);
		}

		PixelCanvas& Rotate(float deg) noexcept
		{
			transform_.rotationDeg += deg;
			return *this;
		}

		PixelCanvas& RotateX(float deg) noexcept
		{
			transform_.rotationXDeg += deg;
			return *this;
		}

		PixelCanvas& RotateY(float deg) noexcept
		{
			transform_.rotationYDeg += deg;
			return *this;
		}

		PixelCanvas& TranslateZ(float dz) noexcept
		{
			transform_.translateZ += dz;
			return *this;
		}

		PixelCanvas& Scale(float s) noexcept
		{
			// 叠乘更符合“连续缩放”的直觉
			transform_.scale *= s;
			return *this;
		}

		PixelCanvas& Shear(float sx, float sy) noexcept
		{
			transform_.shearX += sx;
			transform_.shearY += sy;
			return *this;
		}

		PixelCanvas& Clip(const RECT& clip) noexcept
		{
			transform_.enableClip = true;
			transform_.clipRect = clip;
			return *this;
		}

		PixelCanvas& DisableClip() noexcept
		{
			transform_.enableClip = false;
			return *this;
		}

		PixelCanvas& Crop(const RECT& src) noexcept
		{
			transform_.enableSrcCrop = true;
			transform_.srcRect = src;
			return *this;
		}

		PixelCanvas& DisableCrop() noexcept
		{
			transform_.enableSrcCrop = false;
			return *this;
		}

		/**
		 * @brief “伪 3D”旋转（近似）：把状态设置成“像 3D 的样子”（不是真正透视）
		 *
		 * @details
		 * 这是为了“给小朋友玩起来像 3D”。它会覆盖 shear/scale（不会影响你已有的 offset/rotate）。
		 */
		PixelCanvas& Rotate3D(float rotXDeg, float rotYDeg, float strength = 0.45f) noexcept
		{
			const float rx = rotXDeg * (3.1415926f / 180.0f);
			const float ry = rotYDeg * (3.1415926f / 180.0f);

			transform_.shearX = std::tanf(ry) * strength * 0.25f;
			transform_.shearY = std::tanf(rx) * strength * 0.25f;

			const float sx = std::max(0.2f, std::cosf(ry));
			const float sy = std::max(0.2f, std::cosf(rx));
			transform_.scale = (sx + sy) * 0.5f;
			return *this;
		}

		/**
		 * @brief 动态扭曲：直接把状态设置成“随时间变化”的那一套
		 */
		PixelCanvas& DynamicDistortion(float timeSec) noexcept
		{
			transform_ = TransformParams{};
			pivotEnabled_ = false;
			transform_.scale = 1.0f + std::sinf(timeSec * 2.0f) * 0.20f;
			transform_.rotationDeg = timeSec * 30.0f;
			transform_.shearX = std::sinf(timeSec * 1.5f) * 0.10f;
			transform_.shearY = std::cosf(timeSec * 1.3f) * 0.08f;
			transform_.offsetX = std::sinf(timeSec * 3.0f) * 5.0f;
			transform_.offsetY = std::cosf(timeSec * 2.7f) * 5.0f;
			return *this;
		}

		/**
		 * @brief 从目标 DC 抓取一帧到缓冲
		 */
		void Capture(int srcX = 0, int srcY = 0) const
		{
			if (!memDC_ || !targetDC_) return;
			BitBlt(memDC_, 0, 0, w_, h_, targetDC_, srcX, srcY, SRCCOPY);
		}

		/**
		 * @brief 把缓冲贴回目标 DC
		 */
		void Present(int dstX = 0, int dstY = 0) const
		{
			if (!memDC_ || !targetDC_) return;
			BitBlt(targetDC_, dstX, dstY, w_, h_, memDC_, 0, 0, SRCCOPY);
		}

	private:
		void PresentTo_(HDC hdcTarget, int dstX, int dstY) const
		{
			HDC dc = hdcTarget ? hdcTarget : targetDC_;
			if (!dc || !memDC_) return;
			BitBlt(dc, dstX, dstY, w_, h_, memDC_, 0, 0, SRCCOPY);
		}

		struct Mat3 {
			float m[3][3]{};
		};

		static bool Invert3x3_(const Mat3& a, Mat3& out) noexcept
		{
			const float a00 = a.m[0][0], a01 = a.m[0][1], a02 = a.m[0][2];
			const float a10 = a.m[1][0], a11 = a.m[1][1], a12 = a.m[1][2];
			const float a20 = a.m[2][0], a21 = a.m[2][1], a22 = a.m[2][2];

			const float b00 = a11 * a22 - a12 * a21;
			const float b01 = a02 * a21 - a01 * a22;
			const float b02 = a01 * a12 - a02 * a11;
			const float b10 = a12 * a20 - a10 * a22;
			const float b11 = a00 * a22 - a02 * a20;
			const float b12 = a02 * a10 - a00 * a12;
			const float b20 = a10 * a21 - a11 * a20;
			const float b21 = a01 * a20 - a00 * a21;
			const float b22 = a00 * a11 - a01 * a10;

			const float det = a00 * b00 + a01 * b10 + a02 * b20;
			if (std::fabs(det) < 1e-8f) return false;
			const float invDet = 1.0f / det;

			out.m[0][0] = b00 * invDet;
			out.m[0][1] = b01 * invDet;
			out.m[0][2] = b02 * invDet;
			out.m[1][0] = b10 * invDet;
			out.m[1][1] = b11 * invDet;
			out.m[1][2] = b12 * invDet;
			out.m[2][0] = b20 * invDet;
			out.m[2][1] = b21 * invDet;
			out.m[2][2] = b22 * invDet;
			return true;
		}

		static Mat3 UnitSquareToQuad_(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3) noexcept
		{
			// p00=(x0,y0), p10=(x1,y1), p11=(x2,y2), p01=(x3,y3)
			const float dx1 = x1 - x2;
			const float dx2 = x3 - x2;
			const float dx3 = x0 - x1 + x2 - x3;
			const float dy1 = y1 - y2;
			const float dy2 = y3 - y2;
			const float dy3 = y0 - y1 + y2 - y3;

			Mat3 m{};
			if (std::fabs(dx3) < 1e-6f && std::fabs(dy3) < 1e-6f) {
				// affine
				m.m[0][0] = x1 - x0; m.m[0][1] = x3 - x0; m.m[0][2] = x0;
				m.m[1][0] = y1 - y0; m.m[1][1] = y3 - y0; m.m[1][2] = y0;
				m.m[2][0] = 0.0f;    m.m[2][1] = 0.0f;    m.m[2][2] = 1.0f;
				return m;
			}

			const float denom = dx1 * dy2 - dx2 * dy1;
			if (std::fabs(denom) < 1e-8f) {
				// fallback affine
				m.m[0][0] = x1 - x0; m.m[0][1] = x3 - x0; m.m[0][2] = x0;
				m.m[1][0] = y1 - y0; m.m[1][1] = y3 - y0; m.m[1][2] = y0;
				m.m[2][0] = 0.0f;    m.m[2][1] = 0.0f;    m.m[2][2] = 1.0f;
				return m;
			}

			const float g = (dx3 * dy2 - dx2 * dy3) / denom;
			const float h = (dx1 * dy3 - dx3 * dy1) / denom;

			m.m[0][0] = x1 - x0 + g * x1;
			m.m[0][1] = x3 - x0 + h * x3;
			m.m[0][2] = x0;
			m.m[1][0] = y1 - y0 + g * y1;
			m.m[1][1] = y3 - y0 + h * y3;
			m.m[1][2] = y0;
			m.m[2][0] = g;
			m.m[2][1] = h;
			m.m[2][2] = 1.0f;
			return m;
		}

		static Pixel32 SampleNearest_(const Pixel32* src, int srcW, int srcH, int sx, int sy) noexcept
		{
			if (!src) return Pixel32{};
			sx = std::clamp(sx, 0, srcW - 1);
			sy = std::clamp(sy, 0, srcH - 1);
			return src[sy * srcW + sx];
		}

		static Pixel32 SampleBilinear_(const Pixel32* src, int srcW, int srcH, float fx, float fy) noexcept
		{
			if (!src) return Pixel32{};
			if (srcW <= 0 || srcH <= 0) return Pixel32{};

			fx = std::clamp(fx, 0.0f, static_cast<float>(srcW - 1));
			fy = std::clamp(fy, 0.0f, static_cast<float>(srcH - 1));

			const int x0 = static_cast<int>(fx);
			const int y0 = static_cast<int>(fy);
			const int x1 = std::min(x0 + 1, srcW - 1);
			const int y1 = std::min(y0 + 1, srcH - 1);

			const float tx = fx - static_cast<float>(x0);
			const float ty = fy - static_cast<float>(y0);

			const Pixel32 p00 = src[y0 * srcW + x0];
			const Pixel32 p10 = src[y0 * srcW + x1];
			const Pixel32 p01 = src[y1 * srcW + x0];
			const Pixel32 p11 = src[y1 * srcW + x1];

			auto Lerp = [](float a, float b, float t) { return a + (b - a) * t; };
			Pixel32 out{};
			out.b = static_cast<BYTE>(Lerp(Lerp(p00.b, p10.b, tx), Lerp(p01.b, p11.b, tx), ty));
			out.g = static_cast<BYTE>(Lerp(Lerp(p00.g, p10.g, tx), Lerp(p01.g, p11.g, tx), ty));
			out.r = static_cast<BYTE>(Lerp(Lerp(p00.r, p10.r, tx), Lerp(p01.r, p11.r, tx), ty));
			out.a = static_cast<BYTE>(Lerp(Lerp(p00.a, p10.a, tx), Lerp(p01.a, p11.a, tx), ty));
			return out;
		}

		static void ApplyFishEye_(const Pixel32* src, Pixel32* dst, int w, int h, float centerX, float centerY, float radius, float strength, bool fast) noexcept
		{
			if (!src || !dst || w <= 0 || h <= 0) return;

			if (radius <= 1.0f || strength <= 0.0f) {
				std::memcpy(dst, src, static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * sizeof(Pixel32));
				return;
			}

			strength = std::clamp(strength, 0.0f, 1.5f);
			const float r2 = radius * radius;

			for (int y = 0; y < h; ++y) {
				const float dy = static_cast<float>(y) - centerY;
				for (int x = 0; x < w; ++x) {
					const float dx = static_cast<float>(x) - centerX;
					const float dist2 = dx * dx + dy * dy;
					if (dist2 >= r2) {
						dst[y * w + x] = src[y * w + x];
						continue;
					}

					const float dist = std::sqrt(dist2);
					const float t = (radius > 0.0f) ? (dist / radius) : 1.0f; // 0..1

					// 反向映射：dst -> src（中心区域更“放大”）
					const float shrink = 1.0f - strength * (1.0f - t * t); // t=0 -> 1-strength, t=1 -> 1
					const float inv = (dist > 1e-4f) ? (shrink / dist) : 0.0f;

					const float sx = centerX + dx * inv * dist; // dx * shrink
					const float sy = centerY + dy * inv * dist; // dy * shrink

					Pixel32 c{};
					if (fast) {
						c = SampleNearest_(src, w, h, static_cast<int>(sx + 0.5f), static_cast<int>(sy + 0.5f));
					}
					else {
						c = SampleBilinear_(src, w, h, sx, sy);
					}
					dst[y * w + x] = c;
				}
			}
		}

		void EnsureFxBuffers_(HDC dc, int w, int h) const
		{
			if (w <= 0 || h <= 0) return;
			if (fxW_ == w && fxH_ == h && fxDcA_ && fxDcB_ && fxBitsA_ && fxBitsB_) return;

			fxSelA_.reset();
			fxSelB_.reset();
			fxBmpOwnerA_.reset();
			fxBmpOwnerB_.reset();
			fxDcOwnerA_.reset();
			fxDcOwnerB_.reset();
			fxDcA_ = nullptr;
			fxDcB_ = nullptr;
			fxBitsA_ = nullptr;
			fxBitsB_ = nullptr;
			fxW_ = 0;
			fxH_ = 0;

			fxDcOwnerA_.reset(CreateCompatibleDC(dc));
			fxDcOwnerB_.reset(CreateCompatibleDC(dc));
			fxDcA_ = fxDcOwnerA_.get();
			fxDcB_ = fxDcOwnerB_.get();
			if (!fxDcA_ || !fxDcB_) return;

			BITMAPINFO bmi{};
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = w;
			bmi.bmiHeader.biHeight = -h;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;

			void* bitsA = nullptr;
			void* bitsB = nullptr;
			fxBmpOwnerA_.reset(CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bitsA, nullptr, 0));
			fxBmpOwnerB_.reset(CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bitsB, nullptr, 0));
			if (!fxBmpOwnerA_ || !fxBmpOwnerB_ || !bitsA || !bitsB) return;

			fxSelA_ = detail::select_object_guard(fxDcA_, fxBmpOwnerA_.get());
			fxSelB_ = detail::select_object_guard(fxDcB_, fxBmpOwnerB_.get());
			fxBitsA_ = static_cast<Pixel32*>(bitsA);
			fxBitsB_ = static_cast<Pixel32*>(bitsB);
			fxW_ = w;
			fxH_ = h;
		}

		void RenderAffineToFxA_(const TransformParams& p, HDC dc, int w, int h) const
		{
			detail::EnsureGdiPlusStarted();
			EnsureFxBuffers_(dc, w, h);
			if (!fxDcA_ || !fxBitsA_) return;

			// 清空
			const std::uint32_t bg = 0xFF000000u;
			for (int i = 0; i < w * h; ++i) fxBitsA_[i].rgb = bg;

			Gdiplus::Graphics g(fxDcA_);
			if (p.fast) {
				g.SetSmoothingMode(Gdiplus::SmoothingModeNone);
				g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
				g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
				g.SetCompositingQuality(Gdiplus::CompositingQualityHighSpeed);
			}
			else {
				g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
				g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
				g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
				g.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
			}

			HBITMAP hbmp = hbitmap();
			if (!hbmp) return;

			Gdiplus::Bitmap bmp(hbmp, nullptr);
			if (bmp.GetLastStatus() != Gdiplus::Ok) return;

			const int bmpW = static_cast<int>(bmp.GetWidth());
			const int bmpH = static_cast<int>(bmp.GetHeight());
			if (bmpW <= 0 || bmpH <= 0) return;

			Gdiplus::Rect srcRect(0, 0, bmpW, bmpH);
			if (p.enableSrcCrop) {
				const int x0 = std::clamp<int>(static_cast<int>(p.srcRect.left), 0, bmpW);
				const int y0 = std::clamp<int>(static_cast<int>(p.srcRect.top), 0, bmpH);
				const int x1 = std::clamp<int>(static_cast<int>(p.srcRect.right), 0, bmpW);
				const int y1 = std::clamp<int>(static_cast<int>(p.srcRect.bottom), 0, bmpH);
				const int sw = (x1 > x0) ? (x1 - x0) : 0;
				const int sh = (y1 > y0) ? (y1 - y0) : 0;
				if (sw > 0 && sh > 0) srcRect = Gdiplus::Rect(x0, y0, sw, sh);
			}

			const float centerX = pivotEnabled_ ? pivotX_ : (w * 0.5f);
			const float centerY = pivotEnabled_ ? pivotY_ : (h * 0.5f);

			Gdiplus::Matrix m;
			m.Translate(centerX, centerY);
			m.Scale(p.scale, p.scale);
			m.Rotate(p.rotationDeg);
			m.Shear(p.shearX, p.shearY);
			m.Translate(-centerX, -centerY);
			m.Translate(p.offsetX, p.offsetY);

			const auto state = g.Save();
			g.SetTransform(&m);

			if (p.enableClip) {
				const int cw = static_cast<int>(p.clipRect.right - p.clipRect.left);
				const int ch = static_cast<int>(p.clipRect.bottom - p.clipRect.top);
				if (cw > 0 && ch > 0) {
					g.SetClip(Gdiplus::Rect(static_cast<int>(p.clipRect.left), static_cast<int>(p.clipRect.top), cw, ch));
				}
			}

			if (p.enableSrcCrop) {
				g.DrawImage(&bmp, Gdiplus::Rect(0, 0, w, h), srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, Gdiplus::UnitPixel);
			}
			else {
				g.DrawImage(&bmp, 0, 0, w, h);
			}

			g.Restore(state);
		}

		void RenderPerspectiveToFxA_(const TransformParams& p, HDC dc, int w, int h) const
		{
			EnsureFxBuffers_(dc, w, h);
			if (!fxDcA_ || !fxBitsA_) return;

			// 直接复用已有算法：写入 fxBitsA_
			PresentPerspectiveToBits_(p, w, h, fxBitsA_);
		}

		void PresentPerspectiveToBits_(const TransformParams& p, int outW, int outH, Pixel32* dst) const
		{
			if (!dst || !pixels_) return;

			// 源裁剪区域（纹理）
			int srcLeft = 0, srcTop = 0, srcW = w_, srcH = h_;
			if (p.enableSrcCrop) {
				const int l = std::clamp<int>(static_cast<int>(p.srcRect.left), 0, w_);
				const int t = std::clamp<int>(static_cast<int>(p.srcRect.top), 0, h_);
				const int r = std::clamp<int>(static_cast<int>(p.srcRect.right), 0, w_);
				const int b = std::clamp<int>(static_cast<int>(p.srcRect.bottom), 0, h_);
				const int cw = std::max(0, r - l);
				const int ch = std::max(0, b - t);
				if (cw > 0 && ch > 0) {
					srcLeft = l; srcTop = t; srcW = cw; srcH = ch;
				}
			}
			const Pixel32* srcBase = pixels_ + srcTop * w_ + srcLeft;

			// 背景
			const Pixel32 bg{ .rgb = 0xFF000000u };
			for (int i = 0; i < outW * outH; ++i) dst[i] = bg;

			// 计算 3D -> 2D 的四个角（对应纹理四角）
			const float radX = p.rotationXDeg * (3.1415926f / 180.0f);
			const float radY = p.rotationYDeg * (3.1415926f / 180.0f);
			const float radZ = p.rotationDeg * (3.1415926f / 180.0f);

			const float cosX = std::cos(radX), sinX = std::sin(radX);
			const float cosY = std::cos(radY), sinY = std::sin(radY);
			const float cosZ = std::cos(radZ), sinZ = std::sin(radZ);

			auto Apply3D = [&](float x, float y, float z, float& outX, float& outY) {
				// scale
				x *= p.scale;
				y *= p.scale;
				z *= p.scale;

				// Z
				{
					const float tx = x * cosZ - y * sinZ;
					const float ty = x * sinZ + y * cosZ;
					x = tx; y = ty;
				}
				// Y
				{
					const float tx = x * cosY + z * sinY;
					const float tz = -x * sinY + z * cosY;
					x = tx; z = tz;
				}
				// X
				{
					const float ty = y * cosX - z * sinX;
					const float tz = y * sinX + z * cosX;
					y = ty; z = tz;
				}

				z += p.translateZ;

				const float denom = 1.0f + z * p.perspective;
				const float factor = (std::fabs(denom) < 1e-5f) ? 1e5f : (1.0f / denom);
				const float baseX = pivotEnabled_ ? pivotX_ : (outW * 0.5f);
				const float baseY = pivotEnabled_ ? pivotY_ : (outH * 0.5f);
				outX = x * factor + baseX + p.offsetX;
				outY = y * factor + baseY + p.offsetY;
			};

			const float hw = static_cast<float>(srcW) * 0.5f;
			const float hh = static_cast<float>(srcH) * 0.5f;

			float qx0, qy0, qx1, qy1, qx2, qy2, qx3, qy3;
			Apply3D(-hw, -hh, 0.0f, qx0, qy0); // TL
			Apply3D(+hw, -hh, 0.0f, qx1, qy1); // TR
			Apply3D(+hw, +hh, 0.0f, qx2, qy2); // BR
			Apply3D(-hw, +hh, 0.0f, qx3, qy3); // BL

			const Mat3 M = UnitSquareToQuad_(qx0, qy0, qx1, qy1, qx2, qy2, qx3, qy3);
			Mat3 invM{};
			if (!Invert3x3_(M, invM)) return;

			const int minX = std::clamp(static_cast<int>(std::floor(std::min({ qx0, qx1, qx2, qx3 }))), 0, outW - 1);
			const int maxX = std::clamp(static_cast<int>(std::ceil(std::max({ qx0, qx1, qx2, qx3 }))), 0, outW - 1);
			const int minY = std::clamp(static_cast<int>(std::floor(std::min({ qy0, qy1, qy2, qy3 }))), 0, outH - 1);
			const int maxY = std::clamp(static_cast<int>(std::ceil(std::max({ qy0, qy1, qy2, qy3 }))), 0, outH - 1);

			for (int y = minY; y <= maxY; ++y) {
				for (int x = minX; x <= maxX; ++x) {
					if (p.enableClip) {
						if (x < p.clipRect.left || x >= p.clipRect.right || y < p.clipRect.top || y >= p.clipRect.bottom) continue;
					}

					const float fx = static_cast<float>(x);
					const float fy = static_cast<float>(y);
					const float u = invM.m[0][0] * fx + invM.m[0][1] * fy + invM.m[0][2];
					const float v = invM.m[1][0] * fx + invM.m[1][1] * fy + invM.m[1][2];
					const float ww = invM.m[2][0] * fx + invM.m[2][1] * fy + invM.m[2][2];
					if (std::fabs(ww) < 1e-8f) continue;

					const float uu = u / ww;
					const float vv = v / ww;
					if (uu < 0.0f || uu > 1.0f || vv < 0.0f || vv > 1.0f) continue;

					const float srcXf = uu * static_cast<float>(srcW - 1);
					const float srcYf = vv * static_cast<float>(srcH - 1);

					Pixel32 color{};
					if (p.fast) {
						color = SampleNearest_(srcBase, srcW, srcH, static_cast<int>(srcXf + 0.5f), static_cast<int>(srcYf + 0.5f));
					}
					else {
						color = SampleBilinear_(srcBase, srcW, srcH, srcXf, srcYf);
					}

					dst[y * outW + x] = color;
				}
			}
		}

		void PresentWithFishEye_(const TransformParams& p, HDC hdcTarget, int dstWidth, int dstHeight) const
		{
			HDC dc = hdcTarget ? hdcTarget : targetDC_;
			if (!dc) return;

			const int outW = (dstWidth > 0) ? dstWidth : w_;
			const int outH = (dstHeight > 0) ? dstHeight : h_;
			if (outW <= 0 || outH <= 0) return;

			EnsureFxBuffers_(dc, outW, outH);
			if (!fxDcA_ || !fxDcB_ || !fxBitsA_ || !fxBitsB_) return;

			// 1) 先把“基础变换”渲染到 A（GDI+ 仿射 or 真透视）
			if (p.enablePerspective) {
				RenderPerspectiveToFxA_(p, dc, outW, outH);
			}
			else {
				RenderAffineToFxA_(p, dc, outW, outH);
			}

			// 2) 再把 A 做鱼眼，输出到 B
			const float cx = p.fishEyeUseCenter ? p.fishEyeCenterX : (outW * 0.5f);
			const float cy = p.fishEyeUseCenter ? p.fishEyeCenterY : (outH * 0.5f);
			ApplyFishEye_(fxBitsA_, fxBitsB_, outW, outH, cx, cy, p.fishEyeRadius, p.fishEyeStrength, p.fast);

			// 3) 贴到目标
			BitBlt(dc, 0, 0, outW, outH, fxDcB_, 0, 0, SRCCOPY);
		}

		void PresentPerspective_(const TransformParams& p, HDC hdcTarget, int dstWidth, int dstHeight) const
		{
			HDC dc = hdcTarget ? hdcTarget : targetDC_;
			if (!dc || !pixels_) return;

			const int outW = (dstWidth > 0) ? dstWidth : w_;
			const int outH = (dstHeight > 0) ? dstHeight : h_;
			if (outW <= 0 || outH <= 0) return;

			EnsureFxBuffers_(dc, outW, outH);
			if (!fxDcA_ || !fxBitsA_) return;
			PresentPerspectiveToBits_(p, outW, outH, fxBitsA_);
			BitBlt(dc, 0, 0, outW, outH, fxDcA_, 0, 0, SRCCOPY);
		}

		// 目标 DC（可能是外部传入，也可能是我们 GetDC 得到的）
		HDC targetDC_ = nullptr;
		HWND releaseWnd_ = nullptr; // ReleaseDC 需要 HWND
		bool ownsTargetDC_ = false;

		// 内部缓冲 DC + DIB
		detail::unique_hdc memDcOwner_;
		detail::unique_hbitmap dibOwner_;
		detail::select_object_guard dibSel_;
		HDC memDC_ = nullptr;       // view
		Pixel32Ptr pixels_ = nullptr;
		int w_ = 0;
		int h_ = 0;
		TransformParams transform_{};
		bool stateEnabled_ = true;
		bool pivotEnabled_ = false;
		float pivotX_ = 0.0f;
		float pivotY_ = 0.0f;

		// 变换/滤镜用的复用缓冲（避免每帧频繁 CreateDIBSection）
		mutable detail::unique_hdc fxDcOwnerA_;
		mutable detail::unique_hdc fxDcOwnerB_;
		mutable detail::unique_hbitmap fxBmpOwnerA_;
		mutable detail::unique_hbitmap fxBmpOwnerB_;
		mutable detail::select_object_guard fxSelA_;
		mutable detail::select_object_guard fxSelB_;
		mutable HDC fxDcA_ = nullptr;
		mutable HDC fxDcB_ = nullptr;
		mutable Pixel32* fxBitsA_ = nullptr;
		mutable Pixel32* fxBitsB_ = nullptr;
		mutable int fxW_ = 0;
		mutable int fxH_ = 0;

		void Reset_() noexcept
		{
			dibSel_.reset();
			dibOwner_.reset();
			memDcOwner_.reset();

			if (ownsTargetDC_ && targetDC_) {
				ReleaseDC(releaseWnd_, targetDC_);
			}

			targetDC_ = nullptr;
			releaseWnd_ = nullptr;
			ownsTargetDC_ = false;
			memDC_ = nullptr;
			pixels_ = nullptr;
			w_ = 0;
			h_ = 0;
		}

		void MoveFrom_(PixelCanvas& other) noexcept
		{
			targetDC_ = std::exchange(other.targetDC_, nullptr);
			releaseWnd_ = std::exchange(other.releaseWnd_, nullptr);
			ownsTargetDC_ = std::exchange(other.ownsTargetDC_, false);

			memDcOwner_ = std::move(other.memDcOwner_);
			dibOwner_ = std::move(other.dibOwner_);
			dibSel_ = std::move(other.dibSel_);
			memDC_ = std::exchange(other.memDC_, nullptr);
			pixels_ = std::exchange(other.pixels_, nullptr);
			w_ = std::exchange(other.w_, 0);
			h_ = std::exchange(other.h_, 0);
		}

		void InitScreen_()
		{
			detail::window_dc dc(nullptr);
			if (!dc) detail::ThrowWin32("PixelCanvas: GetDC(nullptr) failed.");
			const int w = GetSystemMetrics(SM_CXSCREEN);
			const int h = GetSystemMetrics(SM_CYSCREEN);
			InitFromDC_(dc.get(), w, h, true, nullptr);
		}

		static void InferSize_(HDC dc, int& outW, int& outH)
		{
			outW = 0;
			outH = 0;

			RECT clip{};
			if (GetClipBox(dc, &clip) != ERROR) {
				outW = clip.right - clip.left;
				outH = clip.bottom - clip.top;
			}

			if (outW <= 0 || outH <= 0) {
				outW = GetDeviceCaps(dc, HORZRES);
				outH = GetDeviceCaps(dc, VERTRES);
			}
		}

		void InitFromDC_(HDC target, int w, int h, bool ownsDc, HWND releaseWnd)
		{
			Reset_();
			targetDC_ = target;
			ownsTargetDC_ = ownsDc;
			releaseWnd_ = releaseWnd;

			int iw = w;
			int ih = h;
			if (iw <= 0 || ih <= 0) {
				InferSize_(target, iw, ih);
			}
			if (iw <= 0 || ih <= 0) detail::ThrowWin32("PixelCanvas: unable to infer target size.");

			w_ = iw;
			h_ = ih;

			memDcOwner_.reset(CreateCompatibleDC(target));
			memDC_ = memDcOwner_.get();
			if (!memDC_) detail::ThrowWin32("PixelCanvas: CreateCompatibleDC failed.");

			BITMAPINFO bmi{};
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = w_;
			bmi.bmiHeader.biHeight = -h_; // 负数：顶向下（更符合“窗口坐标系”）
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;

			void* bits = nullptr;
			dibOwner_.reset(CreateDIBSection(target, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0));
			if (!dibOwner_) detail::ThrowWin32("PixelCanvas: CreateDIBSection failed.");

			pixels_ = static_cast<Pixel32Ptr>(bits);
			dibSel_ = detail::select_object_guard(memDC_, dibOwner_.get());
		}
	};


	/**
	 * @brief 你原来的 HuaPing3（改成更像库的写法）
	 *
	 * @param executionTimes  执行次数
	 * @param target          目标 DC（默认 nullptr = 屏幕 DC）
	 */
	inline void HuaPing3(int executionTimes, HDC target = nullptr)
	{
		if (executionTimes <= 0) return;

		PixelCanvas canvas(target);
		canvas.Capture();

		const int w = canvas.width();
		const int h = canvas.height();
		auto* px = canvas.pixels();
		if (!px) return;

		for (int execution = 0; execution < executionTimes; ++execution)
		{
			for (int y = 0; y < h; ++y)
			{
				const int rowBase = y * w;
				for (int x = 0; x < w; ++x)
				{
					const int i = rowBase + x;
					px[i].rgb += static_cast<std::uint32_t>(x * y);
				}
			}
			canvas.Present();
		}
	}

} // namespace evgdi
