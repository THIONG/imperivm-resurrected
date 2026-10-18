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
	CHECK_EQ(CatalogCategoryFromName("Maps"), static_cast<int>(kCatScenarios));
	CHECK_EQ(CatalogCategoryFromName("maps"), static_cast<int>(kCatScenarios));
	CHECK_EQ(CatalogCategoryFromName("Scenarios"), static_cast<int>(kCatScenarios));
	CHECK_EQ(CatalogCategoryFromName("Gameplay & Balance"), static_cast<int>(kCatGameplay));
	CHECK_EQ(CatalogCategoryFromName("Gameplay and Balance"), static_cast<int>(kCatGameplay));
	CHECK_EQ(CatalogCategoryFromName("Total Conversions"), static_cast<int>(kCatGameplay));
	CHECK_EQ(CatalogCategoryFromName("Units & Heroes"), static_cast<int>(kCatUnits));
	CHECK_EQ(CatalogCategoryFromName("Civilizations"), static_cast<int>(kCatCivilizations));
	CHECK_EQ(CatalogCategoryFromName("Campaigns"), static_cast<int>(kCatAdventures));
	CHECK_EQ(CatalogCategoryFromName("Adventures"), static_cast<int>(kCatAdventures));
	CHECK_EQ(CatalogCategoryFromName("Conquest"), static_cast<int>(kCatConquests));
	CHECK_EQ(CatalogCategoryFromName("Conquests"), static_cast<int>(kCatConquests));
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
	CHECK_EQ(CatalogCategoryGroup(kCatScenarios), 1);
	CHECK_EQ(CatalogCategoryGroup(kCatConquests), 1);
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
	CHECK_EQ(a.category, static_cast<int>(kCatScenarios));
	CHECK_EQ(a.players, P({2, 3, 4}));
	CHECK_EQ(a.languages, L({kLangSpanish}));
	const CatalogFacts b = CatalogFactsOf("", {"Translations", "Italian"});
	CHECK_EQ(b.category, static_cast<int>(kCatTranslations));
	CHECK_EQ(b.languages, L({kLangItalian}));
	const CatalogFacts c = CatalogFactsOf("Something else", {});
	CHECK_EQ(c.category, -1);
	CHECK_EQ(CatalogCategoriesOf(c), 0u);
}

void TagsWithTitles()
{
	const CatalogFacts caesar = CatalogFactsOf("Gameplay & Balance", {"Gameplay & Balance", "Type: Civilizations", "Type: Units & Heroes",
		"Type: Scenarios", "Type: Adventures", "Language: Spanish", "Language: English", "Software Used: Blender"});
	CHECK_EQ(caesar.category, static_cast<int>(kCatGameplay));
	CHECK_EQ(CatalogCategoriesOf(caesar), (1u << kCatGameplay) | (1u << kCatCivilizations) | (1u << kCatUnits) | (1u << kCatScenarios) | (1u << kCatAdventures));
	CHECK_EQ(caesar.languages, L({kLangSpanish, kLangEnglish}));
	CHECK_EQ(caesar.players, 0u);

	const CatalogFacts map = CatalogFactsOf("Scenarios", {"Players: 2-4", "Players: 8"});
	CHECK_EQ(map.players, P({2, 3, 4, 8}));
	CHECK_EQ(CatalogFactsOf("Scenarios", {"Players: 2v2"}).players, P({4}));
	CHECK_EQ(CatalogFactsOf("Scenarios", {"Jugadores: 4"}).players, P({4}));
	CHECK_EQ(CatalogFactsOf("Translations", {"Idioma: Espa\xC3\xB1ol"}).languages, L({kLangSpanish}));
	CHECK_EQ(CatalogFactsOf("Graphics", {"tipo: audio & voices"}).categories, (1u << kCatGraphics) | (1u << kCatAudio));
	CHECK_EQ(CatalogFactsOf("", {"Type: Conquests"}).category, static_cast<int>(kCatConquests));

	const CatalogFacts other = CatalogFactsOf("Scenarios", {"Version: 2", "Software Used: Spanish Editor", "Author: Maps"});
	CHECK_EQ(other.players, 0u);
	CHECK_EQ(other.languages, 0u);
	CHECK_EQ(CatalogCategoriesOf(other), 1u << kCatScenarios);

	CHECK(CatalogTagIsFacet("Type: Graphics"));
	CHECK(CatalogTagIsFacet("Players: 4"));
	CHECK(CatalogTagIsFacet("Language: Italian"));
	CHECK(CatalogTagIsFacet("4 players"));
	CHECK(CatalogTagIsFacet("Spanish"));
	CHECK(!CatalogTagIsFacet("Type: Something"));
	CHECK(!CatalogTagIsFacet("Software Used: Blender"));
	CHECK(!CatalogTagIsFacet("Historical"));
}

