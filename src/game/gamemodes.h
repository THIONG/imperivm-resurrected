// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>
#include <vector>

struct GameMode {
	std::string key;
	std::string modFolder;
	int gold = -2;
	int population = -1;
	int noFog = -1;
	int explored = -1;
	int bonus = -1;
	std::vector<int> civs;
	std::vector<std::string> civNames;
};

const GameMode *GameModeFor(const char *key);
const GameMode *GameModeOfSettings(const char *settings);
bool GameModeAllows(const GameMode &mode, int race);
std::vector<int> GameModeRandomPool(const GameMode &mode, bool ai);
int GameModeFirstAllowed(const GameMode &mode, bool ai);
bool GameModeLocksOption(const GameMode *mode, int optionId);
void GameModeApplyOptions(char *opts);
void GameModeApplySettings(char *settings);
