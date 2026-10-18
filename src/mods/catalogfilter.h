// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

enum CatalogCategory {
	kCatGameplay,
	kCatUnits,
	kCatCivilizations,
	kCatScenarios,
	kCatAdventures,
	kCatConquests,
	kCatGraphics,
	kCatAudio,
	kCatTranslations,
	kCatCount
};

enum CatalogLanguage { kLangSpanish, kLangEnglish, kLangItalian, kLangGerman, kLangFrench, kLangPolish, kLangSeveral, kLangCount };

const int kCatalogMinPlayers = 2;
const int kCatalogMaxPlayers = 8;

struct CatalogFacts {
	int category = -1;
	uint32_t categories = 0;
	uint32_t players = 0;
	uint32_t languages = 0;
};

struct CatalogFilter {
	uint32_t types = 0;
	uint32_t players = 0;
	uint32_t languages = 0;
};

struct CatalogCounts {
	int types[kCatCount] = {};
	int players[kCatalogMaxPlayers + 1] = {};
	int languages[kCatCount][kLangCount] = {};
};

const char *CatalogCategoryName(int category);
const char *CatalogCategoryLabel(int category);
int CatalogCategoryGroup(int category);
const char *CatalogLanguageLabel(int language);
bool CatalogHasPlayers(int category);
bool CatalogHasLanguage(int category);

int CatalogCategoryFromName(const std::string &name);
uint32_t CatalogPlayersFromTag(const std::string &tag);
uint32_t CatalogLanguagesFromTag(const std::string &tag);
CatalogFacts CatalogFactsOf(const std::string &category, const std::vector<std::string> &tags);
uint32_t CatalogCategoriesOf(const CatalogFacts &facts);
bool CatalogTagIsFacet(const std::string &tag);
std::string CatalogPlayersText(uint32_t players);

CatalogFilter CatalogNormalize(CatalogFilter filter);
bool CatalogFilterEmpty(const CatalogFilter &filter);
int CatalogFilterSize(const CatalogFilter &filter);
bool CatalogMatches(const CatalogFacts &facts, const CatalogFilter &filter);
void CatalogCount(const CatalogFacts &facts, CatalogCounts &counts);
int CatalogLanguageCount(const CatalogCounts &counts, uint32_t types, int language);
