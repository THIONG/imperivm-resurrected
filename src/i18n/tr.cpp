// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.

#include <windows.h>

#include <algorithm>
#include <mutex>
#include <unordered_map>

#include "i18n/tr.h"
#include "core/log.h"

struct TrRow {
	const char *en, *es, *it;
};
extern const TrRow kTrRows[];
extern const int kTrCount;

namespace {

struct Piece {
	bool placeholder = false;
	char kind = 's';
	std::string literal;
	std::string spec;
};
struct Pattern {
	std::vector<Piece> en;
	const TrRow *row;
};

std::mutex g_lock;
bool g_built = false;
std::unordered_map<std::string, const TrRow *> g_exact;
std::vector<Pattern> g_patterns;
std::unordered_map<std::string, std::string> g_cache[2];
std::unordered_map<std::string, bool> g_missing;
std::unordered_map<std::string, std::string> g_source;
bool g_sourceBuilt = false;

std::vector<Piece> Split(const std::string &s)
{
	std::vector<Piece> out;
	std::string lit;
	for (size_t i = 0; i < s.size(); i++) {
		if (s[i] == '%' && i + 1 < s.size()) {
			if (s[i + 1] == '%') {
				lit += '%';
				i++;
				continue;
			}
			size_t j = i + 1;
			while (j < s.size() && (isdigit(static_cast<unsigned char>(s[j])) || s[j] == '.' || s[j] == '-' || s[j] == 'l')) j++;
			if (j < s.size() && strchr("dusxXfi", s[j])) {
				if (!lit.empty()) {
					Piece p;
					p.literal = lit;
					out.push_back(p);
					lit.clear();
				}
				Piece p;
				p.placeholder = true;
				p.kind = (s[j] == 'd' || s[j] == 'u' || s[j] == 'i' || s[j] == 'x' || s[j] == 'X') ? 'd' : s[j] == 'f' ? 'f' : 's';
				p.spec = s.substr(i, j - i + 1);
				out.push_back(p);
				i = j;
				continue;
			}
		}
		lit += s[i];
	}
	if (!lit.empty()) {
		Piece p;
		p.literal = lit;
		out.push_back(p);
	}
	return out;
}

bool HasPlaceholder(const std::vector<Piece> &v)
{
	for (const Piece &p : v) {
		if (p.placeholder) return true;
	}
	return false;
}

bool Match(const std::vector<Piece> &pat, const std::string &s, std::vector<std::string> &values)
{
	values.clear();
	size_t pos = 0;
	for (size_t k = 0; k < pat.size(); k++) {
		const Piece &p = pat[k];
		if (!p.placeholder) {
			if (pos > s.size() || s.compare(pos, p.literal.size(), p.literal) != 0) return false;
			pos += p.literal.size();
			continue;
		}
		size_t end = pos;
		if (p.kind == 'd') {
			if (end < s.size() && (s[end] == '-' || s[end] == '+')) end++;
			while (end < s.size() && isdigit(static_cast<unsigned char>(s[end]))) end++;
			if (end == pos) return false;
		}
		else if (p.kind == 'f') {
			if (end < s.size() && s[end] == '-') end++;
			while (end < s.size() && (isdigit(static_cast<unsigned char>(s[end])) || s[end] == '.' || s[end] == ',')) end++;
			if (end == pos) return false;
		}
		else {
			if (k + 1 >= pat.size()) end = s.size();
			else {
				const std::string &next = pat[k + 1].literal;
				if (pat[k + 1].placeholder || next.empty()) return false;
				const size_t f = s.find(next, pos);
				if (f == std::string::npos) return false;
				end = f;
			}
		}
		values.push_back(s.substr(pos, end - pos));
		pos = end;
	}
	return pos == s.size();
}

void Build()
{
	g_built = true;
	for (int i = 0; i < kTrCount; i++) {
		const TrRow &r = kTrRows[i];
		g_exact[r.en] = &r;
		std::vector<Piece> p = Split(r.en);
		size_t fixed = 0;
		for (const Piece &x : p) fixed += x.placeholder ? 0 : x.literal.size();
		if (HasPlaceholder(p) && fixed >= 4) g_patterns.push_back({p, &r});
	}
	std::stable_sort(g_patterns.begin(), g_patterns.end(), [](const Pattern &a, const Pattern &b) {
		size_t fa = 0, fb = 0;
		for (const Piece &x : a.en) fa += x.placeholder ? 0 : x.literal.size();
		for (const Piece &x : b.en) fb += x.placeholder ? 0 : x.literal.size();
		return fa > fb;
	});
}

std::string Target(const TrRow &r, int lang)
{
	const char *t = lang == 1 ? r.it : r.es;
	return std::string(t && t[0] == '=' && !t[1] ? r.en : (t ? t : r.en));
}

std::string Fill(const std::string &target, const std::vector<std::string> &values)
{
	std::string out;
	size_t next = 0;
	for (const Piece &p : Split(target)) {
		if (!p.placeholder) {
			out += p.literal;
		}
		else {
			out += next < values.size() ? values[next] : "";
			next++;
		}
	}
	return out;
}

const std::string &(*g_languageSource)() = nullptr;

int LanguageIndex()
{
	static const std::string none;
	const std::string &language = g_languageSource ? g_languageSource() : none;
	if (language == "spanish" || language.empty()) return 0;
	if (language == "italian") return 1;
	return -1;
}

bool LogMissing()
{
	static const bool on = [] {
		char path[MAX_PATH];
		GetModuleFileNameA(nullptr, path, MAX_PATH);
		char *slash = strrchr(path, '\\');
		if (slash) *slash = 0;
		return GetPrivateProfileIntA("Debug", "LogUntranslatedTexts", 0, (std::string(path) + "\\imperivm-mod.ini").c_str()) != 0;
	}();
	return on;
}

}

