/*****************************************************************//**
 * @file                kids.hpp
 * @brief               给小朋友用的“像 Python 一样”的最简接口（Header-Only）
 * @details
 * 设计目标：
 * - 小朋友写代码时，尽量不要看到：消息循环、线程、复杂类、HDC 等概念
 * - 只需要像 Python 一样写：open / clear / forward / right / present / wait
 *
 * 最小示例：
 *   #include "kids.hpp"
 *   using namespace evgdi::kids;
 *   int main() {
 *     open(800, 520, L"我的画板");
 *     clear(RGB(255,255,255));
 *     pensize(2);
 *     for (int i=0; i<200 && alive(); ++i) {
 *       forward(4);
 *       right(5);
 *       present();
 *       wait(16);
 *     }
 *     close();
 *   }
 *
 * @date                2025-12-20
 *********************************************************************/

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "draw.hpp"
#include "PixelCanvas.hpp"
#include "MessageBoxWave.hpp"

namespace evgdi::kids {

	namespace detail {
		[[nodiscard]] inline int ScreenW_() { return GetSystemMetrics(SM_CXSCREEN); }
		[[nodiscard]] inline int ScreenH_() { return GetSystemMetrics(SM_CYSCREEN); }
		[[nodiscard]] inline int ClampInt_(int v, int lo, int hi) { return (std::max)(lo, (std::min)(hi, v)); }

		[[nodiscard]] inline std::vector<std::pair<int, int>> MakeCirclePath_(
			int cx, int cy, int radius, int points)
		{
			points = ClampInt_(points, 8, 2000);
			radius = (radius <= 0) ? 200 : radius;

			std::vector<std::pair<int, int>> path;
			path.reserve(static_cast<std::size_t>(points));

			const float pi = 3.1415926535f;
			for (int i = 0; i < points; ++i) {
				const float a = (2.0f * pi) * (static_cast<float>(i) / static_cast<float>(points));
				const int x = static_cast<int>(static_cast<float>(cx) + std::cosf(a) * static_cast<float>(radius));
				const int y = static_cast<int>(static_cast<float>(cy) + std::sinf(a) * static_cast<float>(radius));
				path.emplace_back(x, y);
			}
			return path;
		}

		[[nodiscard]] inline std::vector<std::pair<int, int>> MakeRectPath_(
			int left, int top, int right, int bottom)
		{
			std::vector<std::pair<int, int>> path;
			path.reserve(5);
			path.emplace_back(left, top);
			path.emplace_back(right, top);
			path.emplace_back(right, bottom);
			path.emplace_back(left, bottom);
			path.emplace_back(left, top);
			return path;
		}

		class KidsApp {
		public:
			void Open(int clientW, int clientH, const std::wstring& title)
			{
				if (running_) return;

				width_ = (clientW > 0) ? clientW : 800;
				height_ = (clientH > 0) ? clientH : 600;
				title_ = title.empty() ? L"EvilockGDI Kids" : title;

				Register_();

				const DWORD style = WS_OVERLAPPEDWINDOW;
				RECT wr{ 0, 0, width_, height_ };
				AdjustWindowRect(&wr, style, FALSE);
				const int winW = wr.right - wr.left;
				const int winH = wr.bottom - wr.top;

				hwnd_ = CreateWindowW(
					className_,
					title_.c_str(),
					style,
					120, 80, winW, winH,
					nullptr, nullptr, GetModuleHandleW(nullptr), this
				);
				if (!hwnd_) return;

				ShowWindow(hwnd_, SW_SHOW);
				UpdateWindow(hwnd_);

				windowDC_ = GetDC(hwnd_);
				if (!windowDC_) { Cleanup_(); return; }

				memDC_ = CreateCompatibleDC(windowDC_);
				outDC_ = CreateCompatibleDC(windowDC_);
				if (!memDC_ || !outDC_) { Cleanup_(); return; }

				memBmp_ = CreateCompatibleBitmap(windowDC_, width_, height_);
				outBmp_ = CreateCompatibleBitmap(windowDC_, width_, height_);
				if (!memBmp_ || !outBmp_) { Cleanup_(); return; }

				oldMemBmp_ = SelectObject(memDC_, memBmp_);
				oldOutBmp_ = SelectObject(outDC_, outBmp_);

				// 默认笔尖：信息图标（更可爱）
				icon_ = LoadIcon(nullptr, IDI_INFORMATION);
				pen_ = std::make_unique<::Pen>(memDC_, icon_);

				canvas_ = std::make_unique<evgdi::PixelCanvas>(memDC_, width_, height_);
				effectsEnabled_ = false;

				// open() 之后就认为“程序开始运行”了：这样第一次 present() 能立刻生效
				running_ = true;

				Clear(RGB(255, 255, 255));
				Present();
			}

