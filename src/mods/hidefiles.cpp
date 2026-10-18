// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "mods/hidefiles.h"

std::string HiddenNormalize(const std::string &path)
{
	std::string out;
	for (char c : path) {
		if (c == '\\') c = '/';
		if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + 32);
		if (c == '/' && (out.empty() || out.back() == '/')) continue;
		out += c;
	}
	while (!out.empty() && (out.back() == ' ' || out.back() == '\t' || out.back() == '\r')) out.pop_back();
	return out;
}

std::vector<std::string> HiddenParse(const std::string &text, const std::string &language)
{
	std::vector<std::string> out;
	const std::string lang = HiddenNormalize(language);
	bool on = true;
	size_t start = 0;
	while (start < text.size()) {
		size_t end = text.find('\n', start);
		if (end == std::string::npos) end = text.size();
		std::string line = text.substr(start, end - start);
		start = end + 1;
		const size_t first = line.find_first_not_of(" \t");
		if (first == std::string::npos) continue;
		line = line.substr(first);
		if (line[0] == ';' || line[0] == '#') continue;
		if (line[0] == '[') {
			const size_t close = line.find(']');
			on = close != std::string::npos && HiddenNormalize(line.substr(1, close - 1)) == lang;
			continue;
		}
		if (!on) continue;
		const std::string p = HiddenNormalize(line);
		if (p.empty() || p.back() == '/' || p.find(':') != std::string::npos || p == ".." || p.compare(0, 3, "../") == 0 ||
			p.find("/../") != std::string::npos || p.compare(0, 5, "mods/") == 0) {
			continue;
		}
		out.push_back(p);
	}
	return out;
}

std::string HiddenFindDir(const char *pattern)
{
	std::string p = HiddenNormalize(pattern ? pattern : "");
	const size_t slash = p.find_last_of('/');
	const std::string last = slash == std::string::npos ? p : p.substr(slash + 1);
	if (last.find_first_of("*?") != std::string::npos) p = slash == std::string::npos ? "" : p.substr(0, slash);
	if (!p.empty() && p.back() != '/') p += '/';
	return p;
}

std::set<std::string> HiddenEffective(const std::vector<std::vector<std::string>> &hides, const std::set<std::string> &provided)
{
	std::set<std::string> out;
	for (const std::vector<std::string> &list : hides) {
		for (const std::string &p : list) {
			if (!provided.count(p)) out.insert(p);
		}
	}
	return out;
}
