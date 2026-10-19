// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <vector>

#include "MinHook.h"
#include "hooks/hookcount.h"

#include "core/log.h"
#include "game/mapfog.h"
#include "addr/engine.h"
#include "addr/state.h"
#include "addr/ui.h"
#include "addr/world.h"

namespace {
typedef void *(__fastcall *Rebuild_t)(void *self);
typedef unsigned(__fastcall *VisionRun_t)(void *query, void *edx, int *point, void *player);
typedef unsigned char(__fastcall *IsExplored_t)(void *exploration, void *edx, unsigned *point, unsigned player);
typedef int(__cdecl *MemLock_t)(unsigned handle);
typedef void(__cdecl *MemUnlock_t)(unsigned handle);
typedef void *(__fastcall *ToMap_t)(void *base, void *edx, int *out, int *in);
typedef int(__fastcall *PixelPtr_t)(void *bitmap, void *edx, int x, int y);

typedef void(__fastcall *CenterOn_t)(void *view, void *edx, int *pos, int *x);
CenterOn_t g_origCenterOn = nullptr;
Rebuild_t g_origRebuild = nullptr;
bool g_enabled = true;
int g_calls = 0;
void *g_zoomThis = nullptr;
DWORD g_lastNatural = 0;
bool g_forcing = false;
DWORD g_refreshMs = 16;
int g_exploredN = 0, g_visibleN = 0;

template <typename T>
T At(const void *p, size_t off) { return *reinterpret_cast<const T *>(static_cast<const char *>(p) + off); }

void PixelToWorld(void *zoom, int px, int py, int *out)
{
	const int *vbptr = At<const int *>(zoom, 0);
	const int off = vbptr[3];
	void *base = static_cast<char *>(zoom) + off;
	void **vt = *reinterpret_cast<void ***>(base);
	int in[2] = {px, py};
	out[0] = out[1] = 0;
	reinterpret_cast<ToMap_t>(vt[0x5c / 4])(base, nullptr, out, in);
}

int Darken(void *zoom)
{
	char *game = *reinterpret_cast<char **>(addr::kGame);
	char *map = *reinterpret_cast<char **>(addr::kMap);
	if (!game || !map) return -1;
	char *player = At<char *>(game, 0x12c8);
	char *vis = At<char *>(game, 0x44d4);
	if (!player || !vis || At<int>(vis, 0xd4) == 0) return -1;
	void *exploration = At<void *>(map, 0xb0);
	const unsigned playerIndex = static_cast<unsigned>((player - game - 0x12cc) / 800);
	if (!exploration || playerIndex > 7) return -1;
	char *dst = At<char *>(zoom, 0x14), *src = At<char *>(zoom, 0x18);
	if (!dst || !src) return -1;
	const int w = At<int>(dst, 8), h = At<int>(dst, 0xc);
	if (w < 16 || h < 16 || w > 8192 || h > 8192 || At<short>(dst, 0x10) != 16 || At<int>(src, 8) != w || At<int>(src, 0xc) != h) return -1;
	const unsigned short fogColor = At<unsigned short>(zoom, 0x30);

	const unsigned hd = At<unsigned>(dst, 4);
	reinterpret_cast<MemLock_t>(addr::kMemLock)(hd);
	int asked = 0;
	const int step = 8, gw = (w + step - 1) / step + 1, gh = (h + step - 1) / step + 1;
	auto rowPtr = [](char *bmp, int y) { return reinterpret_cast<unsigned short *>(reinterpret_cast<PixelPtr_t>((*reinterpret_cast<void ***>(bmp))[0x10 / 4])(bmp, nullptr, 0, y)); };

	static std::vector<unsigned char> explored, seen;
	static DWORD gridTick = 0;
	static int gridW = 0, gridH = 0, nextSlice = 0;
	const int slices = 6;
	const DWORD now = GetTickCount();
	int toDo = 0;
	if (gridW != gw || gridH != gh) {
		explored.assign(static_cast<size_t>(gw) * gh, 0);
		seen.assign(static_cast<size_t>(gw) * gh, 0);
		gridW = gw;
		gridH = gh;
		toDo = slices;
	}
	else {
		toDo = std::max(1, std::min(slices, static_cast<int>((now - gridTick) / 16)));
	}
	gridTick = now;
	g_exploredN = g_visibleN = 0;
	alignas(16) unsigned char query[32] = {};
	for (int k = 0; k < toDo; k++) {
		const int slice = (nextSlice + k) % slices;
		for (int gy = slice; gy < gh; gy += slices) {
			const int py = std::min(h - 1, gy * step);
			for (int gx = 0; gx < gw; gx++) {
				const int px = std::min(w - 1, gx * step);
				const size_t idx = static_cast<size_t>(gy) * gw + gx;
				explored[idx] = 0;
				seen[idx] = 0;
				int world[2] = {};
				PixelToWorld(zoom, px, py, world);
				asked++;
				if (!reinterpret_cast<IsExplored_t>(addr::kIsExplored)(exploration, nullptr, reinterpret_cast<unsigned *>(world), playerIndex)) continue;
				explored[idx] = 1;
				g_exploredN++;
				memset(query, 0, sizeof(query));
				if (reinterpret_cast<VisionRun_t>(addr::kVisionRun)(query, nullptr, world, player)) {
					seen[idx] = 1;
					g_visibleN++;
				}
			}
		}
	}
	nextSlice = (nextSlice + toDo) % slices;

	for (int y = 0; y < h; y++) {
		unsigned short *drow = rowPtr(dst, y);
		if (!drow) continue;
		const int gy = y / step, fy = (y % step) * 256 / step;
		for (int gx = 0; gx < gw - 1; gx++) {
			const size_t idx = static_cast<size_t>(gy) * gw + gx;
			const unsigned char *e0 = &explored[idx], *e1 = e0 + gw;
			if (!(e0[0] | e0[1] | e1[0] | e1[1])) continue;
			const unsigned char *g0 = &seen[idx], *g1 = g0 + gw;
			if (g0[0] & g0[1] & g1[0] & g1[1]) continue;
			const bool inside = (e0[0] & e0[1] & e1[0] & e1[1]) != 0;
			const int x0 = gx * step, x1 = std::min(w, x0 + step);
			if (!(g0[0] | g0[1] | g1[0] | g1[1])) {
				unsigned short *p = drow + x0, *pe = drow + x1;
				if (inside) {
					for (; p < pe; p++) *p = static_cast<unsigned short>((((*p) >> 1) & 0x3DEF) + (((*p) >> 4) & 0x0421));
				}
				else {
					for (; p < pe; p++) {
						if (*p != fogColor) *p = static_cast<unsigned short>((((*p) >> 1) & 0x3DEF) + (((*p) >> 4) & 0x0421));
					}
				}
				continue;
			}
			for (int x = x0; x < x1; x++) {
				if (!inside && drow[x] == fogColor) continue;
				const int fx = (x - x0) * 256 / step;
				const int top = g0[0] * (256 - fx) + g0[1] * fx, bot = g1[0] * (256 - fx) + g1[1] * fx;
				const int s = (top * (256 - fy) + bot * fy) >> 8;
				if (s >= 255) continue;
				const int k = 256 - ((256 - s) * 115 >> 8);
				const unsigned p = drow[x];
				const int r = ((p >> 10) & 31) * k >> 8, g = ((p >> 5) & 31) * k >> 8, b = (p & 31) * k >> 8;
				drow[x] = static_cast<unsigned short>((r << 10) | (g << 5) | b);
			}
		}
	}
	reinterpret_cast<MemUnlock_t>(addr::kMemUnlock)(hd);
	return asked;
}

int SafeDarken(void *zoom)
{
	__try {
		return Darken(zoom);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		g_enabled = false;
		return -2;
	}
}

void __fastcall NoZoomNotify(void *) {}
void *g_fakeVtable[64];
void *g_fakeZoom[2] = {g_fakeVtable, nullptr};

void __fastcall HookCenterOn(void *view, void *edx, int *pos, int *x)
{
	void **global = reinterpret_cast<void **>(addr::kZoomMapGlobal);
	if (*global) {
		g_origCenterOn(view, edx, pos, x);
		return;
	}
	static bool filled = false;
	if (!filled) {
		for (void *&v : g_fakeVtable) v = reinterpret_cast<void *>(&NoZoomNotify);
		filled = true;
	}
	static int logged = 0;
	if (logged++ < 3) LogD("game", "camera: centering before the zoom map exists (returning to a game in progress); crash avoided");
	*global = g_fakeZoom;
	__try {
		g_origCenterOn(view, edx, pos, x);
	}
	__finally {
		*global = nullptr;
	}
}

void *__fastcall HookRebuild(void *self)
{
	if (!g_forcing) {
		g_zoomThis = self;
		g_lastNatural = GetTickCount();
	}
	void *r = g_origRebuild(self);
	if (!g_enabled || r != nullptr) return r;
	const DWORD t0 = GetTickCount();
	const int asked = SafeDarken(self);
	if (asked == -2) LogE("game", "map fog: ERROR, disabled");
	else if (asked > 0 && g_calls++ < 20) LogD("game", "map fog: %d points queried, %d explored, %d in view, %lu ms", asked, g_exploredN, g_visibleN, GetTickCount() - t0);
	return r;
}

void *ForceRebuild(void *self)
{
	__try {
		return HookRebuild(self);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		g_zoomThis = nullptr;
		return nullptr;
	}
}

}