			void Close()
			{
				if (!hwnd_) return;
				DestroyWindow(hwnd_);
				Pump_();
				Cleanup_();
			}

			[[nodiscard]] bool Alive()
			{
				Pump_();
				return running_ && hwnd_ != nullptr;
			}

			void Clear(COLORREF color)
			{
				if (!memDC_) return;
				HBRUSH brush = CreateSolidBrush(color);
				if (!brush) return;
				RECT r{ 0, 0, width_, height_ };
				FillRect(memDC_, &r, brush);
				DeleteObject(brush);
			}

			void EnableEffects(bool on)
			{
				effectsEnabled_ = on;
			}

			void ResetEffects()
			{
				effectsEnabled_ = true;
				auto& c = CanvasRef();
				c.EnableState(true);
				c.ResetTransform();
			}

			void Present()
			{
				Pump_();
				if (!running_ || !windowDC_ || !outDC_ || !memDC_) return;

				if (!effectsEnabled_) {
					BitBlt(outDC_, 0, 0, width_, height_, memDC_, 0, 0, SRCCOPY);
				}
				else {
					auto& c = CanvasRef();
					c.Capture();
					c.PresentTransformed(outDC_);
				}

				BitBlt(windowDC_, 0, 0, width_, height_, outDC_, 0, 0, SRCCOPY);
			}

			void Wait(int ms)
			{
				if (ms <= 0) { Pump_(); return; }

				const ULONGLONG end = GetTickCount64() + static_cast<ULONGLONG>(ms);
				while (Alive() && GetTickCount64() < end) {
					// Sleep 是 WinAPI，不需要解释“线程”概念
					::Sleep(1);
				}
			}

			::Pen& PenRef()
			{
				if (!pen_) {
					icon_ = LoadIcon(nullptr, IDI_INFORMATION);
					pen_ = std::make_unique<::Pen>(memDC_, icon_);
				}
				return *pen_;
			}

			evgdi::PixelCanvas& CanvasRef()
			{
				if (!canvas_) {
					canvas_ = std::make_unique<evgdi::PixelCanvas>(memDC_, width_, height_);
				}
				return *canvas_;
			}

		private:
			static constexpr const wchar_t* className_ = L"EvgdiKidsWindow";

			HWND hwnd_ = nullptr;
			HDC windowDC_ = nullptr;
			HDC memDC_ = nullptr;
			HDC outDC_ = nullptr;
			HBITMAP memBmp_ = nullptr;
			HBITMAP outBmp_ = nullptr;
			HGDIOBJ oldMemBmp_ = nullptr;
			HGDIOBJ oldOutBmp_ = nullptr;
			HICON icon_ = nullptr;

			int width_ = 800;
			int height_ = 600;
			std::wstring title_;

			bool running_ = false;
			bool effectsEnabled_ = false;

			std::unique_ptr<::Pen> pen_;
			std::unique_ptr<evgdi::PixelCanvas> canvas_;

			static void Register_()
			{
				static bool registered = false;
				if (registered) return;

				WNDCLASSW wc{};
				wc.lpfnWndProc = WndProc_;
				wc.hInstance = GetModuleHandleW(nullptr);
				wc.lpszClassName = className_;
				wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
				wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
				RegisterClassW(&wc);
				registered = true;
			}

			static LRESULT CALLBACK WndProc_(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
			{
				KidsApp* self = reinterpret_cast<KidsApp*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
				if (msg == WM_NCCREATE) {
					auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
					self = reinterpret_cast<KidsApp*>(cs->lpCreateParams);
					SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
				}

				switch (msg) {
				case WM_ERASEBKGND:
					return 1; // 避免闪烁
				case WM_CLOSE:
					DestroyWindow(hwnd);
					return 0;
				case WM_DESTROY:
					if (self) {
						self->running_ = false;
						self->hwnd_ = nullptr;
					}
					return 0;
				case WM_PAINT:
					if (self && self->outDC_) {
						PAINTSTRUCT ps{};
						HDC hdc = BeginPaint(hwnd, &ps);
						BitBlt(hdc, 0, 0, self->width_, self->height_, self->outDC_, 0, 0, SRCCOPY);
						EndPaint(hwnd, &ps);
						return 0;
					}
					break;
				}
				return DefWindowProcW(hwnd, msg, wp, lp);
			}

			void Pump_()
			{
				MSG msg{};
				while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
					if (msg.message == WM_QUIT) {
						running_ = false;
						break;
					}
					TranslateMessage(&msg);
					DispatchMessageW(&msg);
				}
			}

