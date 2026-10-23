// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>
#include <cmath>

#include "ui/modui_internal.h"

using namespace modui_impl;

namespace modui_impl {
ClipBox g_clip;
}

namespace modui_impl {
namespace {
inline void BlendPx(uint32_t &dst, COLORREF c, float a)
{
	if (a <= 0.0f) return;
	if (a > 1.0f) a = 1.0f;
	const int dr = (dst >> 16) & 255, dg = (dst >> 8) & 255, db = dst & 255;
	const int r = static_cast<int>(dr + (GetRValue(c) - dr) * a + 0.5f);
	const int g = static_cast<int>(dg + (GetGValue(c) - dg) * a + 0.5f);
	const int b = static_cast<int>(db + (GetBValue(c) - db) * a + 0.5f);
	dst = 0xff000000u | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
}

float RoundRectDist(float px, float py, float cx, float cy, float hw, float hh, float r)
{
	const float qx = std::fabs(px - cx) - (hw - r), qy = std::fabs(py - cy) - (hh - r);
	const float ox = std::max(qx, 0.0f), oy = std::max(qy, 0.0f);
	return std::sqrt(ox * ox + oy * oy) + std::min(std::max(qx, qy), 0.0f) - r;
}
}
}

namespace modui_impl {
void FillRound(uint32_t *buf, int W, int H, float x, float y, float w, float h, float r, COLORREF c, float a)
{
	const int x0 = std::max(std::max(0, g_clip.x0), static_cast<int>(std::floor(x)) - 1), x1 = std::min(std::min(W, g_clip.x1), static_cast<int>(std::ceil(x + w)) + 1);
	const int y0 = std::max(std::max(0, g_clip.y0), static_cast<int>(std::floor(y)) - 1), y1 = std::min(std::min(H, g_clip.y1), static_cast<int>(std::ceil(y + h)) + 1);
	const float hw = w / 2, hh = h / 2;
	r = std::min(r, std::min(hw, hh));
	for (int j = y0; j < y1; j++) {
		int fullL = x1, fullR = x1;
		const float cy = j + 0.5f;
		if (cy >= y + 1 && cy <= y + h - 1) {
			const float inset = (cy >= y + r && cy <= y + h - r) ? 1.0f : r + 1.0f;
			fullL = std::max(x0, static_cast<int>(std::ceil(x + inset)));
			fullR = std::min(x1, static_cast<int>(std::floor(x + w - inset)));
			if (fullR < fullL) fullL = fullR = x1;
		}
		uint32_t *row = buf + static_cast<size_t>(j) * W;
		for (int i = x0; i < x1; i++) {
			if (i == fullL) {
				for (; i < fullR; i++) BlendPx(row[i], c, a);
				if (i >= x1) break;
			}
			const float d = RoundRectDist(i + 0.5f, cy, x + hw, y + hh, hw, hh, r);
			const float cov = std::max(0.0f, std::min(1.0f, 0.5f - d));
			if (cov > 0) BlendPx(row[i], c, a * cov);
		}
	}
}

void StrokeRound(uint32_t *buf, int W, int H, float x, float y, float w, float h, float r, float thick, COLORREF c, float a)
{
	const int x0 = std::max(std::max(0, g_clip.x0), static_cast<int>(std::floor(x)) - 1), x1 = std::min(std::min(W, g_clip.x1), static_cast<int>(std::ceil(x + w)) + 1);
	const int y0 = std::max(std::max(0, g_clip.y0), static_cast<int>(std::floor(y)) - 1), y1 = std::min(std::min(H, g_clip.y1), static_cast<int>(std::ceil(y + h)) + 1);
	const float hw = w / 2, hh = h / 2;
	r = std::min(r, std::min(hw, hh));
	const float m = thick + 1.5f;
	for (int j = y0; j < y1; j++) {
		int skipL = x1, skipR = x1;
		const float cy = j + 0.5f;
		if (cy >= y + m && cy <= y + h - m) {
			const float inset = (cy >= y + r + m && cy <= y + h - r - m) ? m : r + m;
			skipL = std::max(x0, static_cast<int>(std::ceil(x + inset)));
			skipR = std::min(x1, static_cast<int>(std::floor(x + w - inset)));
			if (skipR <= skipL) skipL = skipR = x1;
		}
		uint32_t *row = buf + static_cast<size_t>(j) * W;
		for (int i = x0; i < x1; i++) {
			if (i == skipL) {
				i = skipR;
				if (i >= x1) break;
			}
			const float d = RoundRectDist(i + 0.5f, cy, x + hw, y + hh, hw, hh, r);
			const float cov = std::max(0.0f, std::min(1.0f, thick * 0.5f + 0.5f - std::fabs(d + thick * 0.5f)));
			if (cov > 0) BlendPx(row[i], c, a * cov);
		}
	}
}

void FillDisc(uint32_t *buf, int W, int H, float cx, float cy, float r, COLORREF c, float a)
{
	FillRound(buf, W, H, cx - r, cy - r, r * 2, r * 2, r, c, a);
}

void FillShape(uint32_t *buf, int W, int H, const std::vector<std::vector<ShapePoint>> &shape, float x, float y, float scale, COLORREF c, float a)
{
	float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
	for (const std::vector<ShapePoint> &poly : shape) {
		for (const ShapePoint &pt : poly) {
			minX = std::min(minX, pt.x);
			maxX = std::max(maxX, pt.x);
			minY = std::min(minY, pt.y);
			maxY = std::max(maxY, pt.y);
		}
	}
	if (minX > maxX || scale <= 0) return;
	const int x0 = std::max(std::max(0, g_clip.x0), static_cast<int>(std::floor(x + minX * scale)));
	const int x1 = std::min(std::min(W, g_clip.x1), static_cast<int>(std::ceil(x + maxX * scale)) + 1);
	const int y0 = std::max(std::max(0, g_clip.y0), static_cast<int>(std::floor(y + minY * scale)));
	const int y1 = std::min(std::min(H, g_clip.y1), static_cast<int>(std::ceil(y + maxY * scale)) + 1);
	const int kSub = 4;
	for (int j = y0; j < y1; j++) {
		for (int i = x0; i < x1; i++) {
			int hits = 0;
			for (int sy = 0; sy < kSub; sy++) {
				const float py = (j + (sy + 0.5f) / kSub - y) / scale;
				for (int sx = 0; sx < kSub; sx++) {
					const float px = (i + (sx + 0.5f) / kSub - x) / scale;
					bool inside = false;
					for (const std::vector<ShapePoint> &poly : shape) {
						for (size_t k = 0, m = poly.size() - 1; k < poly.size(); m = k++) {
							const ShapePoint &p = poly[k], &q = poly[m];
							if ((p.y > py) != (q.y > py) && px < (q.x - p.x) * (py - p.y) / (q.y - p.y) + p.x) inside = !inside;
						}
					}
					if (inside) hits++;
				}
			}
			if (hits) BlendPx(buf[static_cast<size_t>(j) * W + i], c, a * hits / (kSub * kSub));
		}
	}
}

void FillTriangle(uint32_t *buf, int W, int H, float ax, float ay, float bx, float by, float cx, float cy, COLORREF c, float a)
{
	const float area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
	const float sign = area < 0 ? -1.0f : 1.0f;
	const float px[3] = {ax, bx, cx}, py[3] = {ay, by, cy};
	const int x0 = std::max(std::max(0, g_clip.x0), static_cast<int>(std::floor(std::min({ax, bx, cx}))) - 1), x1 = std::min(std::min(W, g_clip.x1), static_cast<int>(std::ceil(std::max({ax, bx, cx}))) + 1);
	const int y0 = std::max(std::max(0, g_clip.y0), static_cast<int>(std::floor(std::min({ay, by, cy}))) - 1), y1 = std::min(std::min(H, g_clip.y1), static_cast<int>(std::ceil(std::max({ay, by, cy}))) + 1);
	for (int j = y0; j < y1; j++) {
		for (int i = x0; i < x1; i++) {
			float d = 1e9f;
			for (int k = 0; k < 3; k++) {
				const int n = (k + 1) % 3;
				const float ex = px[n] - px[k], ey = py[n] - py[k], len = std::sqrt(ex * ex + ey * ey);
				const float dist = sign * (ex * (j + 0.5f - py[k]) - ey * (i + 0.5f - px[k])) / len;
				d = std::min(d, dist);
			}
			const float cov = std::max(0.0f, std::min(1.0f, d + 0.5f));
			if (cov > 0) BlendPx(buf[static_cast<size_t>(j) * W + i], c, a * cov);
		}
	}
}

void DrawLabel(uint32_t *buf, int W, int H, const std::wstring &t, float fx, float fy, float fw, float fh, UINT align, COLORREF col, float a, HFONT font)
{
	const int x = static_cast<int>(fx), y = static_cast<int>(fy), w = std::min(static_cast<int>(fw), 400), h = std::min(static_cast<int>(fh), 64);
	if (t.empty() || w <= 0 || h <= 0 || !g_spec.text.Ensure(400, 64)) return;
	memset(g_spec.text.bits, 0, static_cast<size_t>(400) * h * 4);
	HDC dc = g_spec.text.dc;
	SelectObject(dc, font ? font : g_fonts[3]);
	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, RGB(255, 255, 255));
	RECT r = {0, 0, w, h};
	DrawTextW(dc, t.c_str(), static_cast<int>(t.size()), &r, align | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	GdiFlush();
	for (int j = 0; j < h; j++) {
		const int dy = y + j;
		if (dy < 0 || dy >= H || dy < g_clip.y0 || dy >= g_clip.y1) continue;
		for (int i = 0; i < w; i++) {
			const int dx = x + i;
			if (dx < 0 || dx >= W || dx < g_clip.x0 || dx >= g_clip.x1) continue;
			const float cov = ((g_spec.text.bits[static_cast<size_t>(j) * 400 + i] >> 8) & 255) / 255.0f;
			if (cov > 0) BlendPx(buf[static_cast<size_t>(dy) * W + dx], col, a * cov);
		}
	}
}

std::wstring ClockText(unsigned ms)
{
	const unsigned total = ms / 1000;
	wchar_t out[24];
	if (total >= 3600) swprintf_s(out, L"%u:%02u:%02u", total / 3600, (total / 60) % 60, total % 60);
	else swprintf_s(out, L"%u:%02u", total / 60, total % 60);
	return out;
}
}
