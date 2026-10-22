// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>
#include <vector>

struct MapPackFacts {
	bool rules = false;
	bool rleTail = false;
	size_t forbidden = 0;
	size_t patchValues = 0;
	size_t hidden = 0;
};

bool MapPackIsSimple(const MapPackFacts &facts, const std::vector<std::string> &paths);
bool PackLoadsWithoutRestart(const MapPackFacts &facts, const std::vector<std::string> &paths);
