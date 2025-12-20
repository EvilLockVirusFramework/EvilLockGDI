/*****************************************************************//**
 * @file				color.hpp
 * @brief				颜色模型定义与调用
 * 
 * @author				EvilLockVirusFramework
 * @date				2024-01-06
 *********************************************************************/



#ifndef COLOR_HPP
#define COLOR_HPP



#pragma once
#include "common.hpp"



/**
 * @brief														: RGB 颜色模型
 */
struct _RGBQUAD {
	union {
		COLORREF rgb;											///< RGB 颜色
		struct {
			BYTE b;												///< Blue 值
			BYTE g;												///< Green 值
			BYTE r;												///< Red 值
			BYTE unused;										///< 保留
		};
	};
};
using PRGBQUAD = _RGBQUAD*;



/**
 * @brief														: HSL 颜色模型
 */
struct HSLQUAD{
	float h;													///< 色相
	float s;													///< 饱和度
	float l;													///< 亮度
};



/**
 * @brief														: HSV 颜色模型
 */
struct HSVQUAD{
	float h;													///< 色相
	float s;													///< 饱和度
	float v;													///< 明度
};



/**
 * @brief														: RGB 颜色模型转 HSL 颜色模型
 * @return														: 目标 HSL
 * 
 * @param[in]			rgb										: 源 RGB
 */
[[nodiscard]] inline HSLQUAD RGBToHSL(_RGBQUAD rgb)
{
	const FLOAT r = rgb.r / 255.0f;
	const FLOAT g = rgb.g / 255.0f;
	const FLOAT b = rgb.b / 255.0f;

	const FLOAT maxC = (std::max)({ r,g,b });
	const FLOAT minC = (std::min)({ r,g,b });
	const FLOAT delta = maxC - minC;

	HSLQUAD hsl{};
	hsl.l = (maxC + minC) * 0.5f;

	if (delta <= 0.0f)
	{
		hsl.h = 0.0f;
		hsl.s = 0.0f;
	}
	else
	{
		const FLOAT c = maxC + minC;
		const FLOAT denom = (hsl.l < 0.5f) ? c : (2.0f - c);
		hsl.s = delta / denom;
		FLOAT h = 0.0f;
		if (maxC == r)
			h = (g - b) / delta + (g < b ? 6.0f : 0.0f);
		else if (maxC == g)
			h = (b - r) / delta + 2.0f;
		else
			h = (r - g) / delta + 4.0f;
		hsl.h = h * (1.0f / 6.0f);
	}
	return hsl;
}



/**
 * @brief														: HSL 颜色模型转 RGB 颜色模型
 * @return														: 目标 RGB
 *
 * @param[in]			hsl										: 源 HSL
 */
[[nodiscard]] inline _RGBQUAD HSLToRGB(HSLQUAD hsl)
{
	const FLOAT h = hsl.h;
	const FLOAT s = hsl.s;
	const FLOAT l = hsl.l;

	FLOAT r = l, g = l, b = l;

	if (s > 0.0f)
	{
		const FLOAT tmp = l * (1.0f + s);
		const FLOAT v = (l <= 0.5f) ? (tmp) : (l + s - l * s);
		const FLOAT m = 2.0f * l - v;
		const FLOAT sv = (v - m) / v;

		FLOAT h6 = h * 6.0f;
		if (h6 < 0.0f) h6 += 6.0f;
		const INT   sextant = (INT)h6;
		const FLOAT fract = h6 - sextant;

		const FLOAT vsf = v * sv * fract;
		const FLOAT mid1 = m + vsf;
		const FLOAT mid2 = v - vsf;

		switch (sextant)
		{
			case 0: r = v;  g = mid1; b = m;    break;
			case 1: r = mid2; g = v;  b = m;    break;
			case 2: r = m;  g = v;  b = mid1;   break;
			case 3: r = m;  g = mid2; b = v;    break;
			case 4: r = mid1; g = m;  b = v;    break;
			case 5: r = v;  g = m;  b = mid2;   break;
		}
	}
	
	_RGBQUAD rgb{};
	rgb.r = (BYTE)(r * 255.0f + 0.5f);
	rgb.g = (BYTE)(g * 255.0f + 0.5f);
	rgb.b = (BYTE)(b * 255.0f + 0.5f);
	rgb.unused = 0;
	return rgb;
}



/**
 * @brief														: RGB 颜色模型转 HSV 颜色模型
 * @return														: 目标 HSV
 *
 * @param[in]			rgb										: 源 RGB
 */
[[nodiscard]] inline HSVQUAD RGBToHSV(_RGBQUAD rgb) {
	const FLOAT r = rgb.r / 255.f;
	const FLOAT g = rgb.g / 255.f;
	const FLOAT b = rgb.b / 255.f;

	const FLOAT minC = (std::min)({ r, g, b });
	const FLOAT maxC = (std::max)({ r, g, b });
	const FLOAT subC = maxC - minC;

	HSVQUAD hsv{};
	FLOAT h = 0.0f, s = 0.0f, v = 0.0f;
	h = (maxC == minC ? 0 : h);
	h = (maxC == r && g >= b ? h = 60.0f * ((g - b) / subC) : h);
	h = (maxC == r && g < b ? 60.0f * ((g - b) / subC) + 360.0f : h);
	h = (maxC == g ? 60.0f * ((b - r) / subC) + 120.0f : h);
	h = (maxC == b ? 60.0f * ((r - g) / subC) + 240.0f : h);
	s = (maxC == 0.0f ? 0.0f : 1.0f - (minC / maxC));
	v = maxC;

	hsv.h = h / 360.f, hsv.s = s, hsv.v = v;
	return hsv;
}



/**
 * @brief														: HSV 颜色模型转 RGB 颜色模型
 * @return														: 目标 RGB
 *
 * @param[in]			hsv										: 源 HSV
 */
[[nodiscard]] inline _RGBQUAD HSVToRGB(HSVQUAD hsv) {
	FLOAT r = 0.0f, g = 0.0f, b = 0.0f;
	FLOAT h = hsv.h, s = hsv.s, v = hsv.v;
	FLOAT _h = h * 360.f;
	FLOAT f = 0.0f, p = 0.0f, q = 0.0f, t = 0.0f;

	if (s == 0.0f) {
		r = v, g = v, b = v;
	}
	else
	{
		_h = _h / 60.0f;
		f = std::fmod(_h, 1.0f);
		p = v * (1.0f - s);
		q = v * (1.0f - f * s);
		t = v * (1.0f - (1.0f - f) * s);
		switch (static_cast<int>(_h))
		{
			case 0: r = v, g = t, b = p; break;
			case 1: r = q, g = v, b = p; break;
			case 2: r = p, g = v, b = t; break;
			case 3: r = p, g = q, b = v; break;
			case 4: r = t, g = p; b = v; break;
			case 5: r = v, g = p, b = q; break;
		}
	}

	_RGBQUAD rgb{};
	rgb.r = static_cast<BYTE>(r * 255.f + 0.5f);
	rgb.g = static_cast<BYTE>(g * 255.f + 0.5f);
	rgb.b = static_cast<BYTE>(b * 255.f + 0.5f);
	rgb.unused = 0;
	return rgb;
}



#endif															// !COLOR_HPP

