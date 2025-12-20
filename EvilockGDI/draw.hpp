/*****************************************************************//**
 * @file				draw.hpp
 * @brief				Icon类绘制 - 图形绘制库
 *
 * @author				EvilLockVirusFramework
 * @date				2024-01-06
 *********************************************************************/

#ifndef DRAW_HPP
#define DRAW_HPP

#pragma once
#include "common.hpp"
#include "gdi_raii.hpp"
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <set>

constexpr float CW = 1.0F;										///< ClockWise 顺时针
constexpr float CCW = -1.0F;									///< Counter-ClockWise 逆时针
constexpr bool	ICON_PEN_DOWN = true;							///< 画笔落下
constexpr bool	ICON_PEN_UP = false;							///< 画笔抬起

/**
 * @brief														: Icon类绘制 - 图形绘制库
 * @details														: 使用标准图形坐标系（Y轴向下，0度指向正右方）
 */
class Pen {
private:
	HDC hdc;													///< 目标设备上下文
	HICON icon;													///< 图标句柄
	bool penState;												///< 画笔状态
	int sensitivity;											///< 灵敏度（图标间隔）
	int penSpeed;												///< 画笔速度（毫秒延迟）
	POINT_2D position;											///< 当前位置
	POINT_2D homePosition;										///< “家/原点”：构造时默认在窗口中心（便于新手理解）
	float angle;												///< 当前方向（弧度制，0度指向正右方）
	COLORREF penColor;											///< 画笔颜色
	int penWidth;												///< 画笔宽度
	std::vector<POINT_2D> fillPoints;							///< 填充路径点
	bool isFilling;												///< 是否正在记录填充路径
	HICON fillIcon;												///< 填充图标句柄
	int fillDensity;											///< 填充密度（图标间距）

	/**
	 * @brief 获取当前“画布”的客户区矩形（兼容窗口DC / 内存DC）
	 * @details
	 * - 如果 hdc 关联到窗口：优先取窗口客户区大小；
	 * - 如果是内存DC：优先取当前选入的位图大小；
	 * - 再不行才退化到 GetDeviceCaps。
	 *
	 * 这样 kids.hpp / PixelCanvas 这类“先画到内存，再统一呈现”的用法也能正常工作。
	 */
	[[nodiscard]] RECT canvasRect_() const
	{
		RECT rect{ 0,0,0,0 };

		if (hdc) {
			if (HWND hwnd = WindowFromDC(hdc)) {
				RECT r{};
				if (GetClientRect(hwnd, &r) && (r.right > r.left) && (r.bottom > r.top)) {
					return r;
				}
			}

			// 内存DC：尝试从当前选入的位图拿尺寸
			if (HGDIOBJ obj = GetCurrentObject(hdc, OBJ_BITMAP)) {
				BITMAP bm{};
				if (GetObject(obj, sizeof(bm), &bm) == sizeof(bm) && bm.bmWidth > 0 && bm.bmHeight > 0) {
					rect.right = bm.bmWidth;
					rect.bottom = bm.bmHeight;
					return rect;
				}
			}

			// 兜底：屏幕/设备尺寸（某些DC这里也能工作）
			rect.right = GetDeviceCaps(hdc, HORZRES);
			rect.bottom = GetDeviceCaps(hdc, VERTRES);
		}

		// 最后兜底：给一个合理默认值，避免出现 0 导致除零/坐标异常
		if (rect.right <= 0) rect.right = 800;
		if (rect.bottom <= 0) rect.bottom = 600;
		return rect;
	}

public:
	/**
	 * @brief													: 获取当前位置
	 * @return													: 当前位置
	 */
	POINT_2D getStartPos() const { return position; }

	/**
	 * @brief													: 初始化IconDrawer类
	 *
	 * @param[in]		hdc										: 目标设备上下文
	 * @param[in]		icon									: 图标句柄
	 */
	explicit Pen(HDC hdc, HICON icon = nullptr) :
		hdc(hdc), icon(icon), penState(ICON_PEN_DOWN),
		sensitivity(10), penSpeed(10), angle(0.0f),
		penColor(RGB(0, 0, 0)), penWidth(1), isFilling(false),
		fillIcon(nullptr), fillDensity(15)
	{
		const RECT rect = canvasRect_();
		const float width = static_cast<float>(rect.right - rect.left);
		const float height = static_cast<float>(rect.bottom - rect.top);

		// 初始位置在窗口中心
		position.set_float_x(width / 2.0F);
		position.set_float_y(height / 2.0F);
		homePosition = position;
	}

