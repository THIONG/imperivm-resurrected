// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include "MinHook.h"

MH_STATUS HookCreate(LPVOID target, LPVOID detour, LPVOID *original);

MH_STATUS HookEnable(LPVOID target);

void LogHookSummary();
