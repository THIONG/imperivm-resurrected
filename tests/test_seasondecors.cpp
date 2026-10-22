// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>

#include "game/seasondecors.h"

namespace {

const char kIni[] =
	"[tr31]\r\ntype = 1\r\nseason = spring\r\n\r\n"
	"[ElhaSmall]\r\ntype = 2\r\n\r\n"
	"[Rock]\r\ntype = 3\r\n\r\n"
	"[tr31 Winter]\r\ntype = 30\r\nseason = winter\r\n\r\n"
	"[ElhaSmall Winter]\r\ntype = 33\r\n\r\n"
	"[tr31 Autumn]\r\ntype = 40\r\n\r\n"
	"[Orphan Winter]\r\ntype = 50\r\n";

void WinterTypesReplaceTheirSpringTwins()
{
	const auto pairs = SeasonDecorPairs(kIni, 3);
	CHECK(pairs.size() == 2);
	bool tree = false, pine = false;
	for (const SeasonDecorPair &p : pairs) {
		if (p.base == 1 && p.twin == 30) tree = true;
		if (p.base == 2 && p.twin == 33) pine = true;
	}
	CHECK(tree && pine);
}

void AutumnUsesItsOwnTwins()
{
	const auto pairs = SeasonDecorPairs(kIni, 2);
	CHECK(pairs.size() == 1);
	CHECK(pairs[0].base == 1 && pairs[0].twin == 40);
}

void SpringAndUnknownSeasonsChangeNothing()
{
	CHECK(SeasonDecorPairs(kIni, 1).empty());
	CHECK(SeasonDecorPairs(kIni, 0).empty());
	CHECK(SeasonDecorPairs("", 3).empty());
}

void SectionsWithoutAValidTypeAreIgnored()
{
	const std::string ini = "[a]\ntype = 0\n[a Winter]\ntype = 9\n[b]\ntype = 300\n[b Winter]\ntype = 8\n";
	CHECK(SeasonDecorPairs(ini, 3).empty());
}

}

int main()
{
	WinterTypesReplaceTheirSpringTwins();
	AutumnUsesItsOwnTwins();
	SpringAndUnknownSeasonsChangeNothing();
	SectionsWithoutAValidTypeAreIgnored();
	return test::Summary("seasondecors");
}