	/**
	 * @brief Python 风格：回到“家”（窗口中心）
	 * @details 海龟库里常见的 `home()`，对孩子来说比“设置坐标”更直观。
	 */
	void home()
	{
		position = homePosition;
	}

	/**
	 * @brief Python 风格：抬起画笔（移动不画）
	 */
	void penup() { setPenStatus(ICON_PEN_UP); }

	/**
	 * @brief Python 风格：落下画笔（移动会画）
	 */
	void pendown() { setPenStatus(ICON_PEN_DOWN); }

	/**
	 * @brief Python 风格：设置画笔速度（单位：毫秒）
	 * @details 数值越小越快；0 表示不延迟。
	 */
	void speed(int ms) { setPenSpeed(ms); }

	/**
	 * @brief Python 风格：设置画笔粗细
	 */
	void pensize(int width) { setPenWidth(width); }

	/**
	 * @brief Python 风格：设置画笔颜色
	 */
	void pencolor(COLORREF color) { setPenColor(color); }

	/**
	 * @brief Python 风格：瞬移到指定坐标（不改变朝向）
	 */
	void goto_xy(int x, int y) { setStartPos(x, y); }

	// ==================== 字符点阵绘制功能 ====================

	/**
	 * @brief 获取字符的点阵数据（支持中文）
	 * @param ch 要解析的字符（宽字符）
	 * @param fontName 字体名称
	 * @param fontSize 字体大小
	 * @return 点阵数据向量
	 */
	std::vector<POINT_2D> getCharBitmap(wchar_t ch, const std::wstring& fontName = L"微软雅黑", int fontSize = 20) {
		std::vector<POINT_2D> points;

		// 创建字体（使用宽字符版本）
		evgdi::win::unique_hfont font(CreateFontW(
			-fontSize, 0, 0, 0, FW_BOLD,
			FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
			fontName.c_str()
		));

		if (!font) return points;

		// 创建内存DC（需要 DeleteDC，所以用 unique_hdc）
		evgdi::win::unique_hdc memDC(CreateCompatibleDC(hdc));
		if (!memDC) return points;

		evgdi::win::select_object_guard fontSel(memDC.get(), font.get());

		// 获取字符尺寸（宽字符版本）
		SIZE size;
		GetTextExtentPoint32W(memDC.get(), &ch, 1, &size);

		// 创建位图
		evgdi::win::unique_hbitmap bitmap(CreateCompatibleBitmap(hdc, size.cx, size.cy));
		if (!bitmap) return points;

		evgdi::win::select_object_guard bmpSel(memDC.get(), bitmap.get());

		// 设置背景为白色
		RECT rect = { 0, 0, size.cx, size.cy };
		evgdi::win::unique_hbrush whiteBrush(CreateSolidBrush(RGB(255, 255, 255)));
		if (!whiteBrush) return points;
		FillRect(memDC.get(), &rect, whiteBrush.get());

		// 绘制字符（黑色）- 宽字符版本
		SetTextColor(memDC.get(), RGB(0, 0, 0));
		SetBkMode(memDC.get(), TRANSPARENT);
		TextOutW(memDC.get(), 0, 0, &ch, 1);

		// 扫描位图获取点阵数据
		for (int y = 0; y < size.cy; y++) {
			for (int x = 0; x < size.cx; x++) {
				COLORREF color = GetPixel(memDC.get(), x, y);
				// 如果不是白色（即字符像素），则记录该点
				if (color != RGB(255, 255, 255)) {
					POINT_2D point;
					point.set_int_x(x);
					point.set_int_y(y);
					points.push_back(point);
				}
			}
		}

		return points;
	}

