/*****************************************************************//**
 * @file                gdi_raii.hpp
 * @brief               Win32/GDI 资源 RAII 封装（C++20）
 *
 * @details
 * 1) GDI 句柄（HBITMAP/HFONT/HBRUSH/HICON...）必须配对释放：否则会造成 GDI Objects 泄漏。
 * 2) 传统 Win32 示例代码常常“创建-早退-忘记释放”，对新手很不友好。
 * 3) 本文件提供最小但实用的 RAII：让资源跟随对象生命周期自动释放，减少崩溃与泄漏。
 *
 * @author              EvilLockVirusFramework
 * @date                2025-12-20
 *********************************************************************/

#pragma once

#include <utility>
#include <Windows.h>

namespace evgdi::win {

	/**
	 * @brief 通用 unique_handle：用“析构时调用 DestroyFn”管理 Win32 句柄。
	 *
	 * @tparam Handle     Win32 句柄类型（如 HBITMAP、HBRUSH、HDC...）
	 * @tparam DestroyFn  释放函数指针（如 DeleteObject、DeleteDC、DestroyIcon...）
	 */
	template <class Handle, auto DestroyFn>
	class unique_handle {
	public:
		unique_handle() noexcept = default;
		explicit unique_handle(Handle h) noexcept : handle_(h) {}

		unique_handle(const unique_handle&) = delete;
		unique_handle& operator=(const unique_handle&) = delete;

		unique_handle(unique_handle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}
		unique_handle& operator=(unique_handle&& other) noexcept {
			if (this == &other) return *this;
			reset(std::exchange(other.handle_, nullptr));
			return *this;
		}

		~unique_handle() noexcept { reset(); }

		[[nodiscard]] Handle get() const noexcept { return handle_; }
		[[nodiscard]] explicit operator bool() const noexcept { return handle_ != nullptr; }

		Handle release() noexcept { return std::exchange(handle_, nullptr); }

		void reset(Handle h = nullptr) noexcept {
			if (handle_ != nullptr) {
				DestroyFn(handle_);
			}
			handle_ = h;
		}

	private:
		Handle handle_ = nullptr;
	};

	using unique_hbitmap = unique_handle<HBITMAP, DeleteObject>;
	using unique_hbrush = unique_handle<HBRUSH, DeleteObject>;
	using unique_hfont = unique_handle<HFONT, DeleteObject>;
	using unique_hicon = unique_handle<HICON, DestroyIcon>;
	using unique_hdc = unique_handle<HDC, DeleteDC>; // 注意：仅用于 CreateCompatibleDC 等需要 DeleteDC 的 HDC


	/**
	 * @brief SelectObject 保护：构造时选入对象，析构时自动还原旧对象。
	 *
	 * @details
	 * - GDI 规则：删除被选入 DC 的对象是未定义行为（容易 GDI 崩溃/花屏）。
	 * - 所以我们在析构时总是先还原旧对象，再交给 unique_handle 释放对象。
	 */
	class select_object_guard {
	public:
		select_object_guard() = default;

		select_object_guard(HDC dc, HGDIOBJ obj) : dc_(dc) {
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


	/**
	 * @brief GetDC/ReleaseDC 的 RAII 封装。
	 *
	 * @details
	 * - GetDC 需要用同一个 HWND 配对 ReleaseDC。
	 * - 适合“短期借用窗口 DC”这种场景。
	 */
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

} // namespace evgdi::win

