#include <Windows.h>
#include <chrono>
#include <cstring>
#include <functional>
#include <string>
#include <thread>

#include "common.hpp"
#include "draw.hpp"
#include "ScreenGDI.hpp"
#include "BorderedWindowGDI.hpp"
#include "LayeredWindowGdi.hpp"
#include "LayeredTextout.hpp"
#include "MessageBoxWave.hpp"
#include "PixelCanvas.hpp"

/**
 * @file test.cpp
 * @brief 功能演示 + 冒烟测试（面向 GDI/窗口这类强交互功能）
 *
 * @details
 * “测试全部功能”在 GUI 项目里通常不是纯自动单元测试能覆盖的：因为窗口、消息循环、GDI 渲染都依赖系统状态。
 * 所以本文件的目标是：把库里主要模块都跑一遍，确认不崩溃、能看到效果、并且能正常退出。
 *
 * 运行后会依次演示：
 * 1) 画笔 Pen（类似 Python 海龟的用法）
 * 2) ScreenGDI（对窗口做亮度/对比度/饱和度等处理）
 * 3) BorderedWindowGDI（带边框窗口：移动/旋转/抖动/调色）
 * 4) LayeredWindowGDI（分层窗口：旋转/抖动/调色）
 * 5) LayeredTextOut（分层文字特效：波浪/鱼眼/漩涡等）
 * 6) WaveEffect（弹窗波浪）
 */

namespace {
	bool g_quit = false;

	void PumpMessagesOnce()
	{
		MSG msg{};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) {
				g_quit = true;
				return;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	void RunFor(std::chrono::milliseconds dur, const std::function<void()>& tick = {})
	{
		const auto end = std::chrono::steady_clock::now() + dur;
		while (!g_quit && std::chrono::steady_clock::now() < end)
		{
			PumpMessagesOnce();
			if (tick) tick();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	// ====================== 1) 画笔 Pen 演示窗口 ======================
	LRESULT CALLBACK PenDemoWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps{};
			HDC hdc = BeginPaint(hWnd, &ps);

			Pen pen(hdc, LoadIcon(nullptr, IDI_WARNING));
			pen.speed(0);    // 0 = 不延迟（测试更快）
			pen.pensize(1);

			pen.clearCanvas();

			// 画一个六边形（测试 forward/left）
			pen.penup();
			pen.goto_xy(180, 120);
			pen.pendown();
			pen.setAngle(0);
			pen.drawPolygon(6, 40);

			// 画一个圆（测试 drawCircle）
			pen.penup();
			pen.goto_xy(520, 140);
			pen.pendown();
			pen.drawCircle(50);

			// 画爱心（测试 drawArc + beginFill/endFill）
			pen.penup();
			pen.goto_xy(350, 300);
			pen.setAngle(180);
			pen.pendown();

			pen.beginFill(LoadIcon(nullptr, IDI_ERROR), 15);
			pen.left(50);
			pen.forward(133);
			pen.drawArc(50, 200, CW);
			pen.right(140);
			pen.drawArc(50, 200, CW);
			pen.forward(133);
			pen.endFill();

			// 图标文字（测试 drawTextWithIcons）
			pen.drawTextWithIcons(L"EVILOCK GDI", 120, 420, 2.0f, 18, 6, L"Arial", 80);

			EndPaint(hWnd, &ps);
			return 0;
		}
		case WM_DESTROY:
			return 0;
		default:
			return DefWindowProcW(hWnd, msg, wParam, lParam);
		}
	}

	[[nodiscard]] HWND CreatePenDemoWindow(HINSTANCE hInst)
	{
		WNDCLASSW wc{};
		wc.lpfnWndProc = PenDemoWndProc;
		wc.hInstance = hInst;
		wc.lpszClassName = L"EvgdiPenDemo";
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		RegisterClassW(&wc);

		const int w = 800;
		const int h = 600;
		return CreateWindowW(
			wc.lpszClassName,
			L"EvilockGDI 测试 - 画笔 Pen（Python 风格）",
			WS_OVERLAPPEDWINDOW,
			80, 60, w, h,
			nullptr, nullptr, hInst, nullptr
		);
	}

