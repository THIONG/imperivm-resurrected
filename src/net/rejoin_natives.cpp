// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>
#include <map>
#include "MinHook.h"
#include "core/log.h"
#include "net/steamnet.h"
#include "addr/script.h"
#include "addr/world.h"

#include "net/rejoin_internal.h"

using namespace rejoin_impl;

namespace rejoin_impl {
namespace {
struct NativeFrame {
	uint32_t index, ret;
};
thread_local NativeFrame t_nativeStack[kNativeDepth];
thread_local uint32_t t_nativeDepth = 0;

void RecordNative(uint32_t index, uint32_t result)
{
	if (!g_trace.active) return;
	char *sc = *reinterpret_cast<char **>(addr::kCurScriptable);
	if (!sc) return;
	const bool isAi = *reinterpret_cast<uint32_t *>(sc) == addr::kAiVtable;
	int tick;
	if (!Tracing(tick)) return;
	uint32_t top = 0;
	char *interp = *reinterpret_cast<char **>(addr::kCurInterpreter);
	if (interp) {
		char *base = *reinterpret_cast<char **>(interp + 0xc), *sp = *reinterpret_cast<char **>(interp + 0x10);
		if (base && sp > base) top = *reinterpret_cast<uint32_t *>(sp - 4);
	}
	char *mgr = ThreadManager(0);
	const uint32_t slot = *reinterpret_cast<uint32_t *>(addr::kCurScriptId) & 0xffff;
	const uint32_t owner = isAi ? *reinterpret_cast<uint32_t *>(sc + 0x24) : *reinterpret_cast<uint16_t *>(sc + 8);
	TracePush({tick, 'N', static_cast<uint16_t>(index), mgr ? *reinterpret_cast<uint32_t *>(mgr) : 0, top, slot | (owner << 16), result,
		g_natives.orig[index], ThreadsHash()});
}

extern "C" uint32_t __cdecl NativeBefore(uint32_t *slot);
extern "C" uint32_t __cdecl NativeAfterC(uint32_t result);

__declspec(naked) void NativeAfter()
{
	__asm {
		sub esp, 4
		push eax
		push ecx
		push edx
		push eax
		call NativeAfterC
		add esp, 4
		mov [esp + 12], eax
		pop edx
		pop ecx
		pop eax
		ret
	}
}

__declspec(naked) void NativeEntry()
{
	__asm {
		push eax
		push ecx
		push edx
		lea eax, [esp + 12]
		push eax
		call NativeBefore
		add esp, 4
		mov [esp + 12], eax
		pop edx
		pop ecx
		pop eax
		ret
	}
}

extern "C" uint32_t __cdecl NativeBefore(uint32_t *slot)
{
	const uint32_t index = slot[0];
	if (t_nativeDepth < kNativeDepth) {
		t_nativeStack[t_nativeDepth++] = {index, slot[1]};
		slot[1] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&NativeAfter));
	}
	return g_natives.orig[index];
}

extern "C" uint32_t __cdecl NativeAfterC(uint32_t result)
{
	const NativeFrame f = t_nativeStack[--t_nativeDepth];
	__try {
		RecordNative(f.index, result);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}
	return f.ret;
}
}
}

namespace rejoin_impl {
void PatchNatives()
{
	uint32_t *table = reinterpret_cast<uint32_t *>(addr::kScriptEngine);
	const uint32_t n = std::min<uint32_t>(*reinterpret_cast<uint16_t *>(addr::kScriptEngine + 0x4004), kMaxNatives);
	if (!g_natives.thunks) {
		g_natives.thunks = static_cast<uint8_t *>(VirtualAlloc(nullptr, kMaxNatives * kThunkSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		if (!g_natives.thunks) return;
	}
	DWORD old = 0;
	if (!VirtualProtect(table, n * 4, PAGE_READWRITE, &old)) return;
	unsigned patched = 0, emptySlots = 0;
	for (uint32_t i = 0; i < n; i++) {
		uint8_t *thunk = g_natives.thunks + i * kThunkSize;
		const uint32_t fn = table[i];
		if (!fn) emptySlots++;
		if (!fn || fn == static_cast<uint32_t>(reinterpret_cast<uintptr_t>(thunk))) continue;
		g_natives.orig[i] = fn;
		thunk[0] = 0x68;
		*reinterpret_cast<uint32_t *>(thunk + 1) = i;
		thunk[5] = 0xe9;
		*reinterpret_cast<int32_t *>(thunk + 6) =
			static_cast<int32_t>(reinterpret_cast<intptr_t>(&NativeEntry) - reinterpret_cast<intptr_t>(thunk + 10));
		table[i] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(thunk));
		patched++;
	}
	VirtualProtect(table, n * 4, old, &old);
	FlushInstructionCache(GetCurrentProcess(), g_natives.thunks, kMaxNatives * kThunkSize);
	if (patched) LogD("rejoin", "trace of the AI natives: %u of %u functions (%u empty table slots)", patched, n, emptySlots);
}

std::string NativeName(uint32_t index)
{
	static std::map<uint32_t, std::string> names;
	if (names.empty()) {
		char *compiler = *reinterpret_cast<char **>(addr::kScriptEngine + 0x4010);
		char *recs = nullptr;
		uint32_t count = 0;
		if (compiler && SafeCopy(&recs, compiler + 0x20, 4) && SafeCopy(&count, compiler + 0x24, 4) && recs && count < 0x2000) {
			for (uint32_t k = 0; k < count; k++) {
				char rec[0xb4];
				if (!SafeCopy(rec, recs + k * 0xb4, sizeof(rec))) break;
				rec[0x7f] = 0;
				names[*reinterpret_cast<uint16_t *>(rec + 0xb0)] = rec;
			}
		}
		if (names.empty()) names[0xffffffff] = "";
	}
	auto it = names.find(index);
	if (it != names.end()) return it->second;
	char buf[24];
	sprintf_s(buf, "native#%u(%08x)", index, g_natives.orig[index < kMaxNatives ? index : 0]);
	return buf;
}
}
