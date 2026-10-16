// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>
#include <vector>

#include "mods/catalogfilter.h"

namespace {

uint32_t P(std::vector<int> n)
{
	uint32_t out = 0;
	for (int v : n) out |= 1u << v;
	return out;
}

uint32_t L(std::vector<int> n)
{
	uint32_t out = 0;
	for (int v : n) out |= 1u << v;
	return out;
}

CatalogFacts Facts(int category, uint32_t players = 0, uint32_t languages = 0)
{
	CatalogFacts f;
	f.category = category;
	f.players = players;
	f.languages = languages;
	return f;
}

void Categories()
{
	CHECK_EQ(CatalogCategoryFromName("Maps"), static_cast<int>(kCatMaps));
	CHECK_EQ(CatalogCategoryFromName("maps"), static_cast<int>(kCatMaps));
	CHECK_EQ(CatalogCategoryFromName("Scenarios"), static_cast<int>(kCatMaps));
	CHECK_EQ(CatalogCategoryFromName("Gameplay & Balance"), static_cast<int>(kCatGameplay));
	CHECK_EQ(CatalogCategoryFromName("Gameplay and Balance"), static_cast<int>(kCatGameplay));
	CHECK_EQ(CatalogCategoryFromName("Total Conversions"), static_cast<int>(kCatGameplay));
	CHECK_EQ(CatalogCategoryFromName("Units & Heroes"), static_cast<int>(kCatUnits));
	CHECK_EQ(CatalogCategoryFromName("Civilizations"), static_cast<int>(kCatCivilizations));
	CHECK_EQ(CatalogCategoryFromName("Campaigns"), static_cast<int>(kCatCampaigns));
	CHECK_EQ(CatalogCategoryFromName("Conquest"), static_cast<int>(kCatConquest));
	CHECK_EQ(CatalogCategoryFromName("Graphics"), static_cast<int>(kCatGraphics));
	CHECK_EQ(CatalogCategoryFromName("Interface"), static_cast<int>(kCatGraphics));
	CHECK_EQ(CatalogCategoryFromName("Audio & Voices"), static_cast<int>(kCatAudio));
	CHECK_EQ(CatalogCategoryFromName("Translations"), static_cast<int>(kCatTranslations));
	CHECK_EQ(CatalogCategoryFromName("Other"), -1);
	CHECK_EQ(CatalogCategoryFromName(""), -1);
	for (int c = 0; c < kCatCount; c++) {
		CHECK_EQ(CatalogCategoryFromName(CatalogCategoryName(c)), c);
		CHECK_EQ(CatalogCategoryFromName(CatalogCategoryLabel(c)), c);
	}
	CHECK_EQ(CatalogCategoryGroup(kCatGameplay), 0);
	CHECK_EQ(CatalogCategoryGroup(kCatCivilizations), 0);
	CHECK_EQ(CatalogCategoryGroup(kCatMaps), 1);
	CHECK_EQ(CatalogCategoryGroup(kCatConquest), 1);
	CHECK_EQ(CatalogCategoryGroup(kCatGraphics), 2);
	CHECK_EQ(CatalogCategoryGroup(kCatTranslations), 2);
	CHECK_EQ(CatalogCategoryGroup(-1), -1);
}

void Players()
{
	CHECK_EQ(CatalogPlayersFromTag("4 players"), P({4}));
	CHECK_EQ(CatalogPlayersFromTag("4 Players"), P({4}));
	CHECK_EQ(CatalogPlayersFromTag("2 player"), P({2}));
	CHECK_EQ(CatalogPlayersFromTag("4"), P({4}));
	CHECK_EQ(CatalogPlayersFromTag("2-4 players"), P({2, 3, 4}));
	CHECK_EQ(CatalogPlayersFromTag("2 to 4 players"), P({2, 3, 4}));
	CHECK_EQ(CatalogPlayersFromTag("2 a 4 jugadores"), P({2, 3, 4}));
	CHECK_EQ(CatalogPlayersFromTag("6 jugadores"), P({6}));
	CHECK_EQ(CatalogPlayersFromTag("8 giocatori"), P({8}));
	CHECK_EQ(CatalogPlayersFromTag("Up to 6 players"), P({2, 3, 4, 5, 6}));
	CHECK_EQ(CatalogPlayersFromTag("hasta 3 jugadores"), P({2, 3}));
	CHECK_EQ(CatalogPlayersFromTag("1v1"), P({2}));
	CHECK_EQ(CatalogPlayersFromTag("2v2"), P({4}));
	CHECK_EQ(CatalogPlayersFromTag("4 vs 4"), P({8}));
	CHECK_EQ(CatalogPlayersFromTag("2v2v2v2"), P({8}));
	CHECK_EQ(CatalogPlayersFromTag("4p"), P({4}));
	CHECK_EQ(CatalogPlayersFromTag("FFA 6"), P({6}));
	CHECK_EQ(CatalogPlayersFromTag("2-16 players"), P({2, 3, 4, 5, 6, 7, 8}));
	CHECK_EQ(CatalogPlayersFromTag("1 player"), 0u);
	CHECK_EQ(CatalogPlayersFromTag("12 players"), 0u);
	CHECK_EQ(CatalogPlayersFromTag("Version 2"), 0u);
	CHECK_EQ(CatalogPlayersFromTag("Spanish"), 0u);
	CHECK_EQ(CatalogPlayersFromTag(""), 0u);
	CHECK_EQ(CatalogPlayersText(P({4})), std::string("4"));
	CHECK_EQ(CatalogPlayersText(P({2, 3, 4})), std::string("2-4"));
	CHECK_EQ(CatalogPlayersText(P({2, 4})), std::string("2, 4"));
	CHECK_EQ(CatalogPlayersText(P({2, 3})), std::string("2, 3"));
	CHECK_EQ(CatalogPlayersText(0), std::string(""));
}

void Languages()
{
	CHECK_EQ(CatalogLanguagesFromTag("Spanish"), L({kLangSpanish}));
	CHECK_EQ(CatalogLanguagesFromTag("Espa\xC3\xB1ol"), L({kLangSpanish}));
	CHECK_EQ(CatalogLanguagesFromTag("castellano"), L({kLangSpanish}));
	CHECK_EQ(CatalogLanguagesFromTag("English"), L({kLangEnglish}));
	CHECK_EQ(CatalogLanguagesFromTag("Italiano"), L({kLangItalian}));
	CHECK_EQ(CatalogLanguagesFromTag("Deutsch"), L({kLangGerman}));
	CHECK_EQ(CatalogLanguagesFromTag("Fran\xC3\xA7" "ais"), L({kLangFrench}));
	CHECK_EQ(CatalogLanguagesFromTag("Polish"), L({kLangPolish}));
	CHECK_EQ(CatalogLanguagesFromTag("Multilingual"), L({kLangSeveral}));
	CHECK_EQ(CatalogLanguagesFromTag("Spanish voices"), L({kLangSpanish}));
	CHECK_EQ(CatalogLanguagesFromTag("4 players"), 0u);
	CHECK_EQ(CatalogLanguagesFromTag("Englishman"), 0u);
}

void FactsFromGameBanana()
{
	const CatalogFacts a = CatalogFactsOf("Maps", {"Maps", "2-4 players", "Spanish"});
	CHECK_EQ(a.category, static_cast<int>(kCatMaps));
	CHECK_EQ(a.players, P({2, 3, 4}));
	CHECK_EQ(a.languages, L({kLangSpanish}));
	const CatalogFacts b = CatalogFactsOf("", {"Translations", "Italian"});
	CHECK_EQ(b.category, static_cast<int>(kCatTranslations));
	CHECK_EQ(b.languages, L({kLangItalian}));
	const CatalogFacts c = CatalogFactsOf("Something else", {});
	CHECK_EQ(c.category, -1);
}

void Matching()
{
	const CatalogFacts map4 = Facts(kCatMaps, P({4}));
	const CatalogFacts map2 = Facts(kCatMaps, P({2}));
	const CatalogFacts mapNone = Facts(kCatMaps);
	const CatalogFacts campaignEs = Facts(kCatCampaigns, 0, L({kLangSpanish}));
	const CatalogFacts campaignMulti = Facts(kCatCampaigns, 0, L({kLangSeveral}));
	const CatalogFacts rules = Facts(kCatGameplay);
	const CatalogFacts unknown = Facts(-1);

	CatalogFilter none;
	CHECK(CatalogMatches(map4, none));
	CHECK(CatalogMatches(unknown, none));

	CatalogFilter maps;
	maps.types = 1u << kCatMaps;
	CHECK(CatalogMatches(map4, maps));
	CHECK(!CatalogMatches(rules, maps));
	CHECK(!CatalogMatches(unknown, maps));

	CatalogFilter mapsOrCampaigns;
	mapsOrCampaigns.types = (1u << kCatMaps) | (1u << kCatCampaigns);
	CHECK(CatalogMatches(map4, mapsOrCampaigns));
	CHECK(CatalogMatches(campaignEs, mapsOrCampaigns));
	CHECK(!CatalogMatches(rules, mapsOrCampaigns));

	CatalogFilter maps4 = maps;
	maps4.players = P({4});
	CHECK(CatalogMatches(map4, maps4));
	CHECK(!CatalogMatches(map2, maps4));
	CHECK(!CatalogMatches(mapNone, maps4));

	CatalogFilter mixed = mapsOrCampaigns;
	mixed.players = P({4});
	CHECK(CatalogMatches(campaignEs, mixed));
	CHECK(!CatalogMatches(map2, mixed));

	CatalogFilter spanish;
	spanish.types = 1u << kCatCampaigns;
	spanish.languages = L({kLangSpanish});
	CHECK(CatalogMatches(campaignEs, spanish));
	CHECK(CatalogMatches(campaignMulti, spanish));
	CHECK(!CatalogMatches(Facts(kCatCampaigns, 0, L({kLangEnglish})), spanish));
	CHECK(!CatalogMatches(Facts(kCatCampaigns), spanish));

	CatalogFilter several;
	several.types = 1u << kCatCampaigns;
	several.languages = L({kLangSeveral});
	CHECK(CatalogMatches(campaignMulti, several));
	CHECK(!CatalogMatches(campaignEs, several));
}

void Normalize()
{
	CatalogFilter f;
	f.players = P({4});
	f.languages = L({kLangSpanish});
	CHECK(CatalogFilterEmpty(CatalogNormalize(f)));

	f.types = 1u << kCatMaps;
	CatalogFilter n = CatalogNormalize(f);
	CHECK_EQ(n.players, P({4}));
	CHECK_EQ(n.languages, 0u);

	f.types = 1u << kCatTranslations;
	n = CatalogNormalize(f);
	CHECK_EQ(n.players, 0u);
	CHECK_EQ(n.languages, L({kLangSpanish}));

	f.types = (1u << kCatMaps) | (1u << kCatAudio);
	n = CatalogNormalize(f);
	CHECK_EQ(CatalogFilterSize(n), 4);
	CHECK(!CatalogFilterEmpty(n));

	f.types = 1u << 31;
	CHECK(CatalogFilterEmpty(CatalogNormalize(f)));
}

void Counts()
{
	CatalogCounts c;
	CatalogCount(Facts(kCatMaps, P({2, 4})), c);
	CatalogCount(Facts(kCatMaps, P({4})), c);
	CatalogCount(Facts(kCatCampaigns, 0, L({kLangSpanish})), c);
	CatalogCount(Facts(kCatTranslations, 0, L({kLangSpanish, kLangEnglish})), c);
	CatalogCount(Facts(kCatGraphics, 0, L({kLangSpanish})), c);
	CatalogCount(Facts(kCatGameplay), c);
	CatalogCount(Facts(-1), c);
	CHECK_EQ(c.types[kCatMaps], 2);
	CHECK_EQ(c.types[kCatCampaigns], 1);
	CHECK_EQ(c.types[kCatGameplay], 1);
	CHECK_EQ(c.types[kCatGraphics], 1);
	CHECK_EQ(c.types[kCatConquest], 0);
	CHECK_EQ(c.players[2], 1);
	CHECK_EQ(c.players[4], 2);
	CHECK_EQ(c.players[3], 0);
	CHECK_EQ(CatalogLanguageCount(c, 1u << kCatMaps, kLangSpanish), 0);
	CHECK_EQ(CatalogLanguageCount(c, 1u << kCatCampaigns, kLangSpanish), 1);
	CHECK_EQ(CatalogLanguageCount(c, 1u << kCatCampaigns, kLangEnglish), 0);
	CHECK_EQ(CatalogLanguageCount(c, (1u << kCatCampaigns) | (1u << kCatTranslations), kLangSpanish), 2);
	CHECK_EQ(CatalogLanguageCount(c, (1u << kCatCampaigns) | (1u << kCatTranslations), kLangEnglish), 1);
	CHECK_EQ(CatalogLanguageCount(c, 1u << kCatGraphics, kLangSpanish), 0);
	CHECK_EQ(CatalogLanguageCount(c, ~0u, kLangCount), 0);
}

}

int main()
{
	Categories();
	Players();
	Languages();
	FactsFromGameBanana();
	Matching();
	Normalize();
	Counts();
	return test::Summary("catalogfilter");
}