	void TestPenAndScreenGDI(HINSTANCE hInst)
	{
		HWND hWnd = CreatePenDemoWindow(hInst);
		if (!hWnd) throw_runtime_error("CreatePenDemoWindow failed.");

		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);
		InvalidateRect(hWnd, nullptr, TRUE);

		RunFor(std::chrono::milliseconds(600));

		// PixelCanvas：默认屏幕 DC，也可以传入任意 HDC/窗口（这里演示 FromWindow）
		{
			auto canvas = evgdi::PixelCanvas::FromWindow(hWnd);
			canvas.Capture();
			const int w = canvas.width();
			const int h = canvas.height();
			auto* px = canvas.pixels();
			for (int y = 0; y < h; ++y) {
				for (int x = 0; x < w; ++x) {
					px[y * w + x].rgb ^= static_cast<std::uint32_t>(x * y);
				}
			}
			canvas.Present();
		}

		// ScreenGDI：只操作“本窗口”，避免影响桌面（保留旧接口的测试）
		{
			auto g = ScreenGDI::FromWindow(hWnd);
			g.SetRGB(0, 0, 180, 120, 255, 80, 80);
			g.AdjustBrightness(0.95f);
			g.AdjustContrast(1.10f);
			g.AdjustSaturation(1.10f);
		}

