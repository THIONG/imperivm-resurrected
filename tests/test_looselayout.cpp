// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>
#include <vector>

#include "mods/catalogfilter.h"
#include "mods/looselayout.h"

namespace {

std::string Describe(const std::vector<LooseMove> &moves)
{
	std::string out;
	for (const LooseMove &m : moves) out += (out.empty() ? "" : " ") + m.file + ">" + m.folder;
	return out;
}

std::string Header(const char *type)
{
	return std::string("HPFS\x01\x02 junk <game>\r\n<properties\r\n\tgame_type=\"") + type + "\"\r\n\tname=\"Map\"/>";
}

void KindFromTheFile()
{
	CHECK_EQ(BfhpMapKind(Header("0")), static_cast<int>(kMapScenario));
	CHECK_EQ(BfhpMapKind(Header("1")), static_cast<int>(kMapAdventure));
	CHECK_EQ(BfhpMapKind(Header("2")), static_cast<int>(kMapConquest));
	CHECK_EQ(BfhpMapKind(Header("7")), static_cast<int>(kMapUnknown));
	CHECK_EQ(BfhpMapKind(Header("10")), static_cast<int>(kMapUnknown));
	CHECK_EQ(BfhpMapKind("game_type=\"1\" without the properties tag"), static_cast<int>(kMapUnknown));
	CHECK_EQ(BfhpMapKind(""), static_cast<int>(kMapUnknown));
	CHECK_EQ(BfhpMapKind("<properties\r\n\tadventure=\"1\"\r\n\tname=\"Old\"/>"), static_cast<int>(kMapAdventure));
	CHECK_EQ(BfhpMapKind("<properties game_type=\"0\" adventure=\"1\" name=\"Old\"/>"), static_cast<int>(kMapAdventure));
	CHECK_EQ(BfhpMapKind("<properties game_type=\"2\" adventure=\"1\"/>"), static_cast<int>(kMapConquest));
	CHECK_EQ(BfhpMapKind("<properties name=\"No type\"/>"), static_cast<int>(kMapScenario));
	CHECK_EQ(BfhpMapKind("<properties last_game_type=\"1\" name=\"x\"/>"), static_cast<int>(kMapScenario));
	CHECK_EQ(BfhpMapKind("<properties name=\"x\"/> later game_type=\"1\""), static_cast<int>(kMapScenario));
}

void TheFileWinsOverTheCategory()
{
	const std::vector<LooseFile> adventure = {{"El_hijo.BFHP", kMapAdventure}, {"El_hijo.bmp"}, {"other.bmp"}, {"notes.txt"}};
	CHECK_EQ(Describe(PlanLooseFiles(adventure, false, kCatScenarios)), std::string("El_hijo.BFHP>adventures El_hijo.bmp>adventures"));
	const std::vector<LooseFile> scenario = {{"islas.BFHP", kMapScenario}, {"readme map.txt"}};
	CHECK_EQ(Describe(PlanLooseFiles(scenario, false, kCatScenarios)), std::string("islas.BFHP>scenarios"));
	CHECK_EQ(Describe(PlanLooseFiles(scenario, false, kCatConquests)), std::string("islas.BFHP>scenarios"));
	CHECK_EQ(Describe(PlanLooseFiles({{"med.bfhp", kMapConquest}}, false, kCatAdventures)), std::string("med.bfhp>conquests"));
}

void SingleMissionsGoToAdventuresByTheirCategory()
{
	const std::vector<LooseFile> mission = {{"Viaje.BFHP", kMapScenario}, {"Viaje.bmp"}};
	CHECK_EQ(Describe(PlanLooseFiles(mission, false, kCatAdventures)), std::string("Viaje.BFHP>adventures Viaje.bmp>adventures"));
	CHECK_EQ(Describe(PlanLooseFiles(mission, false, -1)), std::string("Viaje.BFHP>scenarios"));
}

void UnreadableFilesFollowTheCategory()
{
	const std::vector<LooseFile> files = {{"map.BFHP"}, {"map.bmp"}};
	CHECK_EQ(Describe(PlanLooseFiles(files, false, kCatScenarios)), std::string("map.BFHP>scenarios"));
	CHECK_EQ(Describe(PlanLooseFiles(files, false, -1)), std::string("map.BFHP>scenarios"));
	CHECK_EQ(Describe(PlanLooseFiles(files, false, kCatGameplay)), std::string("map.BFHP>scenarios"));
	CHECK_EQ(Describe(PlanLooseFiles(files, false, kCatAdventures)), std::string("map.BFHP>adventures map.bmp>adventures"));
	CHECK_EQ(Describe(PlanLooseFiles(files, false, kCatConquests)), std::string("map.BFHP>conquests"));
}

void StructuredPacksAreLeftAlone()
{
	CHECK_EQ(PlanLooseFiles({{"map.bfhp", kMapScenario}}, true, kCatScenarios).size(), static_cast<size_t>(0));
	CHECK_EQ(PlanLooseFiles({{"mod.json"}, {"readme.txt"}}, false, kCatScenarios).size(), static_cast<size_t>(0));
	CHECK_EQ(PlanLooseFiles({}, false, kCatScenarios).size(), static_cast<size_t>(0));
}

void VideosTheMapPlays()
{
	const std::string script = "play(\"movies\\Video1.avi\"); x = 'Movies/Video2.AVI'; movies\\Video1.avi \"movies\\Intro final.avi\" movies\\no.txt";
	const std::vector<std::string> refs = BfhpVideoReferences(script);
	CHECK_EQ(refs.size(), static_cast<size_t>(3));
	CHECK_EQ(refs[0], std::string("video1.avi"));
	CHECK_EQ(refs[1], std::string("video2.avi"));
	CHECK_EQ(refs[2], std::string("intro final.avi"));
	CHECK_EQ(BfhpVideoReferences("movies\\Final.avi and more text").size(), static_cast<size_t>(1));
	CHECK_EQ(BfhpVideoReferences("no videos here").size(), static_cast<size_t>(0));
	CHECK_EQ(BfhpVideoReferences("movies\\").size(), static_cast<size_t>(0));
	const std::vector<std::string> root = {"Video1.avi", "VIDEO2.AVI", "other.avi", "map.bfhp", "notes.txt"};
	CHECK_EQ(Describe(PlanLooseVideos(root, refs)), std::string("Video1.avi>movies VIDEO2.AVI>movies"));
	CHECK_EQ(PlanLooseVideos(root, {}).size(), static_cast<size_t>(0));
}

}

int main()
{
	KindFromTheFile();
	TheFileWinsOverTheCategory();
	SingleMissionsGoToAdventuresByTheirCategory();
	UnreadableFilesFollowTheCategory();
	StructuredPacksAreLeftAlone();
	VideosTheMapPlays();
	return test::Summary("looselayout");
}
