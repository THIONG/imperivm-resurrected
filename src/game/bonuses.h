// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>

struct BonusDef {
	std::string name;
	std::string description;
};

int BonusFileNumber(const std::string &path);
const int kBonusRoomLimit = 100;
std::string BonusScriptNote(const std::string &script, const std::string &key, const std::string &language);
std::string BonusFileKey(const std::string &path);
BonusDef BonusDefine(const std::string &listName, const std::string &path, const std::string &script, const std::string &language);