		RunFor(std::chrono::milliseconds(1500));
		DestroyWindow(hWnd);
		RunFor(std::chrono::milliseconds(400));
	}

	// ====================== 2) BorderedWindowGDI ======================
	void TestBorderedWindow(HINSTANCE hInst)
	{
		BorderedWindowGDI w(hInst, 200, 120, 520, 420);
		w.Create("EvgdiBordered", "BorderedWindowGDI 测试", WS_OVERLAPPEDWINDOW);
		if (!w.hWnd) throw_runtime_error("BorderedWindowGDI::Create failed.");

		{
			RECT rect{ 0,0,w.windowWidth,w.windowHeight };
			HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
			FillRect(w.hdcMem, &rect, brush);
			DeleteObject(brush);

			Pen pen(w.hdcMem, LoadIcon(nullptr, IDI_INFORMATION));
			pen.speed(0);
			pen.penup();
			pen.goto_xy(260, 120);
			pen.pendown();
			pen.drawCircle(60);
			pen.drawTextWithIcons(L"Bordered", 140, 260, 2.0f, 16, 4, L"Arial", 60);
		}
		InvalidateRect(w.hWnd, nullptr, TRUE);

		for (int i = 0; i < 120 && !g_quit; ++i)
		{
			w.MoveRight(3, BOUNCE);
			w.MoveDown(2, BOUNCE);
			if (i % 20 == 0) w.Shake(2, 6);
			if (i % 10 == 0) w.turnRight(6);
			if (i == 40) w.AdjustBrightness(0.9f);
			if (i == 70) w.AdjustSaturation(1.2f);
			RunFor(std::chrono::milliseconds(30));
		}

		// 说明：窗口 DC 最好在 DestroyWindow 之前 ReleaseDC。
		if (w.hdcWindow) {
			ReleaseDC(w.hWnd, w.hdcWindow);
			w.hdcWindow = nullptr;
		}
		DestroyWindow(w.hWnd);
		RunFor(std::chrono::milliseconds(400));
	}

	// ====================== 3) LayeredWindowGDI ======================
	void TestLayeredWindow(HINSTANCE hInst)
	{
		LayeredWindowGDI w(hInst, 260, 160, 420, 320);
		w.Create("EvgdiLayered", "LayeredWindowGDI 测试", true);
		if (!w.hWnd) throw_runtime_error("LayeredWindowGDI::Create failed.");

		{
			RECT rect{ 0,0,w.windowWidth,w.windowHeight };
			HBRUSH brush = CreateSolidBrush(RGB(20, 20, 20));
			FillRect(w.hdcMem, &rect, brush);
			DeleteObject(brush);

			Pen pen(w.hdcMem, LoadIcon(nullptr, IDI_WARNING));
			pen.speed(0);
			pen.penup();
			pen.goto_xy(200, 140);
			pen.pendown();
			pen.drawPolygon(5, 50);
			pen.drawTextWithIcons(L"Layered", 110, 200, 2.0f, 16, 5, L"Arial", 70);

			BitBlt(w.hdcWindow, 0, 0, w.windowWidth, w.windowHeight, w.hdcMem, 0, 0, SRCCOPY);
		}

		for (int i = 0; i < 80 && !g_quit; ++i)
		{
			if (i % 15 == 0) w.Shake(1, 10);
			w.turnRight(8);
			if (i == 20) w.AdjustContrast(1.2f);
			if (i == 40) w.AdjustBrightness(0.9f);
			RunFor(std::chrono::milliseconds(40));
		}

		if (w.hdcWindow) {
			ReleaseDC(w.hWnd, w.hdcWindow);
			w.hdcWindow = nullptr;
		}
		DestroyWindow(w.hWnd);
		RunFor(std::chrono::milliseconds(400));
	}

	// ====================== 3B) PixelCanvas：旋转/缩放/移动/裁剪（内部用 GDI+） ======================
	void TestPixelCanvasTransform(HINSTANCE hInst)
	{
		BorderedWindowGDI w(hInst, 820, 120, 520, 420);
		w.Create("EvgdiCanvasTransform", "PixelCanvas 变换测试（旋转/缩放/移动/裁剪）", WS_OVERLAPPEDWINDOW);
		if (!w.hWnd) throw_runtime_error("BorderedWindowGDI::Create failed (PixelCanvas transform).");

		// 先在内存画布上画一张“静态底图”
		{
			RECT rect{ 0,0,w.windowWidth,w.windowHeight };
			HBRUSH brush = CreateSolidBrush(RGB(10, 10, 20));
			FillRect(w.hdcMem, &rect, brush);
			DeleteObject(brush);

			Pen pen(w.hdcMem, LoadIcon(nullptr, IDI_INFORMATION));
			pen.speed(0);
			pen.pensize(2);

			pen.penup();
			pen.goto_xy(260, 90);
			pen.pendown();
			pen.drawPolygon(6, 55);

			pen.penup();
			pen.goto_xy(260, 210);
			pen.pendown();
			pen.drawCircle(60);

			pen.drawTextWithIcons(L"GDI+ MAGIC", 120, 320, 2.0f, 18, 6, L"Arial", 70);
		}
		BitBlt(w.hdcWindow, 0, 0, w.windowWidth, w.windowHeight, w.hdcMem, 0, 0, SRCCOPY);

		// 关键：用 PixelCanvas 抓一帧（从 hdcMem），然后用它的“子方法”做旋转/缩放/裁剪
		evgdi::PixelCanvas canvas(w.hdcMem, w.windowWidth, w.windowHeight);
		canvas.Capture();

		// 额外示例：直接用 HSL 颜色模型改像素（让左上角更“鲜艳”一点）
		for (int y = 0; y < 70; ++y) {
			for (int x = 0; x < 160; ++x) {
				auto hsl = canvas.GetHsl(x, y);
				hsl.s = (std::min)(1.0f, hsl.s + 0.35f);
				hsl.l = (std::min)(1.0f, hsl.l + 0.05f);
				canvas.SetHsl(x, y, hsl);
			}
		}

		// 动起来：每一帧都用 GDI+ 把 canvas.hbitmap() 变形后画到窗口 DC
		const auto start = std::chrono::steady_clock::now();
		for (int i = 0; i < 210 && !g_quit; ++i)
		{
			const float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();

			// 新增：指定旋转中心（让“围绕某个点旋转”更直观）
			canvas.Pivot(
				(w.windowWidth * 0.5f) + std::sinf(t * 1.1f) * 90.0f,
				(w.windowHeight * 0.5f) + std::cosf(t * 0.9f) * 60.0f
			);

			evgdi::PixelCanvas::TransformParams p{};
			p.scale = 1.0f + std::sinf(t * 2.0f) * 0.18f;    // 缩放
			p.rotationDeg = t * 35.0f;                      // 旋转
			p.shearX = std::sinf(t * 1.3f) * 0.10f;         // 倾斜
			p.shearY = std::cosf(t * 1.1f) * 0.06f;
			p.offsetX = std::sinf(t * 3.2f) * 14.0f;        // 平移（抖动）
			p.offsetY = std::cosf(t * 2.7f) * 10.0f;

			// 前半段：用 ClipRect 做“裁剪”（只显示中间一块）
			if (i < 105) {
				p.enableClip = true;
				p.clipRect = RECT{ 70, 40, w.windowWidth - 70, w.windowHeight - 40 };
			}
			// 后半段：用 SrcCrop 做“裁剪”（先从源图裁一块再变形）
			else {
				p.enableSrcCrop = true;
				const int cx = static_cast<int>((std::sinf(t * 1.2f) * 0.5f + 0.5f) * (w.windowWidth * 0.4f));
				const int cy = static_cast<int>((std::cosf(t * 1.1f) * 0.5f + 0.5f) * (w.windowHeight * 0.3f));
				p.srcRect = RECT{ cx, cy, cx + w.windowWidth / 2, cy + w.windowHeight / 2 };
			}

			p.fast = (i % 2 == 0); // 让小朋友能看出“快/清晰”区别

			canvas.PresentTransformed(p, w.hdcWindow);
			RunFor(std::chrono::milliseconds(33));
		}

		if (w.hdcWindow) {
			ReleaseDC(w.hWnd, w.hdcWindow);
			w.hdcWindow = nullptr;
		}
		DestroyWindow(w.hWnd);
		RunFor(std::chrono::milliseconds(400));
	}

	// ====================== 4) LayeredTextOut ======================
	void TestLayeredTextOut()
	{
		LayeredTextOut text;
		if (!text.Create(720, 220)) throw_runtime_error("LayeredTextOut::Create failed.");

		text.SetBackgroundColor(RGB(0, 0, 0));
		text.SetAlpha(230);
		text.SetWindowPosition(120, 120);
		text.Show();

		// 示例 1：彩虹 + 波浪（最基础的“炫酷效果”）
		text.SetText(L"示例1：彩虹 + 波浪");
		text.SetFontFamily(L"微软雅黑");
		text.SetFontSize(46);
		text.SetRainbowMode(true);
		text.SetDynamicGradientSpeed(1.2f);
		text.EnableWave(true, 14.0f, 8.0f, 0.05f, 0.03f);
		text.EnableFishEye(false);
		text.EnableTwirl(false);
		text.EnablePixelate(false);
		text.EnableInvert(false);
		text.EnableGrayscale(false);
		text.SetContrast(1.0f);
		text.SetBrightness(0.0f);
		RunFor(std::chrono::milliseconds(2200), [&]() { text.UpdateWindowContent(); });

		// 示例 2：宽扁拉伸 + 3D 旋转（更像“立体标题”）
		text.SetText(L"示例2：宽扁拉伸 + 3D旋转");
		text.SetWideFlatEffect(true, 1.8f, 0.75f);
		text.EnableDynamicStretch(true, 2.0f);
		text.SetPerspective(0.0022f);
		text.SetRotationX(0.25f);
		text.SetRotationY(0.15f);
		text.SetRotationZ(0.0f);
		text.EnableWave(false);
		text.EnableFishEye(false);
		text.EnableTwirl(false);
		text.SetSolidColorMode(true, RGB(255, 220, 120));
		RunFor(std::chrono::milliseconds(2000), [&]() {
			// 轻微转动，让孩子能看到“3D”变化
			// 注意：SetRotationZ 内部会刷新，为了不把 CPU 拉满，这里限制到 ~30 FPS。
			static auto last = std::chrono::steady_clock::now();
			const auto now = std::chrono::steady_clock::now();
			if (now - last < std::chrono::milliseconds(33)) return;
			last = now;

			auto t = text.GetTransform3D();
			text.SetRotationZ(t.rotationZ + 0.06f);
		});

		// 示例 3：鱼眼 + 漩涡（几何扭曲类效果）
		text.SetWindowPosition(120, 380);
		text.SetText(L"示例3：鱼眼 + 漩涡（扭曲）");
		text.SetRainbowMode(false);
		text.SetSolidColorMode(false);
		text.SetGradientColors(RGB(80, 255, 120), RGB(180, 80, 255));
		text.SetDynamicGradientSpeed(0.9f);
		text.EnableFishEye(true, 0.65f, false);
		text.EnableTwirl(true, 1.0f, false);
		text.EnableWave(false);
		text.EnablePixelate(false);
		RunFor(std::chrono::milliseconds(2400), [&]() { text.UpdateWindowContent(); });

		// 示例 4：像素化 + 灰度/反色 + 亮度/对比度（像“滤镜”）
		text.SetWindowPosition(900, 120);
		text.SetText(L"示例4：像素化 + 灰度/反色（滤镜）");
		text.SetSolidColorMode(true, RGB(255, 255, 255));
		text.EnablePixelate(true, 10);
		text.EnableGrayscale(true);
		text.EnableInvert(false);
		text.SetContrast(1.15f);
		text.SetBrightness(6.0f);
		RunFor(std::chrono::milliseconds(2200), [&]() { text.UpdateWindowContent(); });

		text.EnableInvert(true);
		text.SetText(L"示例4B：反色开启！");
		RunFor(std::chrono::milliseconds(1600), [&]() { text.UpdateWindowContent(); });

		text.Hide();
		text.Destroy();
		RunFor(std::chrono::milliseconds(400));
	}

	// ====================== 5) MessageBox WaveEffect ======================
	void TestMessageBoxWave()
	{
		WaveEffect::CreateWaveEffect(
			L"这是 WaveEffect（弹窗波浪）",
			L"EvilockGDI 测试",
			MB_OK | MB_ICONINFORMATION,
			12, 4, 24, 120
		);

		RunFor(std::chrono::milliseconds(3500));
		WaveEffect::StopWaveEffect();
		RunFor(std::chrono::milliseconds(400));
	}

	int RunAllTests()
	{
		try
		{
			HINSTANCE hInst = GetModuleHandleW(nullptr);

			TestPenAndScreenGDI(hInst);
			TestBorderedWindow(hInst);
			TestLayeredWindow(hInst);
			TestPixelCanvasTransform(hInst);
			TestLayeredTextOut();
			TestMessageBoxWave();

			MessageBoxW(nullptr, L"测试流程已跑完（主要功能已冒烟）。", L"EvilockGDI", MB_OK | MB_ICONINFORMATION);
			return 0;
		}
		catch (const std::exception& e)
		{
			auto ToWide = [](const char* s) -> std::wstring {
				if (!s) return L"(null)";
				// 优先按 UTF-8 转；如果失败，再退回到系统默认代码页（ACP）
				int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
				UINT cp = CP_UTF8;
				if (len <= 0) {
					cp = CP_ACP;
					len = MultiByteToWideChar(cp, 0, s, -1, nullptr, 0);
				}
				if (len <= 0) return L"(convert failed)";

				std::wstring out;
				out.resize(static_cast<std::size_t>(len), L'\0');
				MultiByteToWideChar(cp, 0, s, -1, out.data(), len);
				if (!out.empty() && out.back() == L'\0') out.pop_back();
				return out;
			};

			// 中文提示 + 附带错误原因（便于你定位）
			const std::wstring msg = L"测试过程中发生异常：\n" + ToWide(e.what());
			MessageBoxW(nullptr, msg.c_str(), L"EvilockGDI - 错误", MB_OK | MB_ICONERROR);
			return 1;
		}
	}
} // namespace

// 同时提供 main 与 WinMain：无论工程是 Console 子系统还是 Windows 子系统，都能作为入口运行。
int main()
{
	return RunAllTests();
}
