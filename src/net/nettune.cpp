// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <vector>

#include "MinHook.h"
#include "hooks/hookcount.h"
#include "core/log.h"
#include "net/nettune.h"
#include "addr/net.h"

namespace {
const unsigned kMinTickMs = 120;
const unsigned kMaxTickMs = 500;
const unsigned kTickPercent = 70;

const size_t kOffTick = 0x54;
const size_t kOffSuggested = 0x70;
const size_t kOffDelay = 0x74;
const size_t kOffMinTick = 0x3f0;
const size_t kOffMaxTick = 0x3f4;

typedef void *(__fastcall *Ctor_t)(void *self, void *edx, unsigned long a, int b, unsigned c, unsigned d);
Ctor_t g_origCtor = nullptr;
bool g_patched = false;

template <typename T>
T &At(void *base, size_t off)
{
	return *reinterpret_cast<T *>(static_cast<char *>(base) + off);
}

static_assert(addr::kDelayMultiplier == 0x0081a2fc && addr::kDelayResume == 0x00407caa, "DelayFixCave hard-codes these addresses in its assembly");

__declspec(naked) void DelayFixCave()
{
	__asm {
		sub esi, ebx
		jns nonneg
		xor esi, esi
	nonneg:
		imul esi, dword ptr ds:[0x0081A2FC]
		push 0x00407CAA
		ret
	}
}

bool PatchDelaySite()
{
	static const unsigned char expect[9] = {0x2B, 0xF3, 0x0F, 0xAF, 0x35, 0xFC, 0xA2, 0x81, 0x00};
	unsigned char *site = reinterpret_cast<unsigned char *>(addr::kDelaySite);
	if (memcmp(site, expect, sizeof(expect)) != 0) return false;
	DWORD old = 0;
	if (!VirtualProtect(site, sizeof(expect), PAGE_EXECUTE_READWRITE, &old)) return false;
	const int32_t rel = static_cast<int32_t>(reinterpret_cast<uintptr_t>(&DelayFixCave) - (addr::kDelaySite + 5));
	site[0] = 0xE9;
	memcpy(site + 1, &rel, sizeof(rel));
	memset(site + 5, 0x90, 4);
	VirtualProtect(site, sizeof(expect), old, &old);
	FlushInstructionCache(GetCurrentProcess(), site, sizeof(expect));
	return true;
}

void *__fastcall HookCtor(void *self, void *edx, unsigned long a, int b, unsigned c, unsigned d)
{
	void *r = g_origCtor(self, edx, a, b, c, d);
	if (self && g_patched) {
		const unsigned oldMin = At<unsigned>(self, kOffMinTick), oldMax = At<unsigned>(self, kOffMaxTick);
		const unsigned oldPercent = *reinterpret_cast<unsigned *>(addr::kTickPercentAddr);
		At<unsigned>(self, kOffMinTick) = kMinTickMs;
		At<unsigned>(self, kOffMaxTick) = kMaxTickMs;
		*reinterpret_cast<unsigned *>(addr::kTickPercentAddr) = kTickPercent;
		LogI("net", "game network: network turns adjusted (minimum %u ms, maximum %u ms, margin %u %%; the game had %u, %u and %u %%)", kMinTickMs,
			kMaxTickMs, kTickPercent, oldMin, oldMax, oldPercent);
	}
	return r;
}

LARGE_INTEGER g_freq = {};
LARGE_INTEGER g_last = {};
unsigned g_lastTick = 0;
bool g_haveLast = false;

struct Window {
	std::vector<unsigned> intervals;
	double suggestedSum = 0, delaySum = 0;
	unsigned suggestedMax = 0, slow = 0;
	DWORD since = 0;
};
Window g_win;

std::mutex g_secMx;
unsigned g_secCount = 0, g_secSum = 0, g_secMax = 0;

void FlushWindow()
{
	if (g_win.intervals.empty()) {
		g_win = Window();
		return;
	}
	std::vector<unsigned> v = g_win.intervals;
	std::sort(v.begin(), v.end());
	double sum = 0;
	for (unsigned x : v) sum += x;
	const size_t n = v.size();
	LogI("net", "game network: %u turns in %u s; mean interval %.0f ms (median %u, p90 %u, max %u); mean turn size "
		"%.0f ms (max %u), mean added delay %.0f ms; turns slower than expected: %u",
		static_cast<unsigned>(n), (GetTickCount() - g_win.since) / 1000, sum / n, v[n / 2], v[std::min(n - 1, n * 9 / 10)], v[n - 1],
		g_win.suggestedSum / n, g_win.suggestedMax, g_win.delaySum / n, g_win.slow);
	g_win = Window();
}

}

void InstallNetTune(bool knownExe)
{
	if (!knownExe) return;
	g_patched = PatchDelaySite();
	const bool hooked = HookCreate(reinterpret_cast<void *>(addr::kTransportCtor), reinterpret_cast<void *>(&HookCtor),
							reinterpret_cast<void **>(&g_origCtor)) == MH_OK &&
		HookEnable(reinterpret_cast<void *>(addr::kTransportCtor)) == MH_OK;
	QueryPerformanceFrequency(&g_freq);
	LogAt(g_patched && hooked ? kLogInfo : kLogError, "net", "game network: turn adjustment %s", g_patched && hooked ? "ready" : "NOT available");
	if (!hooked) g_patched = false;
}

NetTurnSecond NetTuneTakeSecond()
{
	std::lock_guard<std::mutex> lock(g_secMx);
	NetTurnSecond out;
	out.count = static_cast<int>(g_secCount);
	if (g_secCount) {
		out.avgMs = static_cast<int>((g_secSum + g_secCount / 2) / g_secCount);
		out.maxMs = static_cast<int>(g_secMax);
	}
	g_secCount = g_secSum = g_secMax = 0;
	return out;
}

void NetTuneAfterAdvance(void *transport)
{
	if (!transport || !g_freq.QuadPart) return;
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	const unsigned tick = At<unsigned>(transport, kOffTick);
	if (!g_haveLast || tick != g_lastTick + 1) {
		FlushWindow();
		g_win.since = GetTickCount();
	}
	else {
		const double ms = static_cast<double>(now.QuadPart - g_last.QuadPart) * 1000.0 / static_cast<double>(g_freq.QuadPart);
		const unsigned interval = static_cast<unsigned>(ms + 0.5);
		const unsigned suggested = At<unsigned>(transport, kOffSuggested), delay = At<unsigned>(transport, kOffDelay);
		if (suggested < 5000 && delay < 5000 && interval < 60000) {
			g_win.intervals.push_back(interval);
			{
				std::lock_guard<std::mutex> lock(g_secMx);
				g_secCount++;
				g_secSum += interval;
				g_secMax = std::max(g_secMax, interval);
			}
			g_win.suggestedSum += suggested;
			g_win.delaySum += delay;
			g_win.suggestedMax = std::max(g_win.suggestedMax, suggested);
			if (interval > (suggested + delay) * 3 / 2 + 30) g_win.slow++;
		}
		if (GetTickCount() - g_win.since >= 30000) {
			FlushWindow();
			g_win.since = GetTickCount();
		}
	}
	g_last = now;
	g_lastTick = tick;
	g_haveLast = true;
}
