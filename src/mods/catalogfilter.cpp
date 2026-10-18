// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "mods/catalogfilter.h"

#include <cctype>
#include <cstring>

namespace {
struct CategoryRow {
	const char *name;
	const char *label;
	const char *aliases[9];
};

const CategoryRow kCategories[kCatCount] = {
	{"Gameplay & Balance", "Gameplay & Balance", {"gameplay", "balance", "rules", "rules and balance", "total conversions", "total conversion"}},
	{"Units & Heroes", "Units & Heroes", {"units", "heroes", "units and heroes"}},
	{"Civilizations", "Civilizations", {"civilization", "civs", "factions"}},
	{"Scenarios", "Scenarios", {"scenario", "maps", "map", "skirmish", "multiplayer maps"}},
	{"Adventures", "Adventures", {"adventure", "campaigns", "campaign", "battles", "battles and campaigns", "historical battles"}},
	{"Conquests", "Conquests", {"conquest"}},
	{"Graphics", "Graphics", {"interface", "ui", "textures", "skins", "graphics and interface"}},
	{"Audio & Voices", "Audio & Voices", {"audio", "sound", "sounds", "music", "voices", "audio and voices"}},
	{"Translations", "Translations", {"translation", "texts", "localization", "texts and translations"}},
};

struct LanguageRow {
	const char *label;
	const char *words[9];
};

const LanguageRow kLanguages[kLangCount] = {
	{"Spanish", {"spanish", "espanol", "castellano", "spagnolo"}},
	{"English", {"english", "ingles", "inglese"}},
	{"Italian", {"italian", "italiano"}},
	{"German", {"german", "aleman", "tedesco", "deutsch"}},
	{"French", {"french", "frances", "francese", "francais"}},
	{"Polish", {"polish", "polaco", "polacco", "polski"}},
	{"Several languages", {"multilingual", "multilanguage", "multi language", "multi lingual", "several languages", "multilingue",
		"multilenguaje", "varios idiomas", "piu lingue"}},
};

const char *const kAccents[][2] = {{"\xC3\xA1", "a"}, {"\xC3\xA0", "a"}, {"\xC3\xA9", "e"}, {"\xC3\xA8", "e"}, {"\xC3\xAD", "i"},
	{"\xC3\xAC", "i"}, {"\xC3\xB3", "o"}, {"\xC3\xB2", "o"}, {"\xC3\xBA", "u"}, {"\xC3\xB9", "u"}, {"\xC3\xBC", "u"}, {"\xC3\xB1", "n"},
	{"\xC3\xA7", "c"}, {"\xC3\x81", "a"}, {"\xC3\x89", "e"}, {"\xC3\x8D", "i"}, {"\xC3\x93", "o"}, {"\xC3\x9A", "u"}, {"\xC3\x91", "n"}};

std::string Fold(const std::string &s, bool keepDash = false)
{
	std::string t = s;
	for (const auto &a : kAccents) {
		for (size_t at = t.find(a[0]); at != std::string::npos; at = t.find(a[0], at)) t.replace(at, strlen(a[0]), a[1]);
	}
	std::string out;
	bool space = true;
	for (char ch : t) {
		const unsigned char c = static_cast<unsigned char>(ch);
		if (c == '&') {
			if (!space) out += ' ';
			out += "and ";
			space = true;
		}
		else if (std::isalnum(c) || (keepDash && c == '-')) {
			out += static_cast<char>(std::tolower(c));
			space = false;
		}
		else if (!space) {
			out += ' ';
			space = true;
		}
	}
	while (!out.empty() && out.back() == ' ') out.pop_back();
	return out;
}

std::vector<std::string> Words(const std::string &s)
{
	std::vector<std::string> out;
	size_t start = 0;
	while (start < s.size()) {
		size_t end = s.find(' ', start);
		if (end == std::string::npos) end = s.size();
		if (end > start) out.push_back(s.substr(start, end - start));
		start = end + 1;
	}
	return out;
}

bool HasPhrase(const std::string &folded, const char *phrase)
{
	return (" " + folded + " ").find(" " + std::string(phrase) + " ") != std::string::npos;
}

bool AllDigits(const std::string &s)
{
	if (s.empty() || s.size() > 3) return false;
	for (char c : s) {
		if (!std::isdigit(static_cast<unsigned char>(c))) return false;
	}
	return true;
}

uint32_t Range(int from, int to)
{
	if (from > to) {
		const int t = from;
		from = to;
		to = t;
	}
	uint32_t out = 0;
	for (int n = from; n <= to; n++) {
		if (n >= kCatalogMinPlayers && n <= kCatalogMaxPlayers) out |= 1u << n;
	}
	return out;
}

enum TagTitle { kTitleNone, kTitleType, kTitlePlayers, kTitleLanguage, kTitleOther };

const char *const kTypeTitles[] = {"type", "types", "category", "categories", "tipo", "tipos", "categoria", "categorias", "tipologia"};
const char *const kPlayerTitles[] = {"players", "player", "number of players", "jugadores", "giocatori"};
const char *const kLanguageTitles[] = {"language", "languages", "idioma", "idiomas", "lingua", "lingue"};

struct SplitTag {
	TagTitle title = kTitleNone;
	std::string value;
};

bool OneOf(const std::string &folded, const char *const *list, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (folded == list[i]) return true;
	}
	return false;
}