	/**
	 * @brief 根据填充密度筛选点阵数据
	 * @param points 原始点阵数据
	 * @param fillDensity 填充密度 (0-10)
	 *        0: 只绘制轮廓
	 *        5: 中等填充
	 *        10: 完全填充
	 * @return 筛选后的点阵数据
	 */
	std::vector<POINT_2D> filterPointsByDensity(const std::vector<POINT_2D>& points, int fillDensity) {
		if (points.empty() || fillDensity >= 10) {
			return points; // 完全填充或空数据
		}

		if (fillDensity <= 0) {
			return getOutlinePoints(points); // 只绘制轮廓
		}

		// 获取边界框
		int minX = points[0].get_int_x(), maxX = points[0].get_int_x();
		int minY = points[0].get_int_y(), maxY = points[0].get_int_y();
		for (const auto& point : points) {
			minX = (std::min)(minX, point.get_int_x());
			maxX = (std::max)(maxX, point.get_int_x());
			minY = (std::min)(minY, point.get_int_y());
			maxY = (std::max)(maxY, point.get_int_y());
		}

		std::vector<POINT_2D> filteredPoints;

		// 根据密度计算采样间隔
		int step = 11 - fillDensity; // 密度越高，步长越小

		// 分层采样：从轮廓开始，逐渐向内填充
		for (int layer = 0; layer < fillDensity; layer++) {
			int currentStep = step + layer;
			for (int i = layer; i < points.size(); i += currentStep) {
				filteredPoints.push_back(points[i]);
			}
		}

		// 确保包含轮廓点
		std::vector<POINT_2D> outlinePoints = getOutlinePoints(points);
		for (const auto& point : outlinePoints) {
			if (std::find(filteredPoints.begin(), filteredPoints.end(), point) == filteredPoints.end()) {
				filteredPoints.push_back(point);
			}
		}

		return filteredPoints;
	}

	/**
	 * @brief 获取轮廓点（边界点）
	 */
	std::vector<POINT_2D> getOutlinePoints(const std::vector<POINT_2D>& points) {
		if (points.empty()) return points;

		std::vector<POINT_2D> outlinePoints;
		std::set<POINT_2D> pointSet(points.begin(), points.end());

		// 检查每个点的邻居，如果缺少某个方向的邻居，则是边界点
		int dx[] = { -1, 1, 0, 0 };
		int dy[] = { 0, 0, -1, 1 };

		for (const auto& point : points) {
			bool isOutline = false;
			for (int i = 0; i < 4; i++) {
				POINT_2D neighbor{
					static_cast<float>(point.get_int_x() + dx[i]),
					static_cast<float>(point.get_int_y() + dy[i])
				};
				if (pointSet.find(neighbor) == pointSet.end()) {
					isOutline = true;
					break;
				}
			}
			if (isOutline) {
				outlinePoints.push_back(point);
			}
		}

		return outlinePoints;
	}

	/**
	 * @brief 使用图标绘制字符串（支持中文和填充密度控制）
	 * @param text 要绘制的文本（宽字符）
	 * @param startX 起始X坐标
	 * @param startY 起始Y坐标
	 * @param scale 缩放比例
	 * @param spacing 字符间距
	 * @param fillDensity 填充密度 (0-10)
	 * @param fontName 字体名称
	 * @param fontSize 字体大小
	 */
	void drawTextWithIcons(const std::wstring& text, int startX, int startY,
		float scale = 1.0f, int spacing = 5, int fillDensity = 10,
		const std::wstring& fontName = L"微软雅黑", int fontSize = 20) {
		if (!icon) return;

		int currentX = startX;

		for (wchar_t ch : text) {
			// 获取字符点阵数据
			std::vector<POINT_2D> charPoints = getCharBitmap(ch, fontName, fontSize);

			// 根据填充密度筛选点
			std::vector<POINT_2D> filteredPoints = filterPointsByDensity(charPoints, fillDensity);

			// 绘制筛选后的点
			for (const auto& point : filteredPoints) {
				int x = currentX + static_cast<int>(point.get_int_x() * scale);
				int y = startY + static_cast<int>(point.get_int_y() * scale);
				DrawIconEx(hdc, x, y, icon, 0, 0, 0, nullptr, DI_NORMAL);
			}

			// 计算字符宽度并更新位置
			if (!charPoints.empty()) {
				int maxX = 0;
				for (const auto& point : charPoints) {
					maxX = (std::max)(maxX, point.get_int_x());
				}
				currentX += static_cast<int>((maxX + spacing) * scale);
			}
		}
	}

