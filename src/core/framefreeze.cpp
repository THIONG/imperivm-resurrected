// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "core/framefreeze.h"

#include <windows.h>
#include <atomic>

namespace {

const DWORD kMaxMs = 30000;
std::atomic<DWORD> g_until{0};

}

void FrameFreeze(bool on)
{
	g_until = on ? (GetTickCount() + kMaxMs) | 1 : 0;
}

bool FrameFrozen()
{
	const DWORD until = g_until;
	return until && static_cast<int>(GetTickCount() - until) < 0;
}