SplitTag Split(const std::string &tag)
{
	SplitTag out;
	const size_t colon = tag.find(':');
	if (colon == std::string::npos) {
		out.value = tag;
		return out;
	}
	const std::string title = Fold(tag.substr(0, colon));
	const size_t first = tag.find_first_not_of(' ', colon + 1);
	out.value = first == std::string::npos ? std::string() : tag.substr(first);
	while (!out.value.empty() && out.value.back() == ' ') out.value.pop_back();
	if (OneOf(title, kTypeTitles, sizeof(kTypeTitles) / sizeof(kTypeTitles[0]))) out.title = kTitleType;
	else if (OneOf(title, kPlayerTitles, sizeof(kPlayerTitles) / sizeof(kPlayerTitles[0]))) out.title = kTitlePlayers;
	else if (OneOf(title, kLanguageTitles, sizeof(kLanguageTitles) / sizeof(kLanguageTitles[0]))) out.title = kTitleLanguage;
	else out.title = kTitleOther;
	return out;
}

bool ParseVersus(const std::string &word, int &total)
{
	total = 0;
	int sides = 0;
	size_t i = 0;
	while (i < word.size()) {
		size_t j = i;
		while (j < word.size() && std::isdigit(static_cast<unsigned char>(word[j]))) j++;
		if (j == i || j - i > 2) return false;
		total += std::stoi(word.substr(i, j - i));
		sides++;
		if (j == word.size()) break;
		if (word[j] != 'v') return false;
		i = j + 1;
		if (i == word.size()) return false;
	}
	return sides >= 2;
}
}

const char *CatalogCategoryName(int category)
{
	return category >= 0 && category < kCatCount ? kCategories[category].name : "";
}

const char *CatalogCategoryLabel(int category)
{
	return category >= 0 && category < kCatCount ? kCategories[category].label : "";
}

int CatalogCategoryGroup(int category)
{
	return category >= 0 && category < kCatCount ? category / 3 : -1;
}

const char *CatalogLanguageLabel(int language)
{
	return language >= 0 && language < kLangCount ? kLanguages[language].label : "";
}

bool CatalogHasPlayers(int category) { return category == kCatScenarios; }

bool CatalogHasLanguage(int category) { return category == kCatAdventures || category == kCatAudio || category == kCatTranslations; }

int CatalogCategoryFromName(const std::string &name)
{
	const std::string f = Fold(name);
	if (f.empty()) return -1;
	for (int c = 0; c < kCatCount; c++) {
		if (f == Fold(kCategories[c].name)) return c;
	}
	for (int c = 0; c < kCatCount; c++) {
		for (const char *a : kCategories[c].aliases) {
			if (a && f == a) return c;
		}
	}
	return -1;
}