	// ==================== 原有绘图功能 ====================

	/**
	 * @brief													: 开始填充 - 类似turtle.begin_fill()
	 * @param[in]		fillIcon								: 填充图标（可选，nullptr使用颜色填充）
	 * @param[in]		density									: 填充密度（图标间距，默认15）
	 */
	void beginFill(HICON fillIcon = nullptr, int density = 15) {
		fillPoints.clear();
		isFilling = true;
		this->fillIcon = fillIcon;
		this->fillDensity = (density > 0) ? density : 15;

		// 记录当前位置作为起点
		fillPoints.push_back(position);
	}

	/**
	 * @brief													: 结束填充 - 类似turtle.end_fill()
	 * @param[in]		fillColor								: 填充颜色（当不使用图标填充时）
	 */
	void endFill(COLORREF fillColor = RGB(255, 0, 0)) {
		if (!isFilling || fillPoints.size() < 3) {
			isFilling = false;
			fillPoints.clear();
			fillIcon = nullptr;
			return;
		}

		// 确保路径闭合
		if (fillPoints.front().get_int_x() != fillPoints.back().get_int_x() ||
			fillPoints.front().get_int_y() != fillPoints.back().get_int_y()) {
			fillPoints.push_back(fillPoints.front());
		}

		// 根据是否使用图标选择填充方式
		if (fillIcon) {
			fillWithIcons(fillPoints, fillIcon, fillDensity);
		}
		else {
			fillPolygon(fillPoints, fillColor);
		}

		isFilling = false;
		fillPoints.clear();
		fillIcon = nullptr;
	}

	/**
	 * @brief													: 使用图标填充多边形区域
	 *
	 * @param[in]		points									: 多边形顶点
	 * @param[in]		fillIcon								: 填充图标
	 * @param[in]		density									: 填充密度
	 */
	void fillWithIcons(const std::vector<POINT_2D>& points, HICON fillIcon, int density) {
		if (points.size() < 3 || !fillIcon) return;

		// 获取多边形的边界框
		int minX = points[0].get_int_x();
		int maxX = points[0].get_int_x();
		int minY = points[0].get_int_y();
		int maxY = points[0].get_int_y();

		for (const auto& point : points) {
			minX = (std::min)(minX, point.get_int_x());
			maxX = (std::max)(maxX, point.get_int_x());
			minY = (std::min)(minY, point.get_int_y());
			maxY = (std::max)(maxY, point.get_int_y());
		}

		// 创建GDI点数组用于命中测试
		std::vector<POINT> gdiPoints;
		for (const auto& point : points) {
			gdiPoints.push_back({ point.get_int_x(), point.get_int_y() });
		}

		// 创建区域用于命中测试
		HRGN hRegion = CreatePolygonRgn(gdiPoints.data(), static_cast<int>(gdiPoints.size()), WINDING);

		// 保存当前图标状态
		HICON oldIcon = icon;
		int oldPenSpeed = penSpeed;

		// 设置填充图标和速度
		icon = fillIcon;
		penSpeed = 0; // 填充时不需要延迟

		// 扫描填充
		for (int y = minY; y <= maxY; y += density) {
			for (int x = minX; x <= maxX; x += density) {
				if (PtInRegion(hRegion, x, y)) {
					DrawIcon(x, y);
				}
			}
		}

		// 恢复状态
		icon = oldIcon;
		penSpeed = oldPenSpeed;
		DeleteObject(hRegion);
	}

	/**
	 * @brief													: 填充多边形区域
	 *
	 * @param[in]		points									: 多边形顶点
	 * @param[in]		fillColor								: 填充颜色
	 */
	void fillPolygon(const std::vector<POINT_2D>& points, COLORREF fillColor) {
		if (points.size() < 3) return;

		// 创建多边形点数组
		std::vector<POINT> gdiPoints;
		for (const auto& point : points) {
			gdiPoints.push_back({ point.get_int_x(), point.get_int_y() });
		}

		// 使用GDI填充多边形
		HBRUSH hBrush = CreateSolidBrush(fillColor);
		HPEN hPen = CreatePen(PS_NULL, 1, fillColor);

		HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
		HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

		Polygon(hdc, gdiPoints.data(), static_cast<int>(gdiPoints.size()));

		SelectObject(hdc, hOldBrush);
		SelectObject(hdc, hOldPen);
		DeleteObject(hBrush);
		DeleteObject(hPen);
	}