			void Cleanup_()
			{
				pen_.reset();
				canvas_.reset();

				if (outDC_ && oldOutBmp_) { SelectObject(outDC_, oldOutBmp_); oldOutBmp_ = nullptr; }
				if (memDC_ && oldMemBmp_) { SelectObject(memDC_, oldMemBmp_); oldMemBmp_ = nullptr; }

				if (outBmp_) { DeleteObject(outBmp_); outBmp_ = nullptr; }
				if (memBmp_) { DeleteObject(memBmp_); memBmp_ = nullptr; }
				if (outDC_) { DeleteDC(outDC_); outDC_ = nullptr; }
				if (memDC_) { DeleteDC(memDC_); memDC_ = nullptr; }
				if (windowDC_ && hwnd_) { ReleaseDC(hwnd_, windowDC_); windowDC_ = nullptr; }

				hwnd_ = nullptr;
				running_ = false;
				effectsEnabled_ = false;
			}
		};

		inline KidsApp& App()
		{
			static KidsApp app;
			return app;
		}
	} // namespace detail


	// =========================
	// 下面是“给小朋友用”的函数
	// =========================

	// --- 窗口与时间 ---
	inline void open(int w = 800, int h = 600, const wchar_t* title = L"EvilockGDI Kids")
	{
		detail::App().Open(w, h, title ? std::wstring(title) : std::wstring{});
	}

	inline void close()
	{
		detail::App().Close();
	}

	inline bool alive()
	{
		return detail::App().Alive();
	}

	inline void wait(int ms)
	{
		detail::App().Wait(ms);
	}

	inline void clear(COLORREF color = RGB(255, 255, 255))
	{
		detail::App().Clear(color);
	}

	inline void present()
	{
		detail::App().Present();
	}

	// --- 画笔（像 turtle 一样） ---
	inline void penup() { detail::App().PenRef().penup(); }
	inline void pendown() { detail::App().PenRef().pendown(); }
	inline void speed(int ms) { detail::App().PenRef().speed(ms); }
	inline void pensize(int w) { detail::App().PenRef().pensize(w); }
	inline void pencolor(COLORREF c) { detail::App().PenRef().pencolor(c); }
	inline void home() { detail::App().PenRef().home(); }
	inline void goto_xy(int x, int y) { detail::App().PenRef().goto_xy(x, y); }
	inline void forward(int d) { detail::App().PenRef().forward(d); }
	inline void backward(int d) { detail::App().PenRef().backward(d); }
	inline void left(float deg) { detail::App().PenRef().left(deg); }
	inline void right(float deg) { detail::App().PenRef().right(deg); }
	inline void circle(int r) { detail::App().PenRef().drawCircle(r); }
	inline void polygon(int sides, int len) { detail::App().PenRef().drawPolygon(sides, len); }

	// --- 特效（PixelCanvas：状态机制默认开） ---
	inline void effects_on()
	{
		detail::App().EnableEffects(true);
		detail::App().ResetEffects();
	}

	inline void effects_off()
	{
		detail::App().EnableEffects(false);
	}

	inline void effects_reset()
	{
		detail::App().ResetEffects();
	}

	inline void fast(bool on)
	{
		detail::App().EnableEffects(true);
		detail::App().CanvasRef().Fast(on);
	}

	inline void state(bool on)
	{
		detail::App().EnableEffects(true);
		detail::App().CanvasRef().EnableState(on);
	}

	inline void perspective_on(float strength = 0.0022f)
	{
		detail::App().EnableEffects(true);
		auto& c = detail::App().CanvasRef();
		c.EnablePerspective(true);
		c.SetPerspective(strength);
	}

	inline void perspective_off()
	{
		detail::App().EnableEffects(true);
		detail::App().CanvasRef().EnablePerspective(false);
	}

	// 旋转中心：默认是画板中心；你也可以指定成任意点（比如鼠标位置）
	inline void pivot(float x, float y) { detail::App().EnableEffects(true); detail::App().CanvasRef().Pivot(x, y); }
	inline void pivot_center() { detail::App().EnableEffects(true); detail::App().CanvasRef().PivotCenter(); }

	inline void rotate_x(float deg) { detail::App().EnableEffects(true); detail::App().CanvasRef().RotateX(deg); }
	inline void rotate_y(float deg) { detail::App().EnableEffects(true); detail::App().CanvasRef().RotateY(deg); }
	inline void rotate_z(float deg) { detail::App().EnableEffects(true); detail::App().CanvasRef().Rotate(deg); }
	inline void rotate_z_at(float deg, float x, float y) { pivot(x, y); rotate_z(deg); }
	inline void move(float dx, float dy) { detail::App().EnableEffects(true); detail::App().CanvasRef().Move(dx, dy); }
	inline void zoom(float s) { detail::App().EnableEffects(true); detail::App().CanvasRef().Scale(s); }
	inline void push_z(float dz) { detail::App().EnableEffects(true); detail::App().CanvasRef().TranslateZ(dz); }