uint32_t CatalogPlayersFromTag(const std::string &tag)
{
	const std::string f = Fold(tag, true);
	std::string spaced = f;
	for (char &c : spaced) {
		if (c == '-') c = ' ';
	}
	const std::vector<std::string> words = Words(spaced);
	if (words.size() == 1 && AllDigits(words[0])) return Range(std::stoi(words[0]), std::stoi(words[0]));
	static const char *const kPlayerWords[] = {"player", "players", "jugador", "jugadores", "giocatore", "giocatori", "p", "j", "ffa"};
	bool playerWord = false;
	std::vector<int> numbers;
	for (const std::string &w : words) {
		for (const char *p : kPlayerWords) {
			if (w == p) playerWord = true;
		}
		int total = 0;
		if (ParseVersus(w, total)) return Range(total, total);
		if (AllDigits(w)) numbers.push_back(std::stoi(w));
		else if (w.size() >= 2 && (w.back() == 'p' || w.back() == 'j') && AllDigits(w.substr(0, w.size() - 1))) {
			numbers.push_back(std::stoi(w.substr(0, w.size() - 1)));
			playerWord = true;
		}
	}
	for (size_t i = 0; i + 2 < words.size(); i++) {
		if (AllDigits(words[i]) && (words[i + 1] == "v" || words[i + 1] == "vs") && AllDigits(words[i + 2])) {
			const int total = std::stoi(words[i]) + std::stoi(words[i + 2]);
			return Range(total, total);
		}
	}
	if (!playerWord || numbers.empty()) return 0;
	const bool upTo = HasPhrase(spaced, "up to") || HasPhrase(spaced, "hasta") || HasPhrase(spaced, "fino a") || HasPhrase(spaced, "max");
	if (upTo) return Range(kCatalogMinPlayers, numbers.back());
	const bool range = f.find('-') != std::string::npos || HasPhrase(spaced, "to") || HasPhrase(spaced, "a") || HasPhrase(spaced, "al");
	if (numbers.size() == 2 && range) return Range(numbers[0], numbers[1]);
	uint32_t out = 0;
	for (int n : numbers) out |= Range(n, n);
	return out;
}

uint32_t CatalogLanguagesFromTag(const std::string &tag)
{
	const std::string f = Fold(tag);
	uint32_t out = 0;
	for (int l = 0; l < kLangCount; l++) {
		for (const char *w : kLanguages[l].words) {
			if (w && HasPhrase(f, w)) out |= 1u << l;
		}
	}
	return out;
}

CatalogFacts CatalogFactsOf(const std::string &category, const std::vector<std::string> &tags)
{
	CatalogFacts out;
	out.category = CatalogCategoryFromName(category);
	for (const std::string &t : tags) {
		const SplitTag s = Split(t);
		int c = -1;
		if (s.title == kTitleType) c = CatalogCategoryFromName(s.value);
		else if (s.title == kTitleNone) c = CatalogCategoryFromName(t);
		if (c >= 0) {
			if (out.category < 0) out.category = c;
			out.categories |= 1u << c;
		}
		if (s.title == kTitlePlayers) out.players |= CatalogPlayersFromTag(s.value + " players");
		else if (s.title == kTitleNone) out.players |= CatalogPlayersFromTag(t);
		if (s.title == kTitleLanguage || s.title == kTitleNone) out.languages |= CatalogLanguagesFromTag(s.value);
	}
	if (out.category >= 0) out.categories |= 1u << out.category;
	return out;
}

uint32_t CatalogCategoriesOf(const CatalogFacts &facts)
{
	const uint32_t main = facts.category >= 0 && facts.category < kCatCount ? 1u << facts.category : 0u;
	return (facts.categories | main) & ((1u << kCatCount) - 1);
}