std::atomic<int> g_images{0};

void InvalidateZoomWindow(void *zoom)
{
	char *dlg = *reinterpret_cast<char **>(addr::kZoomDialog);
	if (!dlg) return;
	const int *vb = *reinterpret_cast<const int **>(dlg);
	char *win = dlg + vb[2];
	reinterpret_cast<void(__fastcall *)(void *, void *, int, int)>(addr::kInvalidateRect)(win, nullptr, 0, 1);
}

void ZoomFogNoteImage() { g_images++; }

long long ZoomFogGameThreadTick()
{
	if (!g_zoomThis || !g_origRebuild || g_refreshMs == 0 || g_forcing) return 0;
	static LARGE_INTEGER freq = {}, lastForced = {}, statStart = {};
	static int forced = 0, logged = 0, imagesAtStart = 0;
	static unsigned long long spent = 0;
	if (!freq.QuadPart) QueryPerformanceFrequency(&freq);
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	const DWORD tick = GetTickCount();
	if (tick - g_lastNatural > 1200) {
		statStart.QuadPart = 0;
		return 0;
	}
	if (!statStart.QuadPart) {
		statStart = now;
		forced = 0;
		spent = 0;
		imagesAtStart = g_images;
	}
	const long long elapsedUs = (now.QuadPart - statStart.QuadPart) * 1000000 / freq.QuadPart;
	if (elapsedUs >= 2000000 && logged < 12) {
		logged++;
		LogD("game", "zoom map open: %d game images per second, %d forced rebuilds per second, %.1f ms each", static_cast<int>((g_images - imagesAtStart) * 1000000LL / elapsedUs), static_cast<int>(forced * 1000000LL / elapsedUs), forced ? static_cast<double>(spent) / forced / 1000.0 : 0.0);
		statStart = now;
		forced = 0;
		spent = 0;
		imagesAtStart = g_images;
	}
	if ((now.QuadPart - lastForced.QuadPart) * 1000 / freq.QuadPart < static_cast<long long>(g_refreshMs)) return 0;
	lastForced = now;
	g_forcing = true;
	ForceRebuild(g_zoomThis);
	if (g_zoomThis) {
		__try {
			InvalidateZoomWindow(g_zoomThis);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
		}
	}
	g_forcing = false;
	LARGE_INTEGER b;
	QueryPerformanceCounter(&b);
	forced++;
	spent += static_cast<unsigned long long>((b.QuadPart - now.QuadPart) * 1000000 / freq.QuadPart);
	return b.QuadPart - now.QuadPart;
}