	/**
	 * @brief													: 绘制直线
	 * @details													: 以angle为方向，从当前位置绘制指定长度的直线
	 *
	 * @param[in]		distance								: 长度
	 */
	void forward(int distance)
	{
		if (distance == 0) return;

		const float disF = static_cast<float>(distance);
		const float cosA = cosf(angle);
		const float sinA = sinf(angle);

		POINT_2D startPos = position;
		POINT_2D endPos;
		endPos.set_float_x(position.get_float_x() + disF * cosA);
		endPos.set_float_y(position.get_float_y() + disF * sinA);

		// 更新位置
		position = endPos;

		// 记录填充路径点
		if (isFilling && penState) {
			fillPoints.push_back(position);
		}

		// 如果画笔落下，绘制直线
		if (penState)
		{
			if (icon)
			{
				// 使用图标绘制
				drawLineWithIcons(startPos, endPos);
			}
			else
			{
				// 使用GDI直线绘制
				HPEN hPen = CreatePen(PS_SOLID, penWidth, penColor);
				HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

				MoveToEx(hdc, startPos.get_int_x(), startPos.get_int_y(), nullptr);
				LineTo(hdc, endPos.get_int_x(), endPos.get_int_y());

				SelectObject(hdc, hOldPen);
				DeleteObject(hPen);
			}
		}
	}

	/**
	 * @brief													: 反向绘制直线
	 *
	 * @param[in]		distance								: 长度
	 */
	void backward(int distance)
	{
		forward(-distance);
	}

	/**
	 * @brief													: 右转
	 *
	 * @param[in]		degrees									: 角度
	 */
	void right(float degrees)
	{
		angle -= degrees * (PI / 180.0F);
		normalizeAngle();
	}

	/**
	 * @brief													: 左转
	 *
	 * @param[in]		degrees									: 角度
	 */
	void left(float degrees)
	{
		angle += degrees * (PI / 180.0F);
		normalizeAngle();
	}

	/**
	 * @brief													: 移动到指定位置（不绘制）
	 *
	 * @param[in]		x										: X坐标
	 * @param[in]		y										: Y坐标
	 */
	void moveTo(int x, int y)
	{
		bool oldState = penState;
		penState = ICON_PEN_UP;
		position.set_int_x(x);
		position.set_int_y(y);

		// 记录填充路径点（移动也记录路径）
		if (isFilling) {
			fillPoints.push_back(position);
		}

		penState = oldState;
	}

	/**
	 * @brief													: 移动到指定位置并绘制
	 *
	 * @param[in]		x										: X坐标
	 * @param[in]		y										: Y坐标
	 */
	void lineTo(int x, int y)
	{
		POINT_2D startPos = position;
		position.set_int_x(x);
		position.set_int_y(y);

		// 记录填充路径点
		if (isFilling && penState) {
			fillPoints.push_back(position);
		}

		if (penState)
		{
			if (icon)
			{
				drawLineWithIcons(startPos, position);
			}
			else
			{
				HPEN hPen = CreatePen(PS_SOLID, penWidth, penColor);
				HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

				MoveToEx(hdc, startPos.get_int_x(), startPos.get_int_y(), nullptr);
				LineTo(hdc, x, y);

				SelectObject(hdc, hOldPen);
				DeleteObject(hPen);
			}
		}
	}

