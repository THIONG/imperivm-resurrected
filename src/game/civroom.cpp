// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "game/civroom.h"

#include <algorithm>

namespace {
const int kComboRace[9] = {1, 4, 0, 3, 2, 7, 6, 5, 8};
const int kFirstModItem = 10;
}

std::vector<int> CivRoomOrder(const std::vector<RoomCiv> &civs, int items)
{
	std::vector<int> order;
	if (civs.empty() || items < kFirstModItem + static_cast<int>(civs.size())) {
		for (int i = 0; i < items; i++) order.push_back(i);
		return order;
	}
	std::vector<bool> placed(civs.size(), false);
	for (int item = 0; item < 9; item++) {
		bool replaced = false;
		for (size_t i = 0; i < civs.size(); i++) {
			if (!civs[i].replacesBase || civs[i].base != kComboRace[item]) continue;
			order.push_back(kFirstModItem + static_cast<int>(i));
			placed[i] = true;
			replaced = true;
		}
		if (!replaced) order.push_back(item);
	}
	for (size_t i = 0; i < civs.size(); i++) {
		if (!placed[i]) order.push_back(kFirstModItem + static_cast<int>(i));
	}
	return order;
}

int CivRoomReplacement(const std::vector<RoomCiv> &civs, int item, int items)
{
	if (item < 0 || item >= 9) return -1;
	const std::vector<int> order = CivRoomOrder(civs, items);
	if (std::find(order.begin(), order.end(), item) != order.end()) return -1;
	for (size_t i = 0; i < civs.size(); i++) {
		if (civs[i].replacesBase && civs[i].base == kComboRace[item]) return kFirstModItem + static_cast<int>(i);
	}
	return -1;
}
