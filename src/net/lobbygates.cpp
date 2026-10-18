// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>

#include <cstdint>

#include "net/lobbygates.h"
#include "MinHook.h"
#include "hooks/hookcount.h"
#include "core/log.h"
#include "addr/net.h"

namespace {
typedef void(__fastcall *ShellDtor_t)(void *shell);
ShellDtor_t g_origShellDtor = nullptr;

const unsigned kShellDatagramGate = 0x40;
const unsigned kGateSinkKind = 0x10;
const unsigned kGateSinkObject = 0x14;
const char kSinkObject = 1;

bool Detach(char *gate, void *shell)
{
	if (!gate || gate[kGateSinkKind] != kSinkObject || *reinterpret_cast<void **>(gate + kGateSinkObject) != shell) return false;
	gate[kGateSinkKind] = 0;
	return true;
}

int DetachAll(void *shell)
{
	int detached = 0;
	__try {
		char **it = *reinterpret_cast<char ***>(addr::kTcpGateListBegin);
		char **end = *reinterpret_cast<char ***>(addr::kTcpGateListEnd);
		for (; it && it != end; ++it) detached += Detach(*it, shell);
		detached += Detach(*reinterpret_cast<char **>(static_cast<char *>(shell) + kShellDatagramGate), shell);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return -1;
	}
	return detached;
}

void __fastcall HookShellDtor(void *shell)
{
	const int detached = DetachAll(shell);
	if (detached < 0) LogW("net", "lobby: the closing room could not be detached from its connections");
	else if (detached) LogD("net", "lobby: the room closes; %d of its connections stop reporting to it", detached);
	g_origShellDtor(shell);
}
}

void InstallLobbyGates(bool knownExe)
{
	if (!knownExe) return;
	void *dtor = reinterpret_cast<void *>(addr::kShellDtor);
	if (HookCreate(dtor, reinterpret_cast<void *>(&HookShellDtor), reinterpret_cast<void **>(&g_origShellDtor)) != MH_OK || HookEnable(dtor) != MH_OK) {
		LogE("net", "lobby: could not hook the room's network session; a crash of the original game at game start stays possible");
		return;
	}
	LogI("net", "lobby: a closed room no longer receives events from its player connections");
}