bool CatalogTagIsFacet(const std::string &tag)
{
	const SplitTag s = Split(tag);
	switch (s.title) {
	case kTitleType:
		return CatalogCategoryFromName(s.value) >= 0;
	case kTitlePlayers:
		return CatalogPlayersFromTag(s.value + " players") != 0;
	case kTitleLanguage:
		return CatalogLanguagesFromTag(s.value) != 0;
	case kTitleOther:
		return false;
	default:
		return CatalogCategoryFromName(tag) >= 0 || CatalogPlayersFromTag(tag) != 0 || CatalogLanguagesFromTag(tag) != 0;
	}
}

std::string CatalogPlayersText(uint32_t players)
{
	std::vector<int> n;
	for (int i = kCatalogMinPlayers; i <= kCatalogMaxPlayers; i++) {
		if ((players >> i) & 1) n.push_back(i);
	}
	if (n.empty()) return "";
	if (n.size() >= 3 && n.back() - n.front() + 1 == static_cast<int>(n.size())) return std::to_string(n.front()) + "-" + std::to_string(n.back());
	std::string out;
	for (int v : n) out += (out.empty() ? "" : ", ") + std::to_string(v);
	return out;
}

CatalogFilter CatalogNormalize(CatalogFilter filter)
{
	filter.types &= (1u << kCatCount) - 1;
	filter.languages &= (1u << kLangCount) - 1;
	filter.players &= Range(kCatalogMinPlayers, kCatalogMaxPlayers);
	bool players = false, language = false;
	for (int c = 0; c < kCatCount; c++) {
		if (!((filter.types >> c) & 1)) continue;
		players = players || CatalogHasPlayers(c);
		language = language || CatalogHasLanguage(c);
	}
	if (!players) filter.players = 0;
	if (!language) filter.languages = 0;
	return filter;
}

bool CatalogFilterEmpty(const CatalogFilter &filter) { return !filter.types && !filter.players && !filter.languages; }

int CatalogFilterSize(const CatalogFilter &filter)
{
	int n = 0;
	for (uint32_t m : {filter.types, filter.players, filter.languages}) {
		for (; m; m &= m - 1) n++;
	}
	return n;
}

bool CatalogMatches(const CatalogFacts &facts, const CatalogFilter &filter)
{
	const uint32_t categories = CatalogCategoriesOf(facts);
	const uint32_t candidates = filter.types ? categories & filter.types : categories;
	if (filter.types && !candidates) return false;
	if (!candidates) return true;
	for (int c = 0; c < kCatCount; c++) {
		if (!((candidates >> c) & 1)) continue;
		if (filter.players && CatalogHasPlayers(c) && !(facts.players & filter.players)) continue;
		if (filter.languages && CatalogHasLanguage(c) && !(facts.languages & (filter.languages | (1u << kLangSeveral)))) continue;
		return true;
	}
	return false;
}

void CatalogCount(const CatalogFacts &facts, CatalogCounts &counts)
{
	const uint32_t categories = CatalogCategoriesOf(facts);
	bool players = false;
	for (int c = 0; c < kCatCount; c++) {
		if (!((categories >> c) & 1)) continue;
		counts.types[c]++;
		players = players || CatalogHasPlayers(c);
		if (!CatalogHasLanguage(c)) continue;
		for (int l = 0; l < kLangCount; l++) {
			if ((facts.languages >> l) & 1) counts.languages[c][l]++;
		}
	}
	if (!players) return;
	for (int n = kCatalogMinPlayers; n <= kCatalogMaxPlayers; n++) {
		if ((facts.players >> n) & 1) counts.players[n]++;
	}
}

int CatalogLanguageCount(const CatalogCounts &counts, uint32_t types, int language)
{
	if (language < 0 || language >= kLangCount) return 0;
	int n = 0;
	for (int c = 0; c < kCatCount; c++) {
		if ((types >> c) & 1) n += counts.languages[c][language];
	}
	return n;
}