void TrSetLanguageSource(const std::string &(*source)())
{
	g_languageSource = source;
}

std::string Tr(const std::string &english)
{
	if (english.empty()) return english;
	const int lang = LanguageIndex();
	if (lang < 0) return english;
	std::lock_guard<std::mutex> lock(g_lock);
	if (!g_built) Build();
	auto &cache = g_cache[lang];
	auto hit = cache.find(english);
	if (hit != cache.end()) return hit->second;
	std::string result = english;
	bool found = false;
	auto exact = g_exact.find(english);
	if (exact != g_exact.end()) {
		result = Target(*exact->second, lang);
		found = true;
	}
	else {
		std::vector<std::string> values;
		for (const Pattern &p : g_patterns) {
			if (!Match(p.en, english, values)) continue;
			result = Fill(Target(*p.row, lang), values);
			found = true;
			break;
		}
	}
	if (!found && LogMissing() && g_missing.size() < 2000 && !g_missing.count(english)) {
		bool letters = false;
		for (char c : english) {
			if (isalpha(static_cast<unsigned char>(c)) || (c & 0x80)) {
				letters = true;
				break;
			}
		}
		if (letters) {
			g_missing[english] = true;
			LogD("language", "untranslated text: %s", english.c_str());
		}
	}
	if (cache.size() > 6000) cache.clear();
	cache[english] = result;
	return result;
}

std::string TrSource(const std::string &text)
{
	if (text.empty()) return text;
	std::lock_guard<std::mutex> lock(g_lock);
	if (!g_built) Build();
	if (!g_sourceBuilt) {
		g_sourceBuilt = true;
		for (int i = 0; i < kTrCount; i++) {
			for (int lang = 0; lang < 2; lang++) {
				const std::string target = Target(kTrRows[i], lang);
				if (target != kTrRows[i].en) g_source.emplace(target, kTrRows[i].en);
			}
		}
	}
	auto hit = g_source.find(text);
	if (hit != g_source.end()) return hit->second;
	std::vector<std::string> values;
	for (const Pattern &p : g_patterns) {
		for (int lang = 0; lang < 2; lang++) {
			if (Match(Split(Target(*p.row, lang)), text, values)) return Fill(p.row->en, values);
		}
	}
	return text;
}

std::vector<std::string> TrAllLanguages(const std::string &english)
{
	std::lock_guard<std::mutex> lock(g_lock);
	if (!g_built) Build();
	const auto exact = g_exact.find(english);
	if (exact == g_exact.end()) return {english};
	return {english, Target(*exact->second, 0), Target(*exact->second, 1)};
}

std::string TrFmt(const std::string &english, const std::vector<std::string> &values)
{
	return Fill(Tr(english), values);
}

std::wstring TrW(const std::wstring &english)
{
	if (english.empty()) return english;
	if (LanguageIndex() < 0) return english;
	const int n = WideCharToMultiByte(CP_UTF8, 0, english.c_str(), static_cast<int>(english.size()), nullptr, 0, nullptr, nullptr);
	std::string utf8(n > 0 ? n : 0, '\0');
	if (n > 0) WideCharToMultiByte(CP_UTF8, 0, english.c_str(), static_cast<int>(english.size()), &utf8[0], n, nullptr, nullptr);
	const std::string t = Tr(utf8);
	if (t == utf8) return english;
	const int m = MultiByteToWideChar(CP_UTF8, 0, t.c_str(), static_cast<int>(t.size()), nullptr, 0);
	std::wstring out(m > 0 ? m : 0, L'\0');
	if (m > 0) MultiByteToWideChar(CP_UTF8, 0, t.c_str(), static_cast<int>(t.size()), &out[0], m);
	return out;
}