	/**
	 * @brief													: 绘制圆弧
	 * @details													: 以当前位置为切点绘制圆弧，与标准图形库行为一致
	 *
	 * @param[in]		radius									: 半径
	 * @param[in]		angleDegrees							: 角度（度数）
	 * @param[in]		direction								: 方向（CW顺时针，CCW逆时针）
	 */
	void drawArc(int radius, float angleDegrees, float direction = CW)
	{
		if (radius == 0 || angleDegrees == 0) return;

		const float radF = static_cast<float>(radius);
		const float angleRad = fabsf(angleDegrees) * (PI / 180.0f);

		// 修复：圆心总是在光标的左侧（如果半径为正）或右侧（如果半径为负）
		float centerX = position.get_float_x();
		float centerY = position.get_float_y();

		if (radius > 0) {
			centerX += fabsf(radF) * cosf(angle + PI / 2.0f);  // 圆心在左侧
			centerY += fabsf(radF) * sinf(angle + PI / 2.0f);
		}
		else {
			centerX += fabsf(radF) * cosf(angle - PI / 2.0f);  // 圆心在右侧  
			centerY += fabsf(radF) * sinf(angle - PI / 2.0f);
		}

		// 计算从圆心到当前位置的起始角度
		float startAngle = atan2f(position.get_float_y() - centerY,
			position.get_float_x() - centerX);

		// 计算步数
		const float arcLength = fabsf(radF) * angleRad;
		const int steps = static_cast<int>(arcLength / sensitivity) + 1;
		const float angleStep = (direction == CW ? angleRad : -angleRad) / steps;

		POINT_2D lastPoint = position;

		for (int i = 1; i <= steps; i++)
		{
			float currentAngle = startAngle + i * angleStep;
			POINT_2D point;
			point.set_float_x(centerX + fabsf(radF) * cosf(currentAngle));
			point.set_float_y(centerY + fabsf(radF) * sinf(currentAngle));

			// 记录填充路径点
			if (isFilling && penState) {
				fillPoints.push_back(point);
			}

			if (penState)
			{
				if (icon)
				{
					drawLineWithIcons(lastPoint, point);
				}
				else
				{
					HPEN hPen = CreatePen(PS_SOLID, penWidth, penColor);
					HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

					MoveToEx(hdc, lastPoint.get_int_x(), lastPoint.get_int_y(), nullptr);
					LineTo(hdc, point.get_int_x(), point.get_int_y());

					SelectObject(hdc, hOldPen);
					DeleteObject(hPen);
				}
			}

			lastPoint = point;
		}

		// 与标准图形库一致：圆弧结束后自动更新为切线方向
		position = lastPoint;
		float endAngle = startAngle + (direction == CW ? angleRad : -angleRad);
		if (radius > 0) {
			angle = endAngle + PI / 2.0f;  // 正半径：切线方向
		}
		else {
			angle = endAngle - PI / 2.0f;  // 负半径：切线方向
		}
		normalizeAngle();
	}

	/**
	 * @brief													: 绘制圆
	 *
	 * @param[in]		radius									: 半径
	 */
	void drawCircle(int radius)
	{
		drawArc(radius, 360, CW);
	}

	/**
	 * @brief													: 绘制矩形
	 *
	 * @param[in]		width									: 宽度
	 * @param[in]		height									: 高度
	 */
	void drawRectangle(int width, int height)
	{
		POINT_2D startPos = position;

		for (int i = 0; i < 4; i++)
		{
			forward(i % 2 == 0 ? width : height);
			right(90);
		}

		position = startPos;
	}

	/**
	 * @brief													: 绘制多边形
	 *
	 * @param[in]		sides									: 边数
	 * @param[in]		length									: 边长
	 */
	void drawPolygon(int sides, int length)
	{
		if (sides < 3) return;

		float angleStep = 360.0f / sides;
		POINT_2D startPos = position;

		for (int i = 0; i < sides; i++)
		{
			forward(length);
			right(angleStep);
		}

		position = startPos;
	}