	inline void fisheye_on(float strength = 0.75f, float radius = 220.0f)
	{
		detail::App().EnableEffects(true);
		detail::App().CanvasRef().SetFishEye(strength, radius);
	}

	inline void fisheye_at(float cx, float cy, float strength = 0.75f, float radius = 220.0f)
	{
		detail::App().EnableEffects(true);
		detail::App().CanvasRef().FishEye(cx, cy, strength, radius);
	}

	inline void fisheye_off()
	{
		detail::App().EnableEffects(true);
		detail::App().CanvasRef().DisableFishEye();
	}

	// --- 弹窗特效（最简单用法） ---
	// 小朋友不需要理解 hook：你只要把它当成“会动的消息框”就行。

	// 开始波浪弹窗（会一直跑，直到你 stop）
	inline void wavebox_start(const wchar_t* text,
		const wchar_t* caption = L"EvilockGDI Wave",
		UINT uType = MB_OK | MB_ICONINFORMATION,
		int queueLength = 12, int stepSize = 4,
		int windowSpacing = 24, int creationDelay = 120)
	{
		WaveEffect::CreateWaveEffect(
			text ? std::wstring(text) : std::wstring{},
			caption ? std::wstring(caption) : std::wstring{},
			uType,
			queueLength, stepSize, windowSpacing, creationDelay
		);
	}

	// 停止波浪弹窗（会关闭已经出现的那些消息框）
	inline void wavebox_stop()
	{
		WaveEffect::StopWaveEffect();
	}

	// 一次性播放：开始 -> 等一会儿 -> 停止（最适合“最短代码”）
	inline void wavebox(const wchar_t* text,
		const wchar_t* caption = L"EvilockGDI Wave",
		int durationMs = 3500,
		UINT uType = MB_OK | MB_ICONINFORMATION,
		int queueLength = 12, int stepSize = 4,
		int windowSpacing = 24, int creationDelay = 120)
	{
		wavebox_start(text, caption, uType, queueLength, stepSize, windowSpacing, creationDelay);
		if (durationMs > 0) ::Sleep(static_cast<DWORD>(durationMs));
		wavebox_stop();
	}

	// --- “画轨迹”更简单的版本：小朋友只选形状 + 参数 ---

	// 圆形轨迹：最推荐的“酷炫入门”
	inline void wavebox_circle(const wchar_t* text,
		int durationMs = 3500,
		const wchar_t* caption = L"Wave Circle",
		int radius = 0,
		int points = 120,
		UINT uType = MB_OK | MB_ICONINFORMATION,
		int queueLength = 18, int stepSize = 6,
		int windowSpacing = 22, int creationDelay = 80)
	{
		const int sw = detail::ScreenW_();
		const int sh = detail::ScreenH_();
		const int cx = sw / 2;
		const int cy = sh / 2;
		const int r = (radius > 0) ? radius : static_cast<int>(static_cast<float>((std::min)(sw, sh)) * 0.30f);

		const auto path = detail::MakeCirclePath_(cx, cy, r, points);
		WaveEffect::CreateCustomWaveEffect(
			text ? std::wstring(text) : std::wstring{},
			caption ? std::wstring(caption) : std::wstring{},
			uType,
			path,
			queueLength, stepSize, windowSpacing, creationDelay
		);

		if (durationMs > 0) ::Sleep(static_cast<DWORD>(durationMs));
		WaveEffect::StopWaveEffect();
	}

	// 矩形轨迹：绕屏幕边缘跑（像“巡逻队”）
	inline void wavebox_rect(const wchar_t* text,
		int durationMs = 3500,
		const wchar_t* caption = L"Wave Rect",
		int margin = 80,
		UINT uType = MB_OK | MB_ICONINFORMATION,
		int queueLength = 16, int stepSize = 8,
		int windowSpacing = 24, int creationDelay = 70)
	{
		const int sw = detail::ScreenW_();
		const int sh = detail::ScreenH_();
		margin = detail::ClampInt_(margin, 0, (std::min)(sw, sh) / 3);

		const int left = margin;
		const int top = margin;
		const int right = (std::max)(left + 1, sw - margin);
		const int bottom = (std::max)(top + 1, sh - margin);

		const auto path = detail::MakeRectPath_(left, top, right, bottom);
		WaveEffect::CreateCustomWaveEffect(
			text ? std::wstring(text) : std::wstring{},
			caption ? std::wstring(caption) : std::wstring{},
			uType,
			path,
			queueLength, stepSize, windowSpacing, creationDelay
		);

		if (durationMs > 0) ::Sleep(static_cast<DWORD>(durationMs));
		WaveEffect::StopWaveEffect();
	}

} // namespace evgdi::kids
