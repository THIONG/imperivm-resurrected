// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>
#include <vector>

#include "game/techplan.h"

namespace {

size_t IndexOf(const std::vector<TechStep> &steps, const std::string &command)
{
	for (size_t i = 0; i < steps.size(); i++) {
		if (steps[i].command == command) return i;
	}
	return steps.size();
}

void Tokens()
{
	const std::vector<std::string> t = TechParamTokens("ReqSet, Spears, , NameSet, Gaul Iron Spearheads, default");
	CHECK_EQ(t.size(), static_cast<size_t>(6));
	if (t.size() == 6) {
		CHECK_EQ(t[1], std::string("Spears"));
		CHECK_EQ(t[2], std::string(""));
		CHECK_EQ(t[4], std::string("Gaul Iron Spearheads"));
	}
	CHECK(TechParamTokens("").empty());
	CHECK(TechParamTokens("   ").empty());
}

void OnlyResearchesWithAName()
{
	const std::vector<TechStep> steps = TechPlan({
		{"Democracy", "government_form", "Democracy"},
		{"trainGArcher", "train", "GArcher"},
		{"NoName", "research", "SetsSet, x, 1"},
		{"Fights", "Research", "NameSet, Fights, default"},
		{"Nobility", "research", "NamePlr, Nobility, default"},
	});
	CHECK_EQ(steps.size(), static_cast<size_t>(2));
	if (steps.size() == 2) {
		CHECK_EQ(steps[0].command, std::string("Fights"));
		CHECK(!steps[0].playerName);
		CHECK_EQ(steps[1].name, std::string("Nobility"));
		CHECK(steps[1].playerName);
	}
}

void RequirementsComeFirst()
{
	const std::vector<TechStep> steps = TechPlan({
		{"Trident Warrior Exp3", "research", "ReqSet, Fights, , ReqSet, Trident Warrior Exp2, , NameSet, Trident Warrior Exp3, default"},
		{"Trident Warrior Exp2", "research", "ReqSet, Fights, default, NameSet, Trident Warrior Exp2, default"},
		{"Fights", "research", "NameSet, Fights, default"},
		{"Unrelated", "research", "NameSet, Unrelated, default"},
	});
	CHECK_EQ(steps.size(), static_cast<size_t>(4));
	CHECK(IndexOf(steps, "Fights") < IndexOf(steps, "Trident Warrior Exp2"));
	CHECK(IndexOf(steps, "Trident Warrior Exp2") < IndexOf(steps, "Trident Warrior Exp3"));
	CHECK_EQ(IndexOf(steps, "Fights"), static_cast<size_t>(0));
}

void RequirementByAnotherCommandsName()
{
	const std::vector<TechStep> steps = TechPlan({
		{"Advanced Thing", "research", "ReqPlr, Training, default, NameSet, Advanced Thing, default"},
		{"Gaul Training 1", "research", "NamePlr, Training, default"},
	});
	CHECK(IndexOf(steps, "Gaul Training 1") < IndexOf(steps, "Advanced Thing"));
}

void CycleKeepsEverything()
{
	const std::vector<TechStep> steps = TechPlan({
		{"A", "research", "ReqSet, B, , NameSet, A, default"},
		{"B", "research", "ReqSet, A, , NameSet, B, default"},
	});
	CHECK_EQ(steps.size(), static_cast<size_t>(2));
}

void ChoicesAreSharedEffects()
{
	const std::vector<TechStep> steps = TechPlan({
		{"ChooseRepublicanRome", "research", "NameSet, ChooseRepublicanRome, default, SetsSet, ChooseRomanRace, 1"},
		{"ChooseImperialRome", "research", "NameSet, ChooseImperialRome, default, SetsSet, ChooseRomanRace, 1"},
		{"ChoosePunicRome", "research", "NameSet, ChoosePunicRome, default, SetsSet, ChooseRomanRace, 1"},
		{"Gaul Fine Armor", "research", "NameSet, Gaul Fine Armor, default, SetsSet, levels/GWomanWarrior, 6"},
		{"Rome Training 1", "research", "NamePlr, Rome Training 1, default, SetsPlr, maxtrainlevel, 4"},
		{"Gaul Training 1", "research", "NamePlr, Gaul Training 1, default, SetsPlr, maxtrainlevel, 4"},
		{"Other Level", "research", "NameSet, Other Level, default, SetsSet, maxtrainlevel, 4"},
	});
	const size_t imperial = IndexOf(steps, "ChooseImperialRome");
	CHECK(imperial < steps.size());
	if (imperial < steps.size()) {
		CHECK_EQ(steps[imperial].choices.size(), static_cast<size_t>(1));
		if (!steps[imperial].choices.empty()) {
			CHECK(!steps[imperial].choices[0].player);
			CHECK_EQ(steps[imperial].choices[0].var, std::string("ChooseRomanRace"));
			CHECK_EQ(steps[imperial].choices[0].value, std::string("1"));
		}
	}
	const size_t armor = IndexOf(steps, "Gaul Fine Armor");
	CHECK(armor < steps.size() && steps[armor].choices.empty());
	const size_t rome = IndexOf(steps, "Rome Training 1");
	CHECK(rome < steps.size() && steps[rome].choices.size() == 1 && steps[rome].choices[0].player);
	const size_t other = IndexOf(steps, "Other Level");
	CHECK(other < steps.size() && steps[other].choices.empty());
}

}

int main()
{
	Tokens();
	OnlyResearchesWithAName();
	RequirementsComeFirst();
	RequirementByAnotherCommandsName();
	CycleKeepsEverything();
	ChoicesAreSharedEffects();
	return test::Summary("techplan");
}
