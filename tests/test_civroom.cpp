// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <vector>

#include "game/civroom.h"

namespace {

const int kRepublicanRome = 1;
const int kGaul = 0;

std::vector<int> Range(int n)
{
	std::vector<int> v;
	for (int i = 0; i < n; i++) v.push_back(i);
	return v;
}

void WithoutModCivs()
{
	CHECK(CivRoomOrder({}, 9) == Range(9));
	CHECK(CivRoomOrder({{kGaul, false}}, 9) == Range(9));
	CHECK_EQ(CivRoomReplacement({}, 0, 9), -1);
}

void ModCivsGoLast()
{
	const std::vector<RoomCiv> civs = {{kGaul, false}, {kRepublicanRome, false}};
	const std::vector<int> expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11};
	CHECK(CivRoomOrder(civs, 12) == expected);
	CHECK_EQ(CivRoomReplacement(civs, 0, 12), -1);
}

void ReplacedBaseGivesItsPlace()
{
	const std::vector<RoomCiv> civs = {{kGaul, false}, {kRepublicanRome, true}, {kRepublicanRome, true}, {kRepublicanRome, true}};
	const std::vector<int> expected = {11, 12, 13, 1, 2, 3, 4, 5, 6, 7, 8, 10};
	CHECK(CivRoomOrder(civs, 14) == expected);
	CHECK_EQ(CivRoomReplacement(civs, 0, 14), 11);
	CHECK_EQ(CivRoomReplacement(civs, 1, 14), -1);
	CHECK_EQ(CivRoomReplacement(civs, 8, 14), -1);
	CHECK_EQ(CivRoomReplacement(civs, 11, 14), -1);
	CHECK_EQ(CivRoomReplacement(civs, -1, 14), -1);
}

void GaulReplacedInTheMiddle()
{
	const std::vector<RoomCiv> civs = {{kGaul, true}};
	const std::vector<int> expected = {0, 1, 10, 3, 4, 5, 6, 7, 8};
	CHECK(CivRoomOrder(civs, 11) == expected);
	CHECK_EQ(CivRoomReplacement(civs, 2, 11), 10);
}

void ComboWithoutTheModItems()
{
	const std::vector<RoomCiv> civs = {{kRepublicanRome, true}};
	CHECK(CivRoomOrder(civs, 9) == Range(9));
	CHECK_EQ(CivRoomReplacement(civs, 0, 9), -1);
}

}

int main()
{
	WithoutModCivs();
	ModCivsGoLast();
	ReplacedBaseGivesItsPlace();
	GaulReplacedInTheMiddle();
	ComboWithoutTheModItems();
	return test::Summary("civroom");
}
