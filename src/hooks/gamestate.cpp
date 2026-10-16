// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "hooks/gamestate.h"

#include <cstdint>

#include "addr/net.h"
#include "addr/state.h"

bool GameIsMultiplayer()
{
	void *game = *reinterpret_cast<void **>(addr::kGame);
	return game && *reinterpret_cast<uintptr_t *>(game) == addr::kMultiplayerVtable;
}