void InstallZoomFog(bool knownExe)
{
	if (!knownExe) return;
	char path[MAX_PATH] = {};
	GetModuleFileNameA(nullptr, path, MAX_PATH);
	if (char *slash = strrchr(path, '\\')) strcpy_s(slash + 1, MAX_PATH - (slash + 1 - path), "imperivm-mod.ini");
	g_enabled = GetPrivateProfileIntA("Map", "FogVisible", 1, path) != 0;
	g_refreshMs = static_cast<DWORD>(std::max(0, std::min(1000, static_cast<int>(GetPrivateProfileIntA("Map", "RefreshMs", 16, path)))));
	const bool ok = HookCreate(reinterpret_cast<void *>(addr::kRebuild), reinterpret_cast<void *>(&HookRebuild), reinterpret_cast<void **>(&g_origRebuild)) == MH_OK &&
		HookEnable(reinterpret_cast<void *>(addr::kRebuild)) == MH_OK;
	const bool okCenter = HookCreate(reinterpret_cast<void *>(addr::kCenterOn), reinterpret_cast<void *>(&HookCenterOn), reinterpret_cast<void **>(&g_origCenterOn)) == MH_OK &&
		HookEnable(reinterpret_cast<void *>(addr::kCenterOn)) == MH_OK;
	LogAt(okCenter ? kLogInfo : kLogError, "game", "camera: CenterOn protection without zoom map %s", okCenter ? "ready" : "NOT available");
	LogAt(ok ? kLogInfo : kLogError, "game", "zoom map fog: %s", ok ? (g_enabled ? "ready" : "disabled in the .ini") : "NOT available");
}

bool ZoomBitmapSize(int *width, int *height)
{
	void *zoom = g_zoomThis;
	if (!zoom) return false;
	__try {
		char *bitmap = At<char *>(zoom, 0x14);
		if (!bitmap) return false;
		*width = At<int>(bitmap, 8);
		*height = At<int>(bitmap, 0xc);
		return *width > 0 && *height > 0 && *width < 16384 && *height < 16384;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

bool ZoomPixelToWorld(int px, int py, int *world)
{
	void *zoom = g_zoomThis;
	if (!zoom) return false;
	__try {
		PixelToWorld(zoom, px, py, world);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}
