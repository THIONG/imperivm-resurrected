// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>

#include "game/bonuses.h"

namespace {

const std::string kScript =
	"//void, Settlement set, int playerid\n"
	"\n"
	"//@name Fortune\n"
	"//@name.es Fortuna\n"
	"//@description 4000 gold.\n"
	"//@description.es 4000 de oro.\n"
	"//@Description.IT 4000 d'oro.\n"
	"Sleep(100);\n"
	"//@name Late\n";

void ReadsNotesByLanguage()
{
	CHECK_EQ(BonusScriptNote(kScript, "name", "es"), std::string("Fortuna"));
	CHECK_EQ(BonusScriptNote(kScript, "name", "en"), std::string("Fortune"));
	CHECK_EQ(BonusScriptNote(kScript, "name", "fr"), std::string("Fortune"));
	CHECK_EQ(BonusScriptNote(kScript, "description", "it"), std::string("4000 d'oro."));
	CHECK_EQ(BonusScriptNote(kScript, "description", "es"), std::string("4000 de oro."));
	CHECK_EQ(BonusScriptNote(kScript, "other", "es"), std::string());
}

void StopsAtTheFirstCodeLine()
{
	CHECK_EQ(BonusScriptNote("Sleep(1);\n//@name X\n", "name", "en"), std::string());
	CHECK_EQ(BonusScriptNote("", "name", "en"), std::string());
	CHECK_EQ(BonusScriptNote("//@name\n//@name   \n", "name", "en"), std::string());
	CHECK_EQ(BonusScriptNote("//@name   Spaced out  \r\n", "name", "en"), std::string("Spaced out"));
}

void FileNumbers()
{
	CHECK_EQ(BonusFileNumber("Data/BonusScripts/004 BOAR TEETH.VS"), 4);
	CHECK_EQ(BonusFileNumber("Data\\BonusScripts\\100 FORTUNE.VS"), 100);
	CHECK_EQ(BonusFileNumber("007 X.vs"), 7);
	CHECK_EQ(BonusFileNumber("Data/BonusScripts/GIFT.VS"), -1);
	CHECK_EQ(BonusFileNumber("Data/BonusScripts/12.VS"), -1);
	CHECK_EQ(BonusFileNumber(""), -1);
	CHECK_EQ(BonusFileKey("Data/BonusScripts/001 WEALTH.VS"), std::string("wealth"));
	CHECK_EQ(BonusFileKey("Data/BonusScripts/105 FINGER OF DEATH.vs"), std::string("finger of death"));
	CHECK_EQ(BonusFileKey("GIFT.VS"), std::string("gift"));
}

void BuiltinsAndOverrides()
{
	const std::string base = "//void, Settlement set, int playerid\nSleep(100);\nset.SetGold(set.gold + 2000);\nset.GoldConverted(2000);\n";
	const BonusDef wealth = BonusDefine("Riqueza", "Data/BonusScripts/001 WEALTH.VS", base, "es");
	CHECK_EQ(wealth.name, std::string("2000 gold"));
	CHECK(!wealth.description.empty());
	const BonusDef none = BonusDefine("Ninguno", "Data/BonusScripts/000 NONE.VS", "//void, Settlement set, int playerid\n", "en");
	CHECK_EQ(none.name, std::string("No bonus"));
	const BonusDef pack = BonusDefine("Fortune", "Data/BonusScripts/004 FORTUNE.VS", kScript, "es");
	CHECK_EQ(pack.name, std::string("Fortuna"));
	CHECK_EQ(pack.description, std::string("4000 de oro."));
	const BonusDef plain = BonusDefine("Boar Teeth", "Data/BonusScripts/104 BOAR TEETH.VS", "", "en");
	CHECK_EQ(plain.name, std::string("Boar Teeth"));
	CHECK(plain.description.empty());
	const BonusDef changed = BonusDefine("Riqueza", "Data/BonusScripts/001 WEALTH.VS", "//@description 3000 gold.\nset.SetGold(set.gold + 2000);\n", "en");
	CHECK_EQ(changed.description, std::string("3000 gold."));
	CHECK_EQ(changed.name, std::string("2000 gold"));
	const BonusDef other = BonusDefine("Riqueza", "Data/BonusScripts/001 WEALTH.VS", "Sleep(100);\nset.SetFood(set.food + 3000);\n", "en");
	CHECK_EQ(other.name, std::string("Riqueza"));
	CHECK(other.description.empty());
	const BonusDef notEmpty = BonusDefine("Ninguno", "Data/BonusScripts/000 NONE.VS", "set.SetGold(1);\n", "en");
	CHECK_EQ(notEmpty.name, std::string("Ninguno"));
	const BonusDef hero = BonusDefine("Héroe", "Data/BonusScripts/003 HERO.VS", "hero_unit.SetLevel( 12 );\n", "en");
	CHECK_EQ(hero.name, std::string("Hero"));
}
}

int main()
{
	ReadsNotesByLanguage();
	StopsAtTheFirstCodeLine();
	FileNumbers();
	BuiltinsAndOverrides();
	return test::Summary("bonuses");
}
