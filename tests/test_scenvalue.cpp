// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>

#include "net/scenvalue.h"

namespace {

void LooseMapKeepsTheOldFormat()
{
	ScenValue v;
	v.path = "Scenarios/Island";
	v.size = 1234;
	v.hash = "00ff";
	CHECK_EQ(ScenValueEncode(v), std::string("Scenarios/Island\t1234\t00ff"));
	ScenValue back;
	CHECK(ScenValueParse(ScenValueEncode(v), back));
	CHECK_EQ(back.path, v.path);
	CHECK_EQ(back.size, static_cast<uint64_t>(1234));
	CHECK_EQ(back.hash, std::string("00ff"));
	CHECK(back.modId.empty());
	CHECK(!back.simple);
}

void PackMapKeepsTheOldFormatToo()
{
	ScenValue v;
	v.path = "Scenarios/Island";
	v.modId = "gamebanana-728865";
	v.modVersion = "2026.10.17";
	v.modName = "Batalla de islas";
	CHECK_EQ(ScenValueEncode(v), std::string("Scenarios/Island\t0\t\tgamebanana-728865\t2026.10.17\tBatalla de islas"));
	ScenValue back;
	CHECK(ScenValueParse(ScenValueEncode(v), back));
	CHECK_EQ(back.modId, v.modId);
	CHECK_EQ(back.modVersion, v.modVersion);
	CHECK_EQ(back.modName, v.modName);
	CHECK(!back.simple);
	CHECK_EQ(back.size, static_cast<uint64_t>(0));
}

void SimplePackMapCarriesTheFileAndTheSource()
{
	ScenValue v;
	v.path = "Scenarios/Island";
	v.modId = "gamebanana-728865";
	v.modVersion = "2026.10.17";
	v.modName = "Batalla\tde islas";
	v.simple = true;
	v.mapSize = 3797087;
	v.mapHash = "abcdef0123456789";
	v.gbId = 728865;
	ScenValue back;
	CHECK(ScenValueParse(ScenValueEncode(v), back));
	CHECK(back.simple);
	CHECK_EQ(back.mapSize, static_cast<uint64_t>(3797087));
	CHECK_EQ(back.mapHash, std::string("abcdef0123456789"));
	CHECK_EQ(back.gbId, static_cast<uint64_t>(728865));
	CHECK_EQ(back.modName, std::string("Batalla de islas"));
	CHECK(!back.shareable);
	v.shareable = true;
	CHECK(ScenValueParse(ScenValueEncode(v), back));
	CHECK(back.simple);
	CHECK(back.shareable);
	CHECK_EQ(back.gbId, static_cast<uint64_t>(728865));
}

void OldClientsIgnoreTheExtraFields()
{
	ScenValue v;
	v.path = "Scenarios/Island";
	v.modId = "gamebanana-1";
	v.modVersion = "1";
	v.modName = "X";
	v.simple = true;
	v.mapSize = 10;
	v.mapHash = "aa";
	v.gbId = 1;
	const std::string text = ScenValueEncode(v);
	size_t tabs = 0, at = 0;
	std::string field[8];
	for (size_t i = 0; i < text.size() && tabs < 6; i++) {
		if (text[i] == '\t') {
			tabs++;
			at = i + 1;
		}
		else {
			field[tabs] += text[i];
		}
	}
	(void)at;
	CHECK_EQ(field[3], std::string("gamebanana-1"));
	CHECK_EQ(field[5], std::string("X"));
}

void BadValuesAreRefused()
{
	ScenValue v;
	CHECK(!ScenValueParse("", v));
	CHECK(!ScenValueParse("\t0\t", v));
	CHECK(!ScenValueParse("Scenarios/Island", v));
	CHECK(!ScenValueParse("Scenarios/Island\t0", v));
	CHECK(ScenValueParse("Scenarios/Island\t0\t", v));
	CHECK(ScenValueParse("Scenarios/Island\t0\t\tid\t1\tn\t1\t0\t\t0", v));
	CHECK(!v.simple);
	CHECK(ScenValueParse("Scenarios/Island\t0\t\tid\t1\tn\t0\t99\tab\t5", v));
	CHECK(!v.simple);
	CHECK(ScenValueParse("Scenarios/Island\t0\t\t\t1\tn\t1\t99\tab\t5", v));
	CHECK(!v.simple);
}

}

int main()
{
	LooseMapKeepsTheOldFormat();
	PackMapKeepsTheOldFormatToo();
	SimplePackMapCarriesTheFileAndTheSource();
	OldClientsIgnoreTheExtraFields();
	BadValuesAreRefused();
	return test::Summary("scenvalue");
}
