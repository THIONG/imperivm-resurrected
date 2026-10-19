// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#define NOMINMAX
#include <windows.h>
#include <cmath>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "i18n/tr.h"

namespace ui {

struct Hit {
	RECT r;
	int action;
	int arg;
};

inline std::wstring W(const std::string &s)
{
	if (s.empty()) return L"";
	int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), -1, nullptr, 0);
	UINT cp = n > 0 ? CP_UTF8 : CP_ACP;
	DWORD flags = n > 0 ? MB_ERR_INVALID_CHARS : 0;
	n = MultiByteToWideChar(cp, flags, s.c_str(), -1, nullptr, 0);
	std::wstring w(n > 0 ? n - 1 : 0, L'\0');
	if (n > 1) MultiByteToWideChar(cp, flags, s.c_str(), -1, &w[0], n);
	return w;
}

struct Painter {
	HDC dc;
	float s;
	int ox, oy;
	std::vector<Hit> *hits;
	HFONT fonts[5] = {};
	int pressAction = -1, pressArg = 0;
	int focusAction = -1, focusArg = 0;
	bool noHover = false;

	int S(int v) const { return static_cast<int>(v * s + 0.5f); }
	RECT R(int x, int y, int w, int h) const { return {S(x), S(y), S(x + w), S(y + h)}; }

