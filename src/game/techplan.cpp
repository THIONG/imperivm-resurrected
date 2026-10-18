// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "game/techplan.h"

#include <cctype>
#include <map>
#include <set>
#include <utility>

namespace {

std::string Trim(const std::string &s)
{
	size_t a = 0, b = s.size();
	while (a < b && isspace(static_cast<unsigned char>(s[a]))) a++;
	while (b > a && isspace(static_cast<unsigned char>(s[b - 1]))) b--;
	return s.substr(a, b - a);
}

bool SameText(const std::string &a, const char *b)
{
	size_t i = 0;
	for (; i < a.size() && b[i]; i++) {
		if (tolower(static_cast<unsigned char>(a[i])) != tolower(static_cast<unsigned char>(b[i]))) return false;
	}
	return i == a.size() && !b[i];
}

struct Parsed {
	std::string command, name;
	bool playerName = false;
	std::vector<std::string> requires;
	std::vector<TechCondition> sets;
};

bool ParseCommand(const TechCommand &c, Parsed &out)
{
	if (!SameText(Trim(c.method), "research") || c.command.empty()) return false;
	out.command = c.command;
	const std::vector<std::string> t = TechParamTokens(c.param);
	for (size_t i = 0; i < t.size();) {
		const std::string &kind = t[i];
		const bool triple = i + 2 < t.size();
		if ((kind == "ReqSet" || kind == "ReqPlr") && triple) {
			if (!t[i + 1].empty()) out.requires.push_back(t[i + 1]);
			i += 3;
		}
		else if ((kind == "NameSet" || kind == "NamePlr") && triple) {
			if (out.name.empty() && !t[i + 1].empty()) {
				out.name = t[i + 1];
				out.playerName = kind == "NamePlr";
			}
			i += 3;
		}
		else if ((kind == "SetsSet" || kind == "SetsPlr") && triple) {
			if (!t[i + 1].empty()) out.sets.push_back({kind == "SetsPlr", t[i + 1], t[i + 2]});
			i += 3;
		}
		else {
			i++;
		}
	}
	return !out.name.empty();
}

std::string ConditionKey(const TechCondition &c)
{
	return std::string(c.player ? "P|" : "S|") + c.var + "=" + c.value;
}

}

std::vector<std::string> TechParamTokens(const std::string &param)
{
	std::vector<std::string> out;
	if (Trim(param).empty()) return out;
	size_t start = 0;
	for (;;) {
		const size_t comma = param.find(',', start);
		out.push_back(Trim(param.substr(start, comma == std::string::npos ? std::string::npos : comma - start)));
		if (comma == std::string::npos) break;
		start = comma + 1;
	}
	return out;
}

std::vector<TechStep> TechPlan(const std::vector<TechCommand> &commands)
{
	std::vector<Parsed> parsed;
	for (const TechCommand &c : commands) {
		Parsed p;
		if (ParseCommand(c, p)) parsed.push_back(std::move(p));
	}
	std::map<std::string, int> setters;
	for (const Parsed &p : parsed) {
		std::set<std::string> mine;
		for (const TechCondition &s : p.sets) mine.insert(ConditionKey(s));
		for (const std::string &k : mine) setters[k]++;
	}
	std::map<std::string, std::vector<size_t>> byName;
	for (size_t i = 0; i < parsed.size(); i++) byName[parsed[i].name].push_back(i);
	std::vector<int> waiting(parsed.size(), 0);
	std::vector<std::vector<size_t>> unlocks(parsed.size());
	for (size_t i = 0; i < parsed.size(); i++) {
		std::set<size_t> deps;
		for (const std::string &r : parsed[i].requires) {
			auto it = byName.find(r);
			if (it == byName.end()) continue;
			for (size_t d : it->second) {
				if (d != i) deps.insert(d);
			}
		}
		waiting[i] = static_cast<int>(deps.size());
		for (size_t d : deps) unlocks[d].push_back(i);
	}
	std::vector<size_t> order;
	std::vector<bool> done(parsed.size(), false);
	for (;;) {
		size_t pick = parsed.size();
		for (size_t i = 0; i < parsed.size(); i++) {
			if (!done[i] && waiting[i] == 0) {
				pick = i;
				break;
			}
		}
		if (pick == parsed.size()) {
			for (size_t i = 0; i < parsed.size(); i++) {
				if (!done[i]) {
					pick = i;
					break;
				}
			}
		}
		if (pick == parsed.size()) break;
		done[pick] = true;
		order.push_back(pick);
		for (size_t u : unlocks[pick]) waiting[u]--;
	}
	std::vector<TechStep> out;
	for (size_t i : order) {
		const Parsed &p = parsed[i];
		TechStep step;
		step.command = p.command;
		step.name = p.name;
		step.playerName = p.playerName;
		std::set<std::string> seen;
		for (const TechCondition &s : p.sets) {
			const std::string key = ConditionKey(s);
			if (setters[key] > 1 && seen.insert(key).second) step.choices.push_back(s);
		}
		out.push_back(std::move(step));
	}
	return out;
}
