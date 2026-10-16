// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "mods/features.h"

#include <algorithm>

namespace {

std::vector<ModFeature> &Features()
{
	static std::vector<ModFeature> features;
	return features;
}

}

void ModsRegisterFeature(const ModFeature &feature)
{
	std::vector<ModFeature> &all = Features();
	auto at = std::upper_bound(all.begin(), all.end(), feature, [](const ModFeature &a, const ModFeature &b) { return a.order < b.order; });
	all.insert(at, feature);
}

void ModsRebuildFeatures()
{
	for (const ModFeature &f : Features()) {
		if (f.rebuild) f.rebuild();
	}
}

std::vector<std::string> ModsFeatureProblems(const std::string &folder)
{
	std::vector<std::string> out;
	for (const ModFeature &f : Features()) {
		if (!f.problemsFor) continue;
		for (std::string &p : f.problemsFor(folder)) out.push_back(std::move(p));
	}
	return out;
}
