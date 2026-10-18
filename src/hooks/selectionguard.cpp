// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>

#include "hooks/selectionguard.h"
#include "MinHook.h"
#include "hooks/hookcount.h"
#include "core/log.h"
#include "addr/script.h"
#include "addr/ui.h"

namespace {
typedef void(__fastcall *RebuildCommandList_t)(char *manager);
typedef void(__fastcall *SelectionRevalidate_t)(char *selection);
RebuildCommandList_t g_origRebuild = nullptr;

const unsigned kEffectiveSelection = 0xf0;
const unsigned kSelectionHead = 0x18;
const unsigned kSelectionSize = 0x1c;
const unsigned kNodeHandle = 8;

int GoneObjects(char *selection)
{
	char **table = reinterpret_cast<char **>(addr::kHandleTable);
	char *head = *reinterpret_cast<char **>(selection + kSelectionHead);
	const unsigned size = *reinterpret_cast<unsigned *>(selection + kSelectionSize);
	if (!head) return 0;
	int gone = 0;
	unsigned seen = 0;
	for (char *node = *reinterpret_cast<char **>(head); node && node != head && seen <= size; node = *reinterpret_cast<char **>(node), seen++) {
		if (!table[*reinterpret_cast<unsigned short *>(node + kNodeHandle)]) gone++;
	}
	return gone;
}

void __fastcall HookRebuildCommandList(char *manager)
{
	if (manager) {
		char *selection = manager + kEffectiveSelection;
		const int gone = GoneObjects(selection);
		if (gone) {
			LogW("game", "selection: %d selected objects no longer exist; the selection is checked again before its commands are rebuilt", gone);
			reinterpret_cast<SelectionRevalidate_t>(addr::kSelectionRevalidate)(selection);
		}
	}
	g_origRebuild(manager);
}
}

void InstallSelectionGuard(bool knownExe)
{
	if (!knownExe) return;
	void *target = reinterpret_cast<void *>(addr::kRebuildCommandList);
	if (HookCreate(target, reinterpret_cast<void *>(&HookRebuildCommandList), reinterpret_cast<void **>(&g_origRebuild)) != MH_OK ||
		HookEnable(target) != MH_OK) {
		LogE("game", "selection: could not hook the rebuilding of the command bar; a crash of the original game when a selected object disappears stays possible");
		return;
	}
	LogD("game", "selection: objects that no longer exist are dropped before the command bar is rebuilt");
}
