// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>
#include <vector>

struct SeasonDecorPair {
	unsigned base = 0;
	unsigned twin = 0;
};

std::vector<SeasonDecorPair> SeasonDecorPairs(const std::string &decorsIni, int season);
void InstallSeasonDecors(bool knownExe);