	void Font(int i) { SelectObject(dc, fonts[i]); }
	void Fill(int x, int y, int w, int h, COLORREF c)
	{
		RECT r = R(x, y, w, h);
		HBRUSH b = CreateSolidBrush(c);
		FillRect(dc, &r, b);
		DeleteObject(b);
	}
	void ArrowUp(int x, int y, int w, int h, COLORREF c)
	{
		const RECT r = R(x, y, w, h);
		POINT pts[3] = {{r.left, r.bottom}, {r.right, r.bottom}, {(r.left + r.right) / 2, r.top}};
		HBRUSH b = CreateSolidBrush(c);
		HPEN pen = CreatePen(PS_SOLID, 1, c);
		HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, pen);
		Polygon(dc, pts, 3);
		SelectObject(dc, ob);
		SelectObject(dc, op);
		DeleteObject(b);
		DeleteObject(pen);
	}
	void ArrowRight(int x, int y, int w, int h, COLORREF c)
	{
		const RECT r = R(x, y, w, h);
		POINT pts[3] = {{r.left, r.top}, {r.left, r.bottom}, {r.right, (r.top + r.bottom) / 2}};
		HBRUSH b = CreateSolidBrush(c);
		HPEN pen = CreatePen(PS_SOLID, 1, c);
		HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, pen);
		Polygon(dc, pts, 3);
		SelectObject(dc, ob);
		SelectObject(dc, op);
		DeleteObject(b);
		DeleteObject(pen);
	}
	void ArrowLeft(int x, int y, int w, int h, COLORREF c)
	{
		const RECT r = R(x, y, w, h);
		POINT pts[3] = {{r.right, r.top}, {r.right, r.bottom}, {r.left, (r.top + r.bottom) / 2}};
		HBRUSH b = CreateSolidBrush(c);
		HPEN pen = CreatePen(PS_SOLID, 1, c);
		HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, pen);
		Polygon(dc, pts, 3);
		SelectObject(dc, ob);
		SelectObject(dc, op);
		DeleteObject(b);
		DeleteObject(pen);
	}
	void Spinner(int cx, int cy, int radius, int dot, DWORD tick)
	{
		const int n = 12;
		const int head = static_cast<int>((tick / 70) % n);
		for (int i = 0; i < n; i++) {
			const double a = i * 6.283185307 / n - 1.570796327;
			const int px = cx + static_cast<int>(std::lround(std::cos(a) * radius)), py = cy + static_cast<int>(std::lround(std::sin(a) * radius));
			const int k = std::max(0, 255 - ((head - i + n) % n) * 22);
			Ellipse(px - dot / 2, py - dot / 2, dot, dot, RGB(43 + (214 - 43) * k / 255, 34 + (170 - 34) * k / 255, 20 + (84 - 20) * k / 255));
		}
	}
	void ArrowDown(int x, int y, int w, int h, COLORREF c)
	{
		const RECT r = R(x, y, w, h);
		POINT pts[3] = {{r.left, r.top}, {r.right, r.top}, {(r.left + r.right) / 2, r.bottom}};
		HBRUSH b = CreateSolidBrush(c);
		HPEN pen = CreatePen(PS_SOLID, 1, c);
		HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, pen);
		Polygon(dc, pts, 3);
		SelectObject(dc, ob);
		SelectObject(dc, op);
		DeleteObject(b);
		DeleteObject(pen);
	}
	void Line(int x1, int y1, int x2, int y2, COLORREF c, int width = 1)
	{
		LOGBRUSH lb = {BS_SOLID, c, 0};
		HPEN pen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT, std::max(1, S(width)), &lb, 0, nullptr);
		HGDIOBJ old = SelectObject(dc, pen);
		MoveToEx(dc, S(x1), S(y1), nullptr);
		LineTo(dc, S(x2), S(y2));
		SelectObject(dc, old);
		DeleteObject(pen);
	}
	void Ellipse(int x, int y, int w, int h, COLORREF c)
	{
		HBRUSH b = CreateSolidBrush(c);
		HPEN pen = CreatePen(PS_SOLID, 1, c);
		HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, pen);
		const RECT r = R(x, y, w, h);
		::Ellipse(dc, r.left, r.top, r.right, r.bottom);
		SelectObject(dc, ob);
		SelectObject(dc, op);
		DeleteObject(b);
		DeleteObject(pen);
	}
	void RoundRect(int x, int y, int w, int h, int radius, COLORREF c)
	{
		HBRUSH b = CreateSolidBrush(c);
		HPEN pen = CreatePen(PS_SOLID, 1, c);
		HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, pen);
		const RECT r = R(x, y, w, h);
		::RoundRect(dc, r.left, r.top, r.right, r.bottom, S(radius), S(radius));
		SelectObject(dc, ob);
		SelectObject(dc, op);
		DeleteObject(b);
		DeleteObject(pen);
	}
	void Frame(int x, int y, int w, int h, COLORREF c)
	{
		Fill(x, y, w, 1, c);
		Fill(x, y + h - 1, w, 1, c);
		Fill(x, y, 1, h, c);
		Fill(x + w - 1, y, 1, h, c);
	}
	void Image(int x, int y, int w, int h, const uint32_t *bgra, int iw, int ih)
	{
		if (!bgra || iw <= 0 || ih <= 0) return;
		const RECT r = R(x, y, w, h);
		BITMAPINFO bi = {};
		bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
		bi.bmiHeader.biWidth = iw;
		bi.bmiHeader.biHeight = -ih;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biBitCount = 32;
		bi.bmiHeader.biCompression = BI_RGB;
		const int old = SetStretchBltMode(dc, HALFTONE);
		SetBrushOrgEx(dc, 0, 0, nullptr);
		StretchDIBits(dc, r.left, r.top, r.right - r.left, r.bottom - r.top, 0, 0, iw, ih, bgra, &bi, DIB_RGB_COLORS, SRCCOPY);
		SetStretchBltMode(dc, old);
	}
	void Text(int x, int y, int w, int h, const std::string &t, COLORREF c, UINT fmt = DT_LEFT | DT_VCENTER | DT_SINGLELINE)
	{
		RECT r = R(x, y, w, h);
		SetTextColor(dc, c);
		std::wstring ws = W(Tr(t));
		DrawTextW(dc, ws.c_str(), static_cast<int>(ws.size()), &r, fmt | DT_NOPREFIX | DT_END_ELLIPSIS);
	}
	int TextWrapped(int x, int y, int w, int h, const std::string &t, COLORREF c)
	{
		RECT r = R(x, y, w, h);
		SetTextColor(dc, c);
		std::wstring ws = W(Tr(t));
		RECT calc = r;
		DrawTextW(dc, ws.c_str(), static_cast<int>(ws.size()), &calc, DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
		DrawTextW(dc, ws.c_str(), static_cast<int>(ws.size()), &r, DT_WORDBREAK | DT_NOPREFIX | DT_END_ELLIPSIS);
		return static_cast<int>((std::min(calc.bottom, r.bottom) - r.top) / s);
	}
	int Measure(int w, const std::string &t)
	{
		RECT r = {0, 0, S(w), 0};
		std::wstring ws = W(Tr(t));
		DrawTextW(dc, ws.c_str(), static_cast<int>(ws.size()), &r, DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
		return static_cast<int>((r.bottom - r.top) / s + 0.99f);
	}
	int TextWidth(const std::string &t)
	{
		const std::wstring ws = W(Tr(t));
		SIZE sz = {};
		GetTextExtentPoint32W(dc, ws.c_str(), static_cast<int>(ws.size()), &sz);
		return static_cast<int>(sz.cx / s + 0.99f);
	}
	void Clip(int x, int y, int w, int h)
	{
		const RECT r = R(x, y, w, h);
		HRGN g = CreateRectRgn(r.left, r.top, r.right, r.bottom);
		SelectClipRgn(dc, g);
		DeleteObject(g);
	}
	void Unclip() { SelectClipRgn(dc, nullptr); }
	void AddHit(int x, int y, int w, int h, int action, int arg = 0)
	{
		RECT r = R(x, y, w, h);
		OffsetRect(&r, ox, oy);
		hits->push_back({r, action, arg});
		if (action >= 0 && action == focusAction && arg == focusArg) {
			Frame(x, y, w, h, RGB(255, 236, 170));
			Frame(x + 1, y + 1, w - 2, h - 2, RGB(255, 236, 170));
		}
	}
	bool Hover(int x, int y, int w, int h, int mx, int my) const
	{
		if (noHover) return false;
		RECT r = R(x, y, w, h);
		OffsetRect(&r, ox, oy);
		POINT p = {mx, my};
		return PtInRect(&r, p) != 0;
	}
	static const int kToggleW = 26, kToggleH = 13;
	void Toggle(int x, int y, bool on, bool enabled, bool hover)
	{
		const COLORREF track = on ? (enabled ? RGB(122, 92, 36) : RGB(84, 66, 36)) : RGB(36, 29, 20);
		const COLORREF edge = !enabled ? RGB(90, 76, 56) : hover ? RGB(255, 236, 170) : on ? RGB(214, 170, 84) : RGB(120, 96, 60);
		Fill(x, y, kToggleW, kToggleH, track);
		Frame(x, y, kToggleW, kToggleH, edge);
		const int kx = on ? x + kToggleW - kToggleH + 2 : x + 2;
		Fill(kx, y + 2, kToggleH - 4, kToggleH - 4, on ? (enabled ? RGB(255, 226, 150) : RGB(190, 170, 130)) : RGB(150, 130, 100));
	}
	void Button(int x, int y, int w, int h, const std::string &label, int action, int arg, int mx, int my,
		bool enabled = true, bool strong = false, int font = 1)
	{
		const bool hov = enabled && Hover(x, y, w, h, mx, my);
		const bool down = enabled && action == pressAction && arg == pressArg;
		COLORREF bg = !enabled ? RGB(48, 40, 32)
					: down    ? (strong ? RGB(92, 60, 18) : RGB(38, 30, 20))
					: strong  ? (hov ? RGB(150, 104, 40) : RGB(122, 82, 30))
							  : (hov ? RGB(92, 72, 48) : RGB(66, 52, 36));
		Fill(x, y, w, h, bg);
		Frame(x, y, w, h, !enabled ? RGB(90, 76, 56) : down ? RGB(255, 236, 170) : RGB(200, 160, 80));
		if (down) Fill(x + 1, y + 1, w - 2, 1, RGB(20, 15, 10));
		Font(font);
		Text(x, y + (down ? 1 : 0), w, h, label, enabled ? RGB(240, 226, 180) : RGB(130, 118, 96), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		if (enabled) AddHit(x, y, w, h, action, arg);
	}
};

inline void DrawDiscordIcon(Painter &p, int x, int y, int size)
{
	const COLORREF blurple = RGB(88, 101, 242);
	p.RoundRect(x, y + size / 8, size, size * 3 / 4, size / 3, blurple);
	p.Ellipse(x + size / 6, y + size * 3 / 4 - size / 10, size / 4, size / 4, blurple);
	p.Ellipse(x + size - size / 6 - size / 4, y + size * 3 / 4 - size / 10, size / 4, size / 4, blurple);
	const int eye = size / 5 > 3 ? size / 5 : 3;
	p.Ellipse(x + size / 4, y + size * 3 / 8, eye, eye + 1, RGB(255, 255, 255));
	p.Ellipse(x + size - size / 4 - eye, y + size * 3 / 8, eye, eye + 1, RGB(255, 255, 255));
}

inline std::string HumanBytes(uint64_t b)
{
	char buf[32];
	if (b >= 1024ull * 1024 * 1024) sprintf_s(buf, "%.1f GB", b / 1073741824.0);
	else if (b >= 1024 * 1024) sprintf_s(buf, "%.1f MB", b / 1048576.0);
	else sprintf_s(buf, "%.0f KB", b / 1024.0);
	return buf;
}

inline std::string TransferText(uint64_t done, uint64_t total, double perSecond)
{
	std::string out;
	if (total) out = std::to_string(std::min<uint64_t>(100, done * 100 / total)) + " %  ·  ";
	out += HumanBytes(done) + (total ? Tr(" of ") + HumanBytes(total) : "");
	if (perSecond > 1024) out += "  ·  " + HumanBytes(static_cast<uint64_t>(perSecond)) + "/s";
	return out;
}

const COLORREF kText = RGB(232, 216, 160), kDim = RGB(160, 146, 116), kGold = RGB(214, 170, 84),
			   kBad = RGB(236, 110, 90), kOk = RGB(140, 200, 120);

namespace pal {
constexpr COLORREF panel = RGB(28, 22, 16), dialog = RGB(30, 24, 17), well = RGB(18, 14, 10), wellRaised = RGB(24, 19, 14),
				   shadow = RGB(8, 6, 4), line = RGB(110, 84, 44), lineDim = RGB(70, 54, 34), divider = RGB(90, 70, 40),
				   edge = RGB(150, 116, 60), edgeHot = RGB(230, 196, 120), edgeStrong = RGB(200, 160, 80), focus = RGB(255, 236, 170),
				   field = RGB(34, 27, 19), fieldHover = RGB(58, 46, 32), fieldOpen = RGB(46, 36, 24), rowHover = RGB(66, 52, 36),
				   rowSelected = RGB(96, 70, 30), track = RGB(36, 28, 18), thumb = RGB(150, 116, 60), thumbHot = RGB(230, 196, 120),
				   disabledEdge = RGB(90, 76, 56), disabledText = RGB(130, 118, 96), section = RGB(176, 140, 76);
}

struct Canvas {
	HDC dc = nullptr;
	HBITMAP bmp = nullptr;
	HGDIOBJ old = nullptr;
	uint32_t *bits = nullptr;
	int w = 0, h = 0;
	bool Ensure(int nw, int nh)
	{
		if (dc && nw == w && nh == h) return true;
		Release();
		BITMAPINFO bi = {};
		bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
		bi.bmiHeader.biWidth = nw;
		bi.bmiHeader.biHeight = -nh;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biBitCount = 32;
		dc = CreateCompatibleDC(nullptr);
		bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, reinterpret_cast<void **>(&bits), nullptr, 0);
		if (!dc || !bmp) {
			Release();
			return false;
		}
		old = SelectObject(dc, bmp);
		SetBkMode(dc, TRANSPARENT);
		w = nw;
		h = nh;
		return true;
	}
	void Release()
	{
		if (dc && old) SelectObject(dc, old);
		if (bmp) DeleteObject(bmp);
		if (dc) DeleteDC(dc);
		dc = nullptr;
		bmp = nullptr;
		bits = nullptr;
		w = h = 0;
	}
};

inline HFONT g_fonts[5] = {};
inline float g_fontScale = 0;

inline void EnsureFonts(float s)
{
	if (g_fonts[0] && g_fontScale == s) return;
	for (HFONT &f : g_fonts) {
		if (f) DeleteObject(f);
	}
	auto make = [s](int size, int weight, const wchar_t *face) {
		return CreateFontW(-static_cast<int>(size * s + 0.5f), 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, face);
	};
	g_fonts[0] = make(15, FW_NORMAL, L"Georgia");
	g_fonts[1] = make(15, FW_BOLD, L"Georgia");
	g_fonts[2] = make(22, FW_BOLD, L"Georgia");
	g_fonts[3] = make(13, FW_NORMAL, L"Segoe UI");
	g_fonts[4] = make(13, FW_SEMIBOLD, L"Segoe UI");
	g_fontScale = s;
}

inline void Blend(uint32_t *buf, int w, int h, const Canvas &c, int ox, int oy, int alpha)
{
	for (int y = 0; y < c.h; y++) {
		const int ty = oy + y;
		if (ty < 0 || ty >= h) continue;
		for (int x = 0; x < c.w; x++) {
			const int tx = ox + x;
			if (tx < 0 || tx >= w) continue;
			uint32_t src = c.bits[static_cast<size_t>(y) * c.w + x], &dst = buf[static_cast<size_t>(ty) * w + tx];
			uint32_t rb = ((src & 0xff00ff) * alpha + (dst & 0xff00ff) * (255 - alpha)) >> 8;
			uint32_t g = ((src & 0x00ff00) * alpha + (dst & 0x00ff00) * (255 - alpha)) >> 8;
			dst = 0xff000000u | (rb & 0xff00ff) | (g & 0x00ff00);
		}
	}
}

}
