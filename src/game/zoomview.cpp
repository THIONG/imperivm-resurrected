// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

#include "MinHook.h"
#include "hooks/hookcount.h"

#include "addr/state.h"
#include "addr/world.h"
#include "core/log.h"
#include "game/mapfog.h"
#include "game/quickicons.h"
#include "game/zoomview.h"
#include "hooks/displaymode.h"

namespace {
typedef void(__fastcall *ZoomDraw_t)(void *self, void *edx, void *surface, int unused);
typedef void(__fastcall *FillRect_t)(void *surface, void *edx, int *rect, unsigned color);
typedef void(__fastcall *Manager_t)(void *manager);
typedef void(__fastcall *ManagerRect_t)(void *manager, void *edx, int *rect);
typedef void(__fastcall *ManagerApply_t)(void *manager, void *edx, int a, int b);
typedef void(__fastcall *ManagerClear_t)(void *manager, void *edx, int a);
typedef unsigned(__fastcall *Height_t)(void *map, void *edx, unsigned *position);

ZoomDraw_t g_origDraw = nullptr;
bool g_failed = false;

struct Geometry {
	bool valid = false;
	DWORD tick = 0;
	int left = 0, top = 0, right = 0, bottom = 0;
	int winX = 0, winY = 0;
};
Geometry g_geo;

struct Drag {
	bool pending = false, dragging = false;
	int startX = 0, startY = 0, curX = 0, curY = 0;
	WPARAM downWp = 0;
	LPARAM downLp = 0;
};
Drag g_drag;
bool g_swallowLeftUp = false, g_swallowRightUp = false;

const DWORD kOpenWindowMs = 250;
const int kDragThreshold = 5;
const int kMessageLeftDown = 0x111, kMessageLeftDouble = 0x113, kMessageRightDown = 0x121;

template <typename T>
T &At(void *p, int off) { return *reinterpret_cast<T *>(static_cast<char *>(p) + off); }

bool ExtrasEnabled()
{
	char path[MAX_PATH] = {};
	GetModuleFileNameA(nullptr, path, MAX_PATH);
	if (char *slash = strrchr(path, 0x5c)) strcpy_s(slash + 1, MAX_PATH - (slash + 1 - path), "imperivm-mod.ini");
	return GetPrivateProfileIntA("Map", "ZoomExtras", 1, path) != 0;
}

bool ZoomOpen() { return g_geo.valid && GetTickCount() - g_geo.tick < kOpenWindowMs; }

void ResetDrag() { g_drag = Drag(); }

void FillEdge(void *surface, int l, int t, int r, int b, unsigned color)
{
	void **vtable = *reinterpret_cast<void ***>(surface);
	int rect[4] = {l - At<int>(surface, 0x24), t - At<int>(surface, 0x28), r - At<int>(surface, 0x24), b - At<int>(surface, 0x28)};
	reinterpret_cast<FillRect_t>(vtable[0x3c / 4])(surface, nullptr, rect, color);
}

void DrawDragRectangle(void *surface)
{
	if (!g_drag.dragging) return;
	const int l = std::min(g_drag.startX, g_drag.curX), r = std::max(g_drag.startX, g_drag.curX);
	const int t = std::min(g_drag.startY, g_drag.curY), b = std::max(g_drag.startY, g_drag.curY);
	const unsigned kEdge = 0x7fff, kShade = 0x2108;
	FillEdge(surface, l - 1, t - 1, r + 1, t - 1, kShade);
	FillEdge(surface, l - 1, b + 1, r + 1, b + 1, kShade);
	FillEdge(surface, l - 1, t - 1, l - 1, b + 1, kShade);
	FillEdge(surface, r + 1, t - 1, r + 1, b + 1, kShade);
	FillEdge(surface, l, t, r, t, kEdge);
	FillEdge(surface, l, b, r, b, kEdge);
	FillEdge(surface, l, t, l, b, kEdge);
	FillEdge(surface, r, t, r, b, kEdge);
}

void CaptureGeometry(void *self, void *surface)
{
	const bool wasClosed = !ZoomOpen();
	char *base = static_cast<char *>(self);
	Geometry g;
	g.left = *reinterpret_cast<int *>(base - 0x98);
	g.top = *reinterpret_cast<int *>(base - 0x94);
	g.right = *reinterpret_cast<int *>(base - 0x90);
	g.bottom = *reinterpret_cast<int *>(base - 0x8c);
	const int *virtualBase = *reinterpret_cast<int **>(base - 0x9c);
	g.winX = *reinterpret_cast<short *>(base + virtualBase[2] - 0x8c);
	g.winY = *reinterpret_cast<short *>(base + virtualBase[2] - 0x8a);
	g.valid = true;
	g.tick = GetTickCount();
	g_geo = g;
	static int logged = 0;
	if (wasClosed && logged++ < 6) {
		LogD("game", "zoom map open: bitmap %d,%d to %d,%d, window %d,%d, surface offset %d,%d clip %d,%d to %d,%d", g.left, g.top, g.right, g.bottom, g.winX, g.winY,
			At<int>(surface, 0x24), At<int>(surface, 0x28), At<int>(surface, 4), At<int>(surface, 8), At<int>(surface, 0xc), At<int>(surface, 0x10));
	}
}

void PreviewBox(int x0, int y0, int x1, int y1, bool force);

void __fastcall HookZoomDraw(void *self, void *edx, void *surface, int unused)
{
	g_origDraw(self, edx, surface, unused);
	if (g_failed) return;
	__try {
		CaptureGeometry(self, surface);
		QuickIconsDraw(surface, g_geo.winX, g_geo.winY);
		if (g_drag.dragging) PreviewBox(g_drag.startX, g_drag.startY, g_drag.curX, g_drag.curY, false);
		DrawDragRectangle(surface);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		g_failed = true;
		LogE("game", "zoom map: ERROR drawing the extras, disabled");
	}
}

bool ToWorld(int x, int y, int *world)
{
	int width = 0, height = 0;
	if (!ZoomBitmapSize(&width, &height)) return false;
	const int spanX = std::max(1, g_geo.right - g_geo.left), spanY = std::max(1, g_geo.bottom - g_geo.top);
	const int px = std::max(0, std::min(width - 1, (x - g_geo.left) * (width - 1) / spanX));
	const int py = std::max(0, std::min(height - 1, (y - g_geo.top) * (height - 1) / spanY));
	return ZoomPixelToWorld(px, py, world);
}

void *Manager() { return *reinterpret_cast<void **>(addr::kSelectionManager); }

bool ProjectedRect(int x0, int y0, int x1, int y1, int *rect)
{
	int a[2] = {}, b[2] = {};
	if (!ToWorld(x0, y0, a) || !ToWorld(x1, y1, b)) return false;
	void *map = *reinterpret_cast<void **>(addr::kMap);
	if (!map) return false;
	int px[2], py[2];
	const int *points[2] = {a, b};
	for (int i = 0; i < 2; i++) {
		unsigned position[2] = {static_cast<unsigned>(points[i][0]), static_cast<unsigned>(points[i][1])};
		const unsigned height = reinterpret_cast<Height_t>(addr::kGetHeightAt)(map, nullptr, position);
		px[i] = points[i][0];
		py[i] = ((points[i][1] * 0xb5) >> 8) - static_cast<int>(height);
	}
	rect[0] = std::min(px[0], px[1]);
	rect[1] = std::min(py[0], py[1]);
	rect[2] = std::max(px[0], px[1]);
	rect[3] = std::max(py[0], py[1]);
	return true;
}

bool g_boxOpen = false;
DWORD g_lastPreview = 0;

void BeginBox(bool add)
{
	void *manager = Manager();
	if (!manager) return;
	if (!add) reinterpret_cast<ManagerClear_t>(addr::kClearSelection)(manager, nullptr, 1);
	reinterpret_cast<Manager_t>(addr::kBeginBoxSelect)(manager);
	g_boxOpen = true;
	g_lastPreview = 0;
}

void PreviewBox(int x0, int y0, int x1, int y1, bool force)
{
	void *manager = Manager();
	const DWORD now = GetTickCount();
	if (!manager || !g_boxOpen || (!force && now - g_lastPreview < 30)) return;
	g_lastPreview = now;
	int rect[4];
	if (!ProjectedRect(x0, y0, x1, y1, rect)) return;
	reinterpret_cast<ManagerRect_t>(addr::kBoxSelect)(manager, nullptr, rect);
	reinterpret_cast<ManagerApply_t>(addr::kApplySelection)(manager, nullptr, 1, 0);
}

void EndBox()
{
	void *manager = Manager();
	if (!manager || !g_boxOpen) return;
	g_boxOpen = false;
	reinterpret_cast<Manager_t>(addr::kEndBoxSelect)(manager);
	reinterpret_cast<Manager_t>(addr::kCommitPendingSelection)(manager);
	reinterpret_cast<ManagerApply_t>(addr::kApplySelection)(manager, nullptr, 1, 0);
}

bool InsideBitmap(int x, int y) { return x >= g_geo.left && x <= g_geo.right && y >= g_geo.top && y <= g_geo.bottom; }
}

bool ZoomViewHandleMouse(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if (g_failed) return false;
	if (*reinterpret_cast<void **>(addr::kPausedMenu) != nullptr) {
		if (g_boxOpen) EndBox();
		ResetDrag();
		g_swallowLeftUp = g_swallowRightUp = false;
		return false;
	}
	if (!ZoomOpen()) {
		if (g_boxOpen) EndBox();
		if (g_drag.pending || g_drag.dragging) ResetDrag();
		g_swallowLeftUp = g_swallowRightUp = false;
		return false;
	}
	const int x = static_cast<short>(LOWORD(lp)), y = static_cast<short>(HIWORD(lp));
	const int viewX = x - g_geo.winX, viewY = y - g_geo.winY;
	switch (msg) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		ResetDrag();
		if (QuickIconsHit(viewX, viewY)) {
			QuickIconsMouse(msg == WM_LBUTTONDOWN ? kMessageLeftDown : kMessageLeftDouble, viewX, viewY);
			g_swallowLeftUp = true;
			return true;
		}
		if (msg == WM_LBUTTONDOWN && InsideBitmap(x, y)) {
			g_drag.pending = true;
			g_drag.startX = g_drag.curX = x;
			g_drag.startY = g_drag.curY = y;
			g_drag.downWp = wp;
			g_drag.downLp = lp;
			return true;
		}
		return false;
	case WM_RBUTTONDOWN:
		if (QuickIconsHit(viewX, viewY)) {
			QuickIconsMouse(kMessageRightDown, viewX, viewY);
			g_swallowRightUp = true;
			return true;
		}
		return false;
	case WM_MOUSEMOVE:
		if (g_drag.pending) {
			g_drag.curX = x;
			g_drag.curY = y;
			if (!g_drag.dragging && (std::abs(x - g_drag.startX) >= kDragThreshold || std::abs(y - g_drag.startY) >= kDragThreshold)) {
				g_drag.dragging = true;
				BeginBox((GetKeyState(VK_SHIFT) & 0x8000) != 0);
			}
			if (g_drag.dragging) PreviewBox(g_drag.startX, g_drag.startY, x, y, false);
		}
		return false;
	case WM_LBUTTONUP:
		if (g_swallowLeftUp) {
			g_swallowLeftUp = false;
			return true;
		}
		if (g_drag.pending) {
			const Drag drag = g_drag;
			ResetDrag();
			if (drag.dragging) {
				PreviewBox(drag.startX, drag.startY, x, y, true);
				EndBox();
				return true;
			}
			DisplayForwardMessage(hwnd, WM_LBUTTONDOWN, drag.downWp, drag.downLp);
		}
		return false;
	case WM_RBUTTONUP:
		if (g_swallowRightUp) {
			g_swallowRightUp = false;
			return true;
		}
		return false;
	default:
		return false;
	}
}

void InstallZoomView(bool knownExe)
{
	if (!knownExe) return;
	if (!ExtrasEnabled()) {
		g_failed = true;
		LogI("game", "zoom map extras: disabled in the .ini");
		return;
	}
	const bool ok = HookCreate(reinterpret_cast<void *>(addr::kZoomMapOnDraw), reinterpret_cast<void *>(&HookZoomDraw), reinterpret_cast<void **>(&g_origDraw)) == MH_OK &&
		HookEnable(reinterpret_cast<void *>(addr::kZoomMapOnDraw)) == MH_OK;
	if (!ok) g_failed = true;
	LogAt(ok ? kLogInfo : kLogError, "game", "zoom map extras (group icons and box selection): %s", ok ? "ready" : "NOT available");
}
