// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>
#include <vector>

struct ModFeature {
	int order;
	void (*rebuild)();
	std::vector<std::string> (*problemsFor)(const std::string &folder);
};

void ModsRegisterFeature(const ModFeature &feature);
void ModsRebuildFeatures();
std::vector<std::string> ModsFeatureProblems(const std::string &folder);
