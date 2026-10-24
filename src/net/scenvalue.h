// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>
#include <string>

struct ScenValue {
	std::string path;
	uint64_t size = 0;
	std::string hash;
	std::string modId, modVersion, modName;
	bool simple = false;
	bool shareable = false;
	uint64_t mapSize = 0;
	std::string mapHash;
	uint64_t gbId = 0;
};

std::string ScenValueEncode(const ScenValue &v);
bool ScenValueParse(const std::string &text, ScenValue &out);
