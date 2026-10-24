// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>
#include <vector>

#include "mods/mappack.h"

namespace {

void OnlyScenariosArePackedMaps()
{
	const MapPackFacts content;
	CHECK(MapPackIsSimple(content, {"scenarios/island.bfhp"}));
	CHECK(MapPackIsSimple(content, {"scenarios/island.bfhp", "scenarios/island.bmp"}));
	CHECK(MapPackIsSimple(content, {"Scenarios/Island.BFHP", "readme", "notes.txt", "thumbs.db"}));
	CHECK(MapPackIsSimple(content, {"scenarios/a.bfhp", "scenarios/b.bfhp"}));
}

void AnythingElseNeedsTheWholePack()
{
	const MapPackFacts content;
	CHECK(!MapPackIsSimple(content, {}));
	CHECK(!MapPackIsSimple(content, {"readme"}));
	CHECK(!MapPackIsSimple(content, {"scenarios/island.bmp"}));
	CHECK(!MapPackIsSimple(content, {"scenarios/island.bfhp", "movies/intro.avi"}));
	CHECK(!MapPackIsSimple(content, {"scenarios/island.bfhp", "civs/greece.json"}));
	CHECK(!MapPackIsSimple(content, {"scenarios/island.bfhp", "data/ai/ts_gaultactic.vs"}));
	CHECK(!MapPackIsSimple(content, {"scenarios/island.bfhp", "packs/randommap.pak"}));
	CHECK(!MapPackIsSimple(content, {"scenarios/island.bfhp", "gameres/units/x.bmp"}));
	CHECK(!MapPackIsSimple(content, {"scenarios/sub/island.bfhp"}));
	CHECK(!MapPackIsSimple(content, {"adventures/trip.bfhp"}));
	CHECK(!MapPackIsSimple(content, {"scenarios/island.bfhp", "config.ini"}));
}

void TheFactsOfThePackCountToo()
{
	const std::vector<std::string> map = {"scenarios/island.bfhp"};
	MapPackFacts facts;
	CHECK(MapPackIsSimple(facts, map));
	facts.rules = true;
	CHECK(!MapPackIsSimple(facts, map));
	facts = MapPackFacts();
	facts.rleTail = true;
	CHECK(!MapPackIsSimple(facts, map));
	facts = MapPackFacts();
	facts.forbidden = 1;
	CHECK(!MapPackIsSimple(facts, map));
	facts = MapPackFacts();
	facts.patchValues = 2;
	CHECK(!MapPackIsSimple(facts, map));
	facts = MapPackFacts();
	facts.hidden = 1;
	CHECK(!MapPackIsSimple(facts, map));
}

void ScenarioAdventureAndConquestPacksNeedNoRestart()
{
	MapPackFacts none;
	CHECK(PackLoadsWithoutRestart(none, {"scenarios/island.bfhp", "scenarios/island.bmp", "readme map.txt"}));
	CHECK(PackLoadsWithoutRestart(none, {"adventures/greatbattles/one.bfhp", "adventures/greatbattles/one.bmp"}));
	CHECK(PackLoadsWithoutRestart(none, {"conquests/world.bfhp", "conquestmaps/1 - europe/map.bmp"}));
	CHECK(PackLoadsWithoutRestart(none, {"Scenarios/Island.BFHP", "Adventures/A.bfhp"}));
	CHECK(PackLoadsWithoutRestart(none, {}));
	CHECK(!PackLoadsWithoutRestart(none, {"scenarios/island.bfhp", "data/units.xml"}));
	CHECK(!PackLoadsWithoutRestart(none, {"packs/units.pak"}));
	MapPackFacts rules;
	rules.rules = true;
	CHECK(!PackLoadsWithoutRestart(rules, {"scenarios/island.bfhp"}));
	MapPackFacts patched;
	patched.patchValues = 1;
	CHECK(!PackLoadsWithoutRestart(patched, {"adventures/a.bfhp"}));
}

}

int main()
{
	ScenarioAdventureAndConquestPacksNeedNoRestart();
	OnlyScenariosArePackedMaps();
	AnythingElseNeedsTheWholePack();
	TheFactsOfThePackCountToo();
	return test::Summary("mappack");
}
