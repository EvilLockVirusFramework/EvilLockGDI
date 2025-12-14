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
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "msimg32.lib")



constexpr float PI = 3.1415927F;
constexpr int BOUNCE = 1;										///< 自动反弹
constexpr int STOP = 2;											///< 停止移动
constexpr int CONTINUE = 3;										///< 继续移动



/**
 * @brief														: 二维坐标结构
 */
struct POINT_2D {
	std::variant<int, float> x;
	std::variant<int, float> y;


	int		get_int_x()   const { return std::visit([](auto v) { return static_cast<int>(v); }, x); }
	int		get_int_y()   const { return std::visit([](auto v) { return static_cast<int>(v); }, y); }

	float   get_float_x() const { return std::visit([](auto v) { return static_cast<float>(v); }, x); }
	float   get_float_y() const { return std::visit([](auto v) { return static_cast<float>(v); }, y); }


	void	set_int_x(int v) { x = v; }
	void	set_int_y(int v) { y = v; }

	void	set_float_x(float v) { x = v; }
	void	set_float_y(float v) { y = v; }

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
	std::variant<int, float> x;
	std::variant<int, float> y;
	std::variant<int, float> z;

	int		get_int_x()   const { return std::visit([](auto v) { return static_cast<int>(v); }, x); }
	int		get_int_y()   const { return std::visit([](auto v) { return static_cast<int>(v); }, y); }
	int		get_int_z()   const { return std::visit([](auto v) { return static_cast<int>(v); }, z); }

	float   get_float_x() const { return std::visit([](auto v) { return static_cast<float>(v); }, x); }
	float   get_float_y() const { return std::visit([](auto v) { return static_cast<float>(v); }, y); }
	float   get_float_z() const { return std::visit([](auto v) { return static_cast<float>(v); }, z); }


	void	set_int_x(int v) { x = v; }
	void	set_int_y(int v) { y = v; }
	void	set_int_z(int v) { z = v; }

	void	set_float_x(float v) { x = v; }
	void	set_float_y(float v) { y = v; }
	void	set_float_z(float v) { z = v; }
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
[[noreturn]] void throw_runtime_error(const std::string& msg,
	const std::source_location& loc =
	std::source_location::current())
{
	throw std::runtime_error(
		std::string(loc.file_name()) + ":" +
		std::to_string(loc.line()) + " " + msg
	);
}