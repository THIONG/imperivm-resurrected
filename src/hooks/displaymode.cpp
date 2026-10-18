// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>

#include <mutex>
#include <string>
#include <vector>

#include "MinHook.h"
#include "hooks/hookcount.h"
#include "hooks/audio.h"
#include "core/config.h"
#include "hooks/displaymode.h"
#include "core/log.h"

namespace {

typedef LONG(WINAPI *ChangeDisplaySettingsA_t)(DEVMODEA *, DWORD);
typedef BOOL(WINAPI *EnumDisplaySettingsA_t)(LPCSTR, DWORD, DEVMODEA *);
typedef int(WINAPI *GetSystemMetrics_t)(int);
typedef BOOL(WINAPI *SetWindowPos_t)(HWND, HWND, int, int, int, int, UINT);
typedef BOOL(WINAPI *MoveWindow_t)(HWND, int, int, int, int, BOOL);
typedef BOOL(WINAPI *GetClientRect_t)(HWND, LPRECT);
typedef BOOL(WINAPI *GetCursorPos_t)(LPPOINT);
typedef BOOL(WINAPI *ClipCursor_t)(const RECT *);
typedef LONG(WINAPI *SetWindowLongA_t)(HWND, int, LONG);

ChangeDisplaySettingsA_t g_origChange = nullptr;
EnumDisplaySettingsA_t g_origEnum = nullptr;
GetSystemMetrics_t g_origMetrics = nullptr;
SetWindowPos_t g_origSetWindowPos = nullptr;
MoveWindow_t g_origMoveWindow = nullptr;
GetClientRect_t g_origGetClientRect = nullptr;
GetCursorPos_t g_origGetCursorPos = nullptr;
ClipCursor_t g_origClipCursor = nullptr;
SetWindowLongA_t g_origSetWindowLong = nullptr;

bool g_faking = false;
int g_logW = 0, g_logH = 0;
DWORD g_logBpp = 16;
HWND g_gameWnd = nullptr;
WNDPROC g_gameWndProc = nullptr;
LONG g_originalStyle = WS_POPUP | WS_VISIBLE;
DisplayWindowHandler g_windowHandler = nullptr;
std::string g_lastGeometry;
int g_clicksLogged = 0;
const int kClicksLogged = 5;

const LONG kWindowedStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

int DesktopW() { return g_origMetrics ? g_origMetrics(SM_CXSCREEN) : GetSystemMetrics(SM_CXSCREEN); }
int DesktopH() { return g_origMetrics ? g_origMetrics(SM_CYSCREEN) : GetSystemMetrics(SM_CYSCREEN); }

bool Enabled() { return GetConfig().fakeModeChange && GetConfig().renderEnabled; }
bool Windowed() { return Enabled() && GetConfig().windowed; }

RECT PhysClient()
{
	RECT rc = {};
	if (g_gameWnd) {
		g_origGetClientRect(g_gameWnd, &rc);
	}
	return rc;
}

bool Scaling()
{
	if (!g_faking || !g_gameWnd || g_logW <= 0) {
		return false;
	}
	RECT rc = PhysClient();
	return rc.right - rc.left != g_logW || rc.bottom - rc.top != g_logH;
}

void Mapping(float &s, float &offX, float &offY)
{
	RECT rc = PhysClient();
	float pw = static_cast<float>(rc.right - rc.left), ph = static_cast<float>(rc.bottom - rc.top);
	float sx = pw / g_logW, sy = ph / g_logH;
	s = sx < sy ? sx : sy;
	offX = (pw - g_logW * s) / 2;
	offY = (ph - g_logH * s) / 2;
}

void PhysToLog(int &x, int &y)
{
	float s, ox, oy;
	Mapping(s, ox, oy);
	int lx = static_cast<int>((x - ox) / s), ly = static_cast<int>((y - oy) / s);
	x = lx < 0 ? 0 : (lx >= g_logW ? g_logW - 1 : lx);
	y = ly < 0 ? 0 : (ly >= g_logH ? g_logH - 1 : ly);
}

void LogGeometry(const char *why)
{
	if (!g_gameWnd) return;
	RECT wr = {};
	GetWindowRect(g_gameWnd, &wr);
	const RECT cr = PhysClient();
	POINT origin = {0, 0};
	ClientToScreen(g_gameWnd, &origin);
	MONITORINFO mi = {};
	mi.cbSize = sizeof(mi);
	GetMonitorInfoA(MonitorFromWindow(g_gameWnd, MONITOR_DEFAULTTONEAREST), &mi);
	typedef UINT(WINAPI * GetDpiForWindow_t)(HWND);
	static const GetDpiForWindow_t dpiFor = reinterpret_cast<GetDpiForWindow_t>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
	float s = 1, ox = 0, oy = 0;
	if (Scaling()) Mapping(s, ox, oy);
	char text[400];
	sprintf_s(text, "game %dx%d, window %ldx%ld at %ld,%ld, client %ldx%ld at %ld,%ld, monitor %ldx%ld at %ld,%ld, window DPI %u, scale %.3f, margins %.0f,%.0f",
		g_logW, g_logH, wr.right - wr.left, wr.bottom - wr.top, wr.left, wr.top, cr.right - cr.left, cr.bottom - cr.top, origin.x, origin.y,
		mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, mi.rcMonitor.left, mi.rcMonitor.top, dpiFor ? dpiFor(g_gameWnd) : 0u,
		s, ox, oy);
	if (g_lastGeometry == text) return;
	g_lastGeometry = text;
	LogI("display", "geometry (%s): %s", why, text);
}

void LogClick(LPARAM lp)
{
	if (g_clicksLogged >= kClicksLogged) return;
	g_clicksLogged++;
	const int rx = static_cast<short>(LOWORD(lp)), ry = static_cast<short>(HIWORD(lp));
	int gx = rx, gy = ry;
	if (Scaling()) PhysToLog(gx, gy);
	POINT cursor = {};
	if (g_origGetCursorPos ? g_origGetCursorPos(&cursor) : GetCursorPos(&cursor)) ScreenToClient(g_gameWnd, &cursor);
	const RECT cr = PhysClient();
	LogI("display", "click %d: message at %d,%d of a %ldx%ld client, cursor at %ld,%ld -> game %d,%d of %dx%d", g_clicksLogged, rx, ry,
		cr.right - cr.left, cr.bottom - cr.top, cursor.x, cursor.y, gx, gy, g_logW, g_logH);
}

void PlaceWindow()
{
	if (!g_gameWnd || !g_faking) {
		return;
	}
	if (Windowed()) {
		g_origSetWindowLong(g_gameWnd, GWL_STYLE, kWindowedStyle);
		RECT r = {0, 0, GetConfig().windowW, GetConfig().windowH};
		AdjustWindowRectEx(&r, kWindowedStyle, FALSE, 0);
		int w = r.right - r.left, h = r.bottom - r.top;
		g_origSetWindowPos(g_gameWnd, HWND_NOTOPMOST, (DesktopW() - w) / 2, (DesktopH() - h) / 2, w, h,
			SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}
	else {
		g_origSetWindowPos(g_gameWnd, nullptr, 0, 0, DesktopW(), DesktopH(), SWP_NOZORDER | SWP_NOACTIVATE);
	}
	LogGeometry(Windowed() ? "windowed" : "full screen");
}

bool FakeMode(int w, int h, DWORD bpp, bool testOnly)
{
	if (!Enabled()) {
		return false;
	}
	const int maxW = 2 * DesktopW() < 4096 ? 2 * DesktopW() : 4096;
	const int maxH = 2 * DesktopH() < 2304 ? 2 * DesktopH() : 2304;
	if (w <= 0 || h <= 0 || w > maxW || h > maxH) {
		return false;
	}
	if (testOnly) {
		return true;
	}
	g_faking = true;
	g_logW = w;
	g_logH = h;
	g_logBpp = bpp ? bpp : 16;
	LogD("display", "the game requests %dx%d %lubpp -> faked mode, the monitor stays at %dx%d", w, h, g_logBpp,
		DesktopW(), DesktopH());
	PlaceWindow();
	return true;
}

LONG WINAPI HookChangeDisplaySettingsA(DEVMODEA *mode, DWORD flags)
{
	if (!mode) {
		LogD("display", "restore mode -> not needed, the monitor never changed");
		g_faking = false;
		return DISP_CHANGE_SUCCESSFUL;
	}
	const int w = static_cast<int>(mode->dmPelsWidth), h = static_cast<int>(mode->dmPelsHeight);
	if (FakeMode(w, h, mode->dmBitsPerPel, (flags & CDS_TEST) != 0)) {
		return DISP_CHANGE_SUCCESSFUL;
	}
	LogD("display", "%dx%d outside the supported range -> real change", w, h);
	g_faking = false;
	return g_origChange(mode, flags);
}

std::mutex g_modesMx;
std::vector<DWORD> g_modeMap;
bool g_modesReady = false;
bool g_realHas16 = false;

void PrepareModes()
{
	std::lock_guard<std::mutex> lock(g_modesMx);
	if (g_modesReady) return;
	bool has16 = false;
	for (DWORD n = 0;; n++) {
		DEVMODEA dm = {};
		dm.dmSize = sizeof(dm);
		if (!g_origEnum(nullptr, n, &dm)) break;
		if (dm.dmBitsPerPel == 16) has16 = true;
		g_modeMap.push_back(n);
	}
	g_realHas16 = has16;
	g_modesReady = true;
	LogI("display", "Windows lists %d display modes%s", static_cast<int>(g_modeMap.size()),
		has16 ? "" : ", none of 16 bits: the game is offered the 32-bit ones as 16");
}

BOOL WINAPI HookEnumDisplaySettingsA(LPCSTR device, DWORD modeNum, DEVMODEA *mode)
{
	if (!device && modeNum != ENUM_CURRENT_SETTINGS && modeNum != ENUM_REGISTRY_SETTINGS) {
		PrepareModes();
		const DWORD n = static_cast<DWORD>(g_modeMap.size());
		if (modeNum < n) {
			return g_origEnum(device, g_modeMap[modeNum], mode);
		}
		if (g_realHas16 || modeNum >= 2 * n) {
			return FALSE;
		}
		const BOOL real = g_origEnum(device, g_modeMap[modeNum - n], mode);
		if (real) mode->dmBitsPerPel = 16;
		return real;
	}
	BOOL ok = g_origEnum(device, modeNum, mode);
	if (ok && g_faking && modeNum == ENUM_CURRENT_SETTINGS && !device) {
		mode->dmBitsPerPel = g_logBpp;
		mode->dmPelsWidth = g_logW;
		mode->dmPelsHeight = g_logH;
	}
	return ok;
}

int WINAPI HookGetSystemMetrics(int index)
{
	if (g_faking) {
		if (index == SM_CXSCREEN) return g_logW;
		if (index == SM_CYSCREEN) return g_logH;
	}
	return g_origMetrics(index);
}

BOOL WINAPI HookSetWindowPos(HWND hwnd, HWND after, int x, int y, int cx, int cy, UINT flags)
{
	if (g_faking && hwnd == g_gameWnd && !(flags & SWP_NOSIZE)) {
		PlaceWindow();
		return TRUE;
	}
	if (Windowed() && hwnd == g_gameWnd && after == HWND_TOPMOST) {
		after = HWND_NOTOPMOST;
	}
	return g_origSetWindowPos(hwnd, after, x, y, cx, cy, flags);
}

BOOL WINAPI HookMoveWindow(HWND hwnd, int x, int y, int w, int h, BOOL repaint)
{
	if (g_faking && hwnd == g_gameWnd) {
		PlaceWindow();
		return TRUE;
	}
	return g_origMoveWindow(hwnd, x, y, w, h, repaint);
}

BOOL WINAPI HookGetClientRect(HWND hwnd, LPRECT rc)
{
	if (rc && hwnd == g_gameWnd && Scaling()) {
		rc->left = rc->top = 0;
		rc->right = g_logW;
		rc->bottom = g_logH;
		return TRUE;
	}
	return g_origGetClientRect(hwnd, rc);
}

BOOL WINAPI HookGetCursorPos(LPPOINT pt)
{
	BOOL ok = g_origGetCursorPos(pt);
	if (ok && Scaling()) {
		POINT origin = {0, 0};
		ClientToScreen(g_gameWnd, &origin);
		int x = pt->x - origin.x, y = pt->y - origin.y;
		PhysToLog(x, y);
		pt->x = x + origin.x;
		pt->y = y + origin.y;
	}
	return ok;
}

BOOL WINAPI HookClipCursor(const RECT *rc)
{
	if (rc && (Windowed() || !GetConfig().trapMouse)) {
		return g_origClipCursor(nullptr);
	}
	if (rc && Scaling()) {
		float s, ox, oy;
		Mapping(s, ox, oy);
		POINT origin = {0, 0};
		ClientToScreen(g_gameWnd, &origin);
		RECT p = {static_cast<LONG>(origin.x + ox + (rc->left - origin.x) * s),
			static_cast<LONG>(origin.y + oy + (rc->top - origin.y) * s),
			static_cast<LONG>(origin.x + ox + (rc->right - origin.x) * s),
			static_cast<LONG>(origin.y + oy + (rc->bottom - origin.y) * s)};
		return g_origClipCursor(&p);
	}
	return g_origClipCursor(rc);
}

LRESULT CALLBACK GameWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if (msg == WM_LBUTTONDOWN) LogClick(lp);
	if (msg == WM_SIZE || msg == WM_DISPLAYCHANGE || msg == WM_DPICHANGED) LogGeometry("window changed");
	if (Scaling() && msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST && msg != WM_MOUSEWHEEL && msg != WM_MOUSEHWHEEL) {
		int x = static_cast<short>(LOWORD(lp)), y = static_cast<short>(HIWORD(lp));
		PhysToLog(x, y);
		lp = MAKELPARAM(x, y);
	}
	if (g_windowHandler) return g_windowHandler(hwnd, msg, wp, lp);
	return CallWindowProcA(g_gameWndProc, hwnd, msg, wp, lp);
}

LONG WINAPI HookSetWindowLongA(HWND hwnd, int index, LONG value)
{
	if (hwnd == g_gameWnd && g_gameWndProc) {
		if (index == GWL_WNDPROC) {
			LONG prev = reinterpret_cast<LONG>(g_gameWndProc);
			g_gameWndProc = reinterpret_cast<WNDPROC>(value);
			return prev;
		}
		if (index == GWL_STYLE && Windowed() && g_faking) {
			value = kWindowedStyle;
		}
		if (index == GWL_EXSTYLE && Windowed()) {
			value &= ~WS_EX_TOPMOST;
		}
	}
	return g_origSetWindowLong(hwnd, index, value);
}

bool Hook(const char *name, void *detour, void **orig)
{
	HMODULE m = GetModuleHandleW(L"user32.dll");
	void *target = m ? reinterpret_cast<void *>(GetProcAddress(m, name)) : nullptr;
	if (target && HookCreate(target, detour, orig) == MH_OK && HookEnable(target) == MH_OK) {
		return true;
	}
	LogE("display", "could not intercept %s", name);
	return false;
}

}

bool DisplayFakeMode(int w, int h, DWORD bpp, bool testOnly)
{
	return FakeMode(w, h, bpp, testOnly);
}

bool InstallDisplayModeHooks()
{
	bool ok = Hook("GetSystemMetrics", reinterpret_cast<void *>(&HookGetSystemMetrics), reinterpret_cast<void **>(&g_origMetrics)) &&
		Hook("GetClientRect", reinterpret_cast<void *>(&HookGetClientRect), reinterpret_cast<void **>(&g_origGetClientRect));
	if (ok && Enabled()) {
		ok = Hook("ChangeDisplaySettingsA", reinterpret_cast<void *>(&HookChangeDisplaySettingsA), reinterpret_cast<void **>(&g_origChange)) &&
			Hook("EnumDisplaySettingsA", reinterpret_cast<void *>(&HookEnumDisplaySettingsA), reinterpret_cast<void **>(&g_origEnum)) &&
			Hook("SetWindowPos", reinterpret_cast<void *>(&HookSetWindowPos), reinterpret_cast<void **>(&g_origSetWindowPos)) &&
			Hook("MoveWindow", reinterpret_cast<void *>(&HookMoveWindow), reinterpret_cast<void **>(&g_origMoveWindow)) &&
			Hook("GetCursorPos", reinterpret_cast<void *>(&HookGetCursorPos), reinterpret_cast<void **>(&g_origGetCursorPos)) &&
			Hook("ClipCursor", reinterpret_cast<void *>(&HookClipCursor), reinterpret_cast<void **>(&g_origClipCursor)) &&
			Hook("SetWindowLongA", reinterpret_cast<void *>(&HookSetWindowLongA), reinterpret_cast<void **>(&g_origSetWindowLong));
	}
	LogAt(ok ? kLogInfo : kLogError, "display", "hooks %s (FakeModeChange=%d, mode %s)", ok ? "installed" : "with errors",
		GetConfig().fakeModeChange, GetConfig().windowed ? "windowed" : "full screen");
	return ok;
}

void DisplayAttachWindow(HWND hwnd)
{
	if (g_gameWnd || !hwnd) {
		return;
	}
	g_gameWnd = hwnd;
	g_originalStyle = GetWindowLongA(hwnd, GWL_STYLE);
	LogGeometry("game window");
	AudioSetBackgroundMute(false);
	if (Enabled()) {
		g_gameWndProc = reinterpret_cast<WNDPROC>(
			g_origSetWindowLong(hwnd, GWL_WNDPROC, reinterpret_cast<LONG>(&GameWndProc)));
		PlaceWindow();
		if (Windowed() || !GetConfig().trapMouse) {
			g_origClipCursor(nullptr);
		}
	}
}

void DisplayGetLogicalSize(HWND hwnd, int &w, int &h)
{
	if (g_faking && g_logW > 0) {
		w = g_logW;
		h = g_logH;
		return;
	}
	RECT rc = {};
	g_origGetClientRect(hwnd, &rc);
	w = rc.right - rc.left;
	h = rc.bottom - rc.top;
}

BOOL DisplayRealClientRect(HWND hwnd, RECT *rc)
{
	return g_origGetClientRect ? g_origGetClientRect(hwnd, rc) : GetClientRect(hwnd, rc);
}

void DisplayApplySettings(bool windowModeChanged)
{
	if (!g_gameWnd || !Enabled()) {
		return;
	}
	if (windowModeChanged) {
		if (!Windowed()) {
			g_origSetWindowLong(g_gameWnd, GWL_STYLE, g_originalStyle);
		}
		PlaceWindow();
	}
	if (Windowed() || !GetConfig().trapMouse) {
		g_origClipCursor(nullptr);
	}
	else {
		RECT r;
		g_origGetClientRect(g_gameWnd, &r);
		MapWindowPoints(g_gameWnd, nullptr, reinterpret_cast<POINT *>(&r), 2);
		g_origClipCursor(&r);
	}
}

void DisplaySetWindowHandler(DisplayWindowHandler handler)
{
	g_windowHandler = handler;
}

LRESULT DisplayForwardMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	return CallWindowProcA(g_gameWndProc, hwnd, msg, wp, lp);
}

bool DisplayIsWindowed()
{
	return Windowed();
}