void SeveralCategories()
{
	CatalogFacts big = Facts(kCatGameplay);
	big.categories = (1u << kCatScenarios) | (1u << kCatAdventures);
	big.languages = L({kLangItalian});

	CatalogFilter scenarios;
	scenarios.types = 1u << kCatScenarios;
	CHECK(CatalogMatches(big, scenarios));
	CatalogFilter graphics;
	graphics.types = 1u << kCatGraphics;
	CHECK(!CatalogMatches(big, graphics));
	CatalogFilter rules;
	rules.types = 1u << kCatGameplay;
	CHECK(CatalogMatches(big, rules));

	CatalogFilter scenarios4 = scenarios;
	scenarios4.players = P({4});
	CHECK(!CatalogMatches(big, scenarios4));
	CatalogFilter both4 = scenarios4;
	both4.types |= 1u << kCatAdventures;
	CHECK(CatalogMatches(big, both4));

	CatalogFilter adventuresSpanish;
	adventuresSpanish.types = 1u << kCatAdventures;
	adventuresSpanish.languages = L({kLangSpanish});
	CHECK(!CatalogMatches(big, adventuresSpanish));
	adventuresSpanish.types |= 1u << kCatGameplay;
	CHECK(CatalogMatches(big, adventuresSpanish));

	CatalogCounts c;
	CatalogCount(big, c);
	CHECK_EQ(c.types[kCatGameplay], 1);
	CHECK_EQ(c.types[kCatScenarios], 1);
	CHECK_EQ(c.types[kCatAdventures], 1);
	CHECK_EQ(c.types[kCatGraphics], 0);
	CHECK_EQ(c.languages[kCatAdventures][kLangItalian], 1);
	CHECK_EQ(c.languages[kCatGameplay][kLangItalian], 0);
}

void Matching()
{
	const CatalogFacts map4 = Facts(kCatScenarios, P({4}));
	const CatalogFacts map2 = Facts(kCatScenarios, P({2}));
	const CatalogFacts mapNone = Facts(kCatScenarios);
	const CatalogFacts campaignEs = Facts(kCatAdventures, 0, L({kLangSpanish}));
	const CatalogFacts campaignMulti = Facts(kCatAdventures, 0, L({kLangSeveral}));
	const CatalogFacts rules = Facts(kCatGameplay);
	const CatalogFacts unknown = Facts(-1);

	CatalogFilter none;
	CHECK(CatalogMatches(map4, none));
	CHECK(CatalogMatches(unknown, none));

	CatalogFilter maps;
	maps.types = 1u << kCatScenarios;
	CHECK(CatalogMatches(map4, maps));
	CHECK(!CatalogMatches(rules, maps));
	CHECK(!CatalogMatches(unknown, maps));

	CatalogFilter mapsOrCampaigns;
	mapsOrCampaigns.types = (1u << kCatScenarios) | (1u << kCatAdventures);
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
	spanish.types = 1u << kCatAdventures;
	spanish.languages = L({kLangSpanish});
	CHECK(CatalogMatches(campaignEs, spanish));
	CHECK(CatalogMatches(campaignMulti, spanish));
	CHECK(!CatalogMatches(Facts(kCatAdventures, 0, L({kLangEnglish})), spanish));
	CHECK(!CatalogMatches(Facts(kCatAdventures), spanish));

	CatalogFilter several;
	several.types = 1u << kCatAdventures;
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

	f.types = 1u << kCatScenarios;
	CatalogFilter n = CatalogNormalize(f);
	CHECK_EQ(n.players, P({4}));
	CHECK_EQ(n.languages, 0u);

	f.types = 1u << kCatTranslations;
	n = CatalogNormalize(f);
	CHECK_EQ(n.players, 0u);
	CHECK_EQ(n.languages, L({kLangSpanish}));

	f.types = (1u << kCatScenarios) | (1u << kCatAudio);
	n = CatalogNormalize(f);
	CHECK_EQ(CatalogFilterSize(n), 4);
	CHECK(!CatalogFilterEmpty(n));

	f.types = 1u << 31;
	CHECK(CatalogFilterEmpty(CatalogNormalize(f)));
}

void Counts()
{
	CatalogCounts c;
	CatalogCount(Facts(kCatScenarios, P({2, 4})), c);
	CatalogCount(Facts(kCatScenarios, P({4})), c);
	CatalogCount(Facts(kCatAdventures, 0, L({kLangSpanish})), c);
	CatalogCount(Facts(kCatTranslations, 0, L({kLangSpanish, kLangEnglish})), c);
	CatalogCount(Facts(kCatGraphics, 0, L({kLangSpanish})), c);
	CatalogCount(Facts(kCatGameplay), c);
	CatalogCount(Facts(-1), c);
	CHECK_EQ(c.types[kCatScenarios], 2);
	CHECK_EQ(c.types[kCatAdventures], 1);
	CHECK_EQ(c.types[kCatGameplay], 1);
	CHECK_EQ(c.types[kCatGraphics], 1);
	CHECK_EQ(c.types[kCatConquests], 0);
	CHECK_EQ(c.players[2], 1);
	CHECK_EQ(c.players[4], 2);
	CHECK_EQ(c.players[3], 0);
	CHECK_EQ(CatalogLanguageCount(c, 1u << kCatScenarios, kLangSpanish), 0);
	CHECK_EQ(CatalogLanguageCount(c, 1u << kCatAdventures, kLangSpanish), 1);
	CHECK_EQ(CatalogLanguageCount(c, 1u << kCatAdventures, kLangEnglish), 0);
	CHECK_EQ(CatalogLanguageCount(c, (1u << kCatAdventures) | (1u << kCatTranslations), kLangSpanish), 2);
	CHECK_EQ(CatalogLanguageCount(c, (1u << kCatAdventures) | (1u << kCatTranslations), kLangEnglish), 1);
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
	TagsWithTitles();
	SeveralCategories();
	Matching();
	Normalize();
	Counts();
	return test::Summary("catalogfilter");
}
