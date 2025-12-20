/*****************************************************************//**
 * @file				common.hpp
 * @brief				包含常用头文件与定义常用宏
 *
 * @author				EvilLockVirusFramework
 * @date				2025-08-28
 *********************************************************************/



#pragma once
#include <variant>
#include <functional>
#include <atomic>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <cmath>
#include <numbers>
#include <iostream>
#include <thread>
#include <source_location>
#include <Windows.h>
#include <tchar.h>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <deque>
#include <list>

// 说明：按你的要求，把“需要链接的系统库”继续放在头文件里。
// - 这是一种 MSVC/Visual Studio 习惯用法：只要包含 common.hpp，就会自动把库加入链接依赖。
// - 如果你以后改用别的构建系统/编译器（例如 clang-cl/CMake），可以在工程里手动添加同名库。
#if defined(_MSC_VER)
#pragma comment(lib, "winmm.lib")   // 多媒体：waveOut 等（bytebeat/音频相关）
#pragma comment(lib, "msimg32.lib") // 图像混合：AlphaBlend/GradientFill/TransparentBlt 等
#endif



/**
 * @brief 圆周率常量（C++20）
 * @details 优先使用标准库 `std::numbers::pi_v<float>`，避免到处手写 PI。
 */
inline constexpr float PI = std::numbers::pi_v<float>;

/**
 * @brief 移动模式（用于窗口/弹窗“撞墙”后的行为）
 */
enum class MoveMode : int {
	Bounce = 1,													///< 自动反弹
	Stop = 2,													///< 停止移动
	Continue = 3												///< 继续移动
};

// 兼容旧代码：保留原来的整型常量名
inline constexpr int BOUNCE = static_cast<int>(MoveMode::Bounce);
inline constexpr int STOP = static_cast<int>(MoveMode::Stop);
inline constexpr int CONTINUE = static_cast<int>(MoveMode::Continue);



/**
 * @brief														: 二维坐标结构
 */
struct POINT_2D {
	/**
	 * @brief x/y 坐标（统一使用 float，既能画整数点，也能画小数点）
	 * @details
	 * 旧版用 `std::variant<int,float>` 存两种类型，理解成本高、也容易写出多余代码。
	 * 对“画图”来说，用 float 最直观：需要整数时再取整即可。
	 */
	float x = 0.0f;
	float y = 0.0f;

	int   get_int_x() const noexcept { return static_cast<int>(x); }
	int   get_int_y() const noexcept { return static_cast<int>(y); }

	float get_float_x() const noexcept { return x; }
	float get_float_y() const noexcept { return y; }

	void  set_int_x(int v) noexcept { x = static_cast<float>(v); }
	void  set_int_y(int v) noexcept { y = static_cast<float>(v); }

	void  set_float_x(float v) noexcept { x = v; }
	void  set_float_y(float v) noexcept { y = v; }

};

// 添加比较运算符
inline bool operator==(const POINT_2D& a, const POINT_2D& b) {
	return a.get_int_x() == b.get_int_x() && a.get_int_y() == b.get_int_y();
}

inline bool operator!=(const POINT_2D& a, const POINT_2D& b) {
	return !(a == b);
}

inline bool operator<(const POINT_2D& a, const POINT_2D& b) {
	if (a.get_int_x() != b.get_int_x())
		return a.get_int_x() < b.get_int_x();
	return a.get_int_y() < b.get_int_y();
}

inline bool operator>(const POINT_2D& a, const POINT_2D& b) {
	return b < a;
}

inline bool operator<=(const POINT_2D& a, const POINT_2D& b) {
	return !(b < a);
}

inline bool operator>=(const POINT_2D& a, const POINT_2D& b) {
	return !(a < b);
}



/**
 * @brief														: 三维坐标结构
 */
struct POINT_3D {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	int   get_int_x() const noexcept { return static_cast<int>(x); }
	int   get_int_y() const noexcept { return static_cast<int>(y); }
	int   get_int_z() const noexcept { return static_cast<int>(z); }

	float get_float_x() const noexcept { return x; }
	float get_float_y() const noexcept { return y; }
	float get_float_z() const noexcept { return z; }

	void  set_int_x(int v) noexcept { x = static_cast<float>(v); }
	void  set_int_y(int v) noexcept { y = static_cast<float>(v); }
	void  set_int_z(int v) noexcept { z = static_cast<float>(v); }

	void  set_float_x(float v) noexcept { x = v; }
	void  set_float_y(float v) noexcept { y = v; }
	void  set_float_z(float v) noexcept { z = v; }
};

// 添加POINT_3D比较运算符
inline bool operator==(const POINT_3D& a, const POINT_3D& b) {
	return a.get_int_x() == b.get_int_x() &&
		a.get_int_y() == b.get_int_y() &&
		a.get_int_z() == b.get_int_z();
}

inline bool operator!=(const POINT_3D& a, const POINT_3D& b) {
	return !(a == b);
}

inline bool operator<(const POINT_3D& a, const POINT_3D& b) {
	if (a.get_int_x() != b.get_int_x())
		return a.get_int_x() < b.get_int_x();
	if (a.get_int_y() != b.get_int_y())
		return a.get_int_y() < b.get_int_y();
	return a.get_int_z() < b.get_int_z();
}



/**
 * @brief														: 抛出带有源代码位置信息的运行时错误
 *
 * @param[in]			msg										: 错误信息
 * @param[in]			loc										: 源代码位置信息
 */
[[noreturn]] inline void throw_runtime_error(const std::string& msg,
	const std::source_location& loc =
	std::source_location::current())
{
	throw std::runtime_error(
		std::string(loc.file_name()) + ":" +
		std::to_string(loc.line()) + " " + msg
	);
}
