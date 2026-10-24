// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "net/scenvalue.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

namespace {
std::vector<std::string> Split(const std::string &text)
{
	std::vector<std::string> out;
	size_t at = 0;
	for (;;) {
		const size_t tab = text.find('\t', at);
		if (tab == std::string::npos) {
			out.push_back(text.substr(at));
			return out;
		}
		out.push_back(text.substr(at, tab - at));
		at = tab + 1;
	}
}

std::string Field(std::string s)
{
	std::replace(s.begin(), s.end(), '\t', ' ');
	return s;
}
}

std::string ScenValueEncode(const ScenValue &v)
{
	std::string out = v.path + "\t" + std::to_string(v.size) + "\t" + v.hash;
	if (v.modId.empty()) return out;
	out += "\t" + Field(v.modId) + "\t" + Field(v.modVersion) + "\t" + Field(v.modName);
	if (v.simple) out += std::string(v.shareable ? "\t2\t" : "\t1\t") + std::to_string(v.mapSize) + "\t" + v.mapHash + "\t" + std::to_string(v.gbId);
	return out;
}

bool ScenValueParse(const std::string &text, ScenValue &out)
{
	out = ScenValue();
	const std::vector<std::string> f = Split(text);
	if (f.size() < 3 || f[0].empty()) return false;
	out.path = f[0];
	out.size = _strtoui64(f[1].c_str(), nullptr, 10);
	out.hash = f[2];
	if (f.size() >= 6 && !f[3].empty()) {
		out.modId = f[3];
		out.modVersion = f[4];
		out.modName = f[5];
	}
	if (f.size() >= 10 && (f[6] == "1" || f[6] == "2") && !out.modId.empty()) {
		out.mapSize = _strtoui64(f[7].c_str(), nullptr, 10);
		out.mapHash = f[8];
		out.gbId = _strtoui64(f[9].c_str(), nullptr, 10);
		out.simple = out.mapSize != 0 && !out.mapHash.empty();
		out.shareable = out.simple && f[6] == "2";
	}
	return true;
}
