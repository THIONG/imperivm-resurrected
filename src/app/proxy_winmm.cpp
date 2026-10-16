// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>

#include "core/log.h"

#define X(idx, ord, name) +1
static const int kExportCount = 0
#include "app/winmm_exports.inc"
	;
#undef X

extern "C" FARPROC g_winmmProcs[kExportCount] = {};

#define X(idx, ord, name)                                                         \
	extern "C" __declspec(naked) void proxy_##idx() {                             \
		__asm { jmp dword ptr [g_winmmProcs + (idx) * 4] }                        \
	}
#include "app/winmm_exports.inc"
#undef X

bool LoadRealWinmm()
{
	wchar_t path[MAX_PATH];
	GetSystemDirectoryW(path, MAX_PATH);
	lstrcatW(path, L"\\winmm.dll");
	HMODULE real = LoadLibraryW(path);
	if (!real) {
		return false;
	}
	int missing = 0;
#define X(idx, ord, name)                                                                 \
	g_winmmProcs[idx] = name ? GetProcAddress(real, name)                                 \
	                         : GetProcAddress(real, MAKEINTRESOURCEA(ord));               \
	if (!g_winmmProcs[idx]) missing++;
#include "app/winmm_exports.inc"
#undef X
	if (missing) {
		LogW("boot", "winmm: %d exports not found in the system DLL", missing);
	}
	return true;
}
