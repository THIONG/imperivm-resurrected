// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "hooks/hookcount.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "core/log.h"

namespace {

std::mutex g_lock;
int g_created = 0;
int g_enabled = 0;
std::vector<std::string> g_failed;

void Fail(const char *what, LPVOID target, MH_STATUS status)
{
	char text[80];
	sprintf_s(text, "%s 0x%08x (%s)", what, static_cast<unsigned>(reinterpret_cast<uintptr_t>(target)), MH_StatusToString(status));
	std::lock_guard<std::mutex> lock(g_lock);
	g_failed.push_back(text);
}

}

MH_STATUS HookCreate(LPVOID target, LPVOID detour, LPVOID *original)
{
	const MH_STATUS status = MH_CreateHook(target, detour, original);
	if (status == MH_OK) {
		std::lock_guard<std::mutex> lock(g_lock);
		g_created++;
	}
	else {
		Fail("create", target, status);
	}
	return status;
}

MH_STATUS HookEnable(LPVOID target)
{
	const MH_STATUS status = MH_EnableHook(target);
	if (status == MH_OK) {
		std::lock_guard<std::mutex> lock(g_lock);
		g_enabled++;
	}
	else {
		Fail("enable", target, status);
	}
	return status;
}

void LogHookSummary()
{
	std::lock_guard<std::mutex> lock(g_lock);
	std::string failed;
	for (const std::string &f : g_failed) failed += (failed.empty() ? "" : ", ") + f;
	LogAt(g_failed.empty() ? kLogInfo : kLogError, "boot", "hooks: %d created, %d enabled, %u failed%s%s", g_created, g_enabled,
		static_cast<unsigned>(g_failed.size()), failed.empty() ? "" : ": ", failed.c_str());
}
