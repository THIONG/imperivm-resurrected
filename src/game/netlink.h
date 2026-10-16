// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once

struct NetLink {
	bool (*multiplayerSaveLoading)() = nullptr;
	bool (*rejoinLoading)() = nullptr;
	unsigned (*roomRules)() = nullptr;
	bool (*isLockstepHost)() = nullptr;
};

void NetLinkSet(const NetLink &link);

bool NetMultiplayerSaveLoading();
bool NetRejoinLoading();
unsigned NetRoomRules();
bool NetIsLockstepHost();
