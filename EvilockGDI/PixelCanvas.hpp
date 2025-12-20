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

#include <Windows.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <source_location>
#include <stdexcept>
#include <string>
#include <utility>

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