	/**
	 * @brief													: 绘制文本
	 * @details													: 在当前位置绘制文本
	 *
	 * @param[in]		text									: 文本内容
	 * @param[in]		fontSize								: 字体大小
	 * @param[in]		textColor								: 文本颜色
	 * @param[in]		fontName								: 字体名称（默认为Arial）
	 */
	void drawText(const std::string& text, int fontSize, COLORREF textColor, const std::string& fontName = "Arial")
	{
		// 创建字体
		evgdi::win::unique_hfont font(CreateFontA(
			-fontSize, 0, 0, 0, FW_NORMAL,
			FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
			fontName.c_str()
		));

		if (!font) return;

		// 选择字体到设备上下文
		evgdi::win::select_object_guard fontSel(hdc, font.get());

		// 设置文本颜色和背景模式
		SetTextColor(hdc, textColor);
		SetBkMode(hdc, TRANSPARENT);

		// 绘制文本
		TextOutA(hdc, position.get_int_x(), position.get_int_y(), text.c_str(), static_cast<int>(text.length()));

		// 字体会在 RAII 析构时自动释放；SelectObject 也会自动还原
	}

private:
	/**
	 * @brief													: 使用图标绘制直线
	 */
	void drawLineWithIcons(const POINT_2D& start, const POINT_2D& end)
	{
		if (!icon) return;

		float dx = end.get_float_x() - start.get_float_x();
		float dy = end.get_float_y() - start.get_float_y();
		float distance = sqrtf(dx * dx + dy * dy);

		if (distance == 0) return;

		int steps = static_cast<int>(distance / sensitivity) + 1;

		for (int i = 0; i <= steps; i++)
		{
			float t = static_cast<float>(i) / steps;
			POINT_2D point;
			point.set_float_x(start.get_float_x() + dx * t);
			point.set_float_y(start.get_float_y() + dy * t);

			DrawIcon(point.get_int_x(), point.get_int_y());
		}
	}

	/**
	 * @brief													: 规范化角度到 [0, 2π) 范围
	 */
	void normalizeAngle()
	{
		while (angle >= 2.0f * PI) angle -= 2.0f * PI;
		while (angle < 0) angle += 2.0f * PI;
	}

public:
	// Getter 和 Setter 方法
	int getStartPosX() const { return position.get_int_x(); }
	int getStartPosY() const { return position.get_int_y(); }

	void setStartPos(int newX, int newY)
	{
		position.set_int_x(newX);
		position.set_int_y(newY);
	}

	void setPenColor(COLORREF color) { penColor = color; }
	void setPenWidth(int width) { penWidth = width; }
	void setSensitivity(int newSens) { sensitivity = (newSens > 0) ? newSens : 1; }
	void setPenSpeed(int newPSpeed) { penSpeed = (newPSpeed >= 0) ? newPSpeed : 0; }
	void setPenStatus(bool newPStatus) { penState = newPStatus; }
	void changeIcon(HICON newIcon) { icon = newIcon; }

	float getAngle() const { return angle * 180.0f / PI; } // 返回角度制
	void setAngle(float degrees)
	{
		angle = degrees * (PI / 180.0f);
		normalizeAngle();
	}

	/**
	 * @brief													: 设置填充密度
	 * @param[in]		density									: 填充密度（图标间距）
	 */
	void setFillDensity(int density) {
		fillDensity = (density > 0) ? density : 15;
	}

	/**
	 * @brief													: 获取填充密度
	 * @return													: 当前填充密度
	 */
	int getFillDensity() const {
		return fillDensity;
	}

	/**
	 * @brief													: 绘制图标
	 */
	void DrawIcon(int x, int y) const
	{
		if (penSpeed > 0)
			Sleep(penSpeed);

		if (!icon) return;

		ICONINFO info{};
		if (!GetIconInfo(icon, &info))
			return;

		BITMAP bmp{};
		if (!GetObjectA(info.hbmMask, sizeof(bmp), &bmp))
		{
			DeleteObject(info.hbmMask);
			if (info.hbmColor) DeleteObject(info.hbmColor);
			return;
		}

		int w = bmp.bmWidth;
		int h = info.hbmColor ? bmp.bmHeight : bmp.bmHeight / 2;

		DrawIconEx(hdc, x - w / 2, y - h / 2, icon, w, h, 0, nullptr, DI_NORMAL);

		DeleteObject(info.hbmMask);
		if (info.hbmColor) DeleteObject(info.hbmColor);
	}

	/**
	 * @brief													: 清除画布
	 */
	void clearCanvas() const
	{
		const RECT rect = canvasRect_();
		HBRUSH hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
		FillRect(hdc, &rect, hBrush);
	}

	/**
	 * @brief													: 获取填充状态
	 * @return													: 是否正在填充
	 */
	bool getFillStatus() const { return isFilling; }
};

// 兼容旧代码：之前的类名叫 IconDrawer
using IconDrawer = Pen;

/**
 * @brief														: 加载图标资源
 */
inline HICON loadCustomIcon(HINSTANCE hInstance, LPCTSTR resourceName)
{
	return LoadIcon(hInstance, resourceName);
}

#endif // DRAW_HPP
