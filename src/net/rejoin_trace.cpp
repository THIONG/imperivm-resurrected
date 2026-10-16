// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>
#include "MinHook.h"
#include "core/guard.h"
#include "net/steamnet.h"
#include "addr/net.h"

#include "net/rejoin_internal.h"

using namespace rejoin_impl;

namespace rejoin_impl {
char *ThreadManager(int m)
{
	char *game = static_cast<char *>(Game());
	return game ? *reinterpret_cast<char **>(game + kGameThreadManagers[m]) : nullptr;
}

}

namespace rejoin_impl {
namespace {
const int kNativeTicksWritten = 6;
}
}

namespace rejoin_impl {
TraceState g_trace;
}

namespace rejoin_impl {
uint32_t ThreadsHash() { return *reinterpret_cast<uint32_t *>(addr::kSyncState + 0xc); }

void TraceStart(int)
{
	PatchNatives();
	if (g_trace.ring.size() != kRingSize) g_trace.ring.assign(kRingSize, TraceRec{});
	g_trace.ringNext = g_trace.ringCount = 0;
	g_trace.active = true;
	g_trace.lastTick = -1;
}

void TracePush(const TraceRec &r)
{
	g_trace.ring[g_trace.ringNext] = r;
	g_trace.ringNext = (g_trace.ringNext + 1) % kRingSize;
	if (g_trace.ringCount < kRingSize) g_trace.ringCount++;
}

bool Tracing(int &tick)
{
	if (!g_trace.active) return false;
	if (!Game()) {
		g_trace.active = false;
		return false;
	}
	char *t = Transport();
	tick = t ? *reinterpret_cast<int *>(t + 0x54) : -1;
	return true;
}

bool WriteTrace(int from)
{
	EnsureLogDir();
	FILE *f = _fsopen(DiskPath(kTraceFile).c_str(), "wb", _SH_DENYWR);
	if (!f) return false;
	const size_t first = (g_trace.ringNext + kRingSize - g_trace.ringCount) % kRingSize;
	const int lastTick = g_trace.ringCount ? g_trace.ring[(g_trace.ringNext + kRingSize - 1) % kRingSize].tick : 0;
	for (size_t k = 0; k < g_trace.ringCount; k++) {
		const TraceRec &r = g_trace.ring[(first + k) % kRingSize];
		if (r.tick < from) continue;
		switch (r.kind) {
		case 'N':
			if (r.tick < lastTick - kNativeTicksWritten) break;
			fprintf(f, "%d N t=%u ai=%u script=%u %s -> %d (stack %d)\n", r.tick, r.t, r.id >> 16, r.id & 0xffff, NativeName(r.h).c_str(),
				static_cast<int>(r.due), static_cast<int>(r.vt));
			break;
		case 'T':
			fprintf(f, "== tick %d, threads %08x\n", r.tick, r.hash);
			break;
		case 'G':
			fprintf(f, "-- continue (tick %d, threads %08x)\n", r.tick, r.hash);
			break;
		case 'A':
			fprintf(f, "%d A t=%u [%d,%d]=%d from=%08x seed=%08x\n", r.tick, r.t, static_cast<int>(r.due), static_cast<int>(r.vt),
				static_cast<int>(r.id), r.from, r.hash);
			break;
		case 'U':
			fprintf(f, "%d U t=%u ai=%u commands=%u active=%u busy=%u launches=%u state=%08x\n", r.tick, r.t, r.h, r.id & 0xffff, r.id >> 16,
				r.from ? 1 : 0, r.vt, r.due);
			break;
		case 'O':
			fprintf(f, "%d O t=%u ai=%u type=%d prio=%d a=%u b=%u from=%08x\n", r.tick, r.t, r.h, static_cast<int>(r.id), static_cast<int>(r.vt),
				r.due & 0xffff, r.due >> 16, r.from);
			break;
		default:
			fprintf(f, "%d %c t=%u h=%u vt=%08x id=%u due=%u from=%08x hash=%08x\n", r.tick, r.kind, r.t, r.h, r.vt, r.id, r.due, r.from,
				r.hash);
		}
	}
	WriteAiState(f);
	fclose(f);
	return true;
}

}

namespace rejoin_impl {
namespace {
void TraceThreadEvent(char kind, void *owner, uint32_t id, uint32_t due, void *from)
{
	int tick;
	if (!owner || !Tracing(tick)) return;
	char *mgr = *reinterpret_cast<char **>(static_cast<char *>(owner) + 0x18);
	if (!mgr || mgr != ThreadManager(0)) return;
	TracePush({tick, kind, *reinterpret_cast<uint16_t *>(static_cast<char *>(owner) + 8), *reinterpret_cast<uint32_t *>(mgr),
		*static_cast<uint32_t *>(owner), id, due, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(from)), ThreadsHash()});
}
}
}

namespace rejoin_impl {
void *__fastcall HookStartThread(void *owner, void *edx, uint32_t id, uint32_t due)
{
	TraceThreadEvent('S', owner, id, due, _ReturnAddress());
	return g_orig.startThread(owner, edx, id, due);
}


void *__fastcall HookRescheduleThread(void *owner, void *edx, uint32_t *thread, uint32_t due)
{
	TraceThreadEvent('R', owner, thread ? thread[4] : 0, due, _ReturnAddress());
	return g_orig.reschedule(owner, edx, thread, due);
}

void *__fastcall HookStopThread(void *owner, void *edx, uint32_t id)
{
	TraceThreadEvent('P', owner, id, 0xffffffff, _ReturnAddress());
	return g_orig.stopThread(owner, edx, id);
}


int __fastcall HookRandomRange(void *rng, void *edx, int lo, int hi)
{
	const uint32_t seed = *reinterpret_cast<uint32_t *>(static_cast<char *>(rng) + 4);
	const int r = g_orig.randomRange(rng, edx, lo, hi);
	int tick;
	char *game = static_cast<char *>(Game());
	if (game && rng == game + 0x12a0 && Tracing(tick)) {
		char *mgr = ThreadManager(0);
		TracePush({tick, 'A', 0, mgr ? *reinterpret_cast<uint32_t *>(mgr) : 0, static_cast<uint32_t>(hi), static_cast<uint32_t>(r),
			static_cast<uint32_t>(lo), static_cast<uint32_t>(reinterpret_cast<uintptr_t>(_ReturnAddress())), seed});
	}
	return r;
}
}
