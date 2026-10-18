// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <vector>

struct RoomCiv {
	int base = 0;
	bool replacesBase = false;
};

std::vector<int> CivRoomOrder(const std::vector<RoomCiv> &civs, int items);
int CivRoomReplacement(const std::vector<RoomCiv> &civs, int item, int items);
