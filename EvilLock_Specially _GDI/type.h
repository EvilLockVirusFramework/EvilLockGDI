//type.h
#include<Windows.h>
#ifndef TYPE_H
#define TYPE_H
#define UP 0
#define DOWN 1
#define PI 3.14159
typedef union _RGBQUAD {
	COLORREF rgb;
	struct {
		BYTE b;
		BYTE g;
		BYTE r;
		BYTE unused;
	};
} *PRGBQUAD;
typedef struct {
	float h;
	float s;
	float l;
} HSLQUAD;
typedef struct {
	float h;
	float s;
	float v;
} HSVQUAD;
class GDI {
public:
    operator HDC() const {
        return GetDC(NULL);
    }

    ~GDI() {
        ReleaseDC(NULL, hdc_);
    }
private:
    HDC hdc_ = GetDC(NULL);
};
typedef int GDIState;        //UP or DOWN
typedef struct { float x, y; } Point; //位置
typedef struct
{
	HDC hdcMem;//虚拟内存hdc
	PRGBQUAD RGBScreen;//虚拟内存hdc的rgb
}MEMGDI;
typedef struct {
    GDI screen;//实际的设备上下文
	MEMGDI MemGDI;//虚拟的内存设备上下文
    double heading;	//方向
    GDIState state; //状态
    Point Pos; //桌面中心当前位置
} SPGDI;
HSLQUAD RGBToHSL(_RGBQUAD rgb) {
	HSLQUAD hsl;
	BYTE r = rgb.r, g = rgb.g, b = rgb.b;
	FLOAT _r = (FLOAT)r / 255.f, _g = (FLOAT)g / 255.f, _b = (FLOAT)b / 255.f;
	FLOAT rgbMin = min(min(_r, _g), _b), rgbMax = max(max(_r, _g), _b);
	FLOAT fDelta = rgbMax - rgbMin;
	FLOAT deltaR, deltaG, deltaB;
	FLOAT h = 0.f, s = 0.f, l = (FLOAT)((rgbMax + rgbMin) / 2.f);
	if (fDelta != 0.f) {
		s = l < .5f ? (FLOAT)(fDelta / (rgbMax + rgbMin)) : (FLOAT)(fDelta / (2.f - rgbMax - rgbMin));
		deltaR = (FLOAT)(((rgbMax - _r) / 6.f + (fDelta / 2.f)) / fDelta), deltaG = (FLOAT)(((rgbMax - _g) / 6.f + (fDelta / 2.f)) / fDelta), deltaB = (FLOAT)(((rgbMax - _b) / 6.f + (fDelta / 2.f)) / fDelta);

		if (_r == rgbMax) {
			h = deltaB - deltaG;
		}
		else if (_g == rgbMax) {
			h = (1.f / 3.f) + deltaR - deltaB;
		}
		else if (_b == rgbMax) {
			h = (2.f / 3.f) + deltaG - deltaR;
		}

		if (h < 0.f) {
			h += 1.f;
		}
		if (h > 1.f) {
			h -= 1.f;
		}
	}
	hsl.h = h, hsl.s = s, hsl.l = l;
	return hsl;
}
_RGBQUAD HSLToRGB(HSLQUAD hsl) {
	_RGBQUAD rgb;
	FLOAT r = hsl.l, g = hsl.l, b = hsl.l;
	FLOAT h = hsl.h, sl = hsl.s, l = hsl.l;
	FLOAT v = (l <= .5f) ? (l * (1.f + sl)) : (l + sl - l * sl);
	FLOAT m, sv, fract, vsf, mid1, mid2;
	INT sextant;
	if (v > 0.f) {
		m = l + l - v;
		sv = (v - m) / v;
		h *= 6.f;
		sextant = (INT)h;
		fract = h - sextant;
		vsf = v * sv * fract;
		mid1 = m + vsf, mid2 = v - vsf;

		switch (sextant) {
		case 0:
			r = v, g = mid1, b = m;
			break;
		case 1:
			r = mid2, g = v, b = m;
			break;
		case 2:
			r = m, g = v, b = mid1;
			break;
		case 3:
			r = m, g = mid2, b = v;
			break;
		case 4:
			r = mid1, g = m, b = v;
			break;
		case 5:
			r = v, g = m, b = mid2;
			break;
		}
	}
	rgb.r = (BYTE)(r * 255.f), rgb.g = (BYTE)(g * 255.f), rgb.b = (BYTE)(b * 255.f);
	return rgb;
}
HSVQUAD RGBToHSV(_RGBQUAD rgb) {
	HSVQUAD hsv;
	FLOAT h = 0, s = 0, v = 0;
	FLOAT r = rgb.r / 255.f, g = rgb.g / 255.f, b = rgb.b / 255.f;
	FLOAT rgbMin = min(r, min(g, b)), rgbMax = max(r, max(g, b));

	h = (rgbMax == rgbMin ? 0 : h);
	h = (rgbMax == r && g >= b ? h = 60 * ((g - b) / (rgbMax - rgbMin)) : h);
	h = (rgbMax == r && g < b ? 60 * ((g - b) / (rgbMax - rgbMin)) + 360 : h);
	h = (rgbMax == g ? 60 * ((b - r) / (rgbMax - rgbMin)) + 120 : h);
	h = (rgbMax == b ? 60 * ((r - g) / (rgbMax - rgbMin)) + 240 : h);
	s = (rgbMax == 0 ? 0 : 1 - (rgbMin / rgbMax));
	v = rgbMax;

	hsv.h = h / 360.f;
	hsv.s = s;
	hsv.v = v;
	return hsv;
}
_RGBQUAD HSVToRGB(HSVQUAD hsv) {
	_RGBQUAD rgb;
	FLOAT r = 0, g = 0, b = 0;
	FLOAT h = hsv.h, s = hsv.s, v = hsv.v;
	FLOAT _h = h * 360.f;
	FLOAT f = 0, p = 0, q = 0, t = 0;
	if (s == 0) {
		r = v, g = v, b = v;
	}
	else {
		_h = _h / 60;
		f = _h - (int)_h;
		p = v * (1 - s);
		q = v * (1 - f * s);
		t = v * (1 - (1 - f) * s);
		switch ((int)_h)
		{
		case 0:
			r = v, g = t, b = p;
			break;
		case 1:
			r = q, g = v, b = p;
			break;
		case 2:
			r = p, g = v, b = t;
			break;
		case 3:
			r = p, g = q, b = v;
			break;
		case 4:
			r = t, g = p; b = v;
			break;
		case 5:
			r = v, g = p, b = q;
			break;
		}
	}
	rgb.r = (unsigned char)(r * 255.f + 0.5f), rgb.g = (unsigned char)(g * 255.f + 0.5f), rgb.b = (unsigned char)(b * 255.f + 0.5f);
	return rgb;
}
#endif