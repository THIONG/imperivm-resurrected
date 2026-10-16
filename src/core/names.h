// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cctype>
#include <string>

inline std::string LowerName(std::string name)
{
	for (char &c : name) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
	return name;
}

inline bool SamePlayerName(const std::string &a, const std::string &b)
{
	return LowerName(a) == LowerName(b);
}

inline bool PlayerNameMatches(const std::string &a, const std::string &b)
{
	const std::string x = LowerName(a), y = LowerName(b);
	if (x == y) return true;
	const std::string &shorter = x.size() < y.size() ? x : y;
	const std::string &longer = x.size() < y.size() ? y : x;
	return shorter.size() >= 8 && longer.compare(0, shorter.size(), shorter) == 0;
}
