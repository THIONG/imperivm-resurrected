// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>
#include <vector>

enum MapKind { kMapUnknown = -1, kMapScenario = 0, kMapAdventure = 1, kMapConquest = 2 };

struct LooseFile {
	std::string name;
	int mapKind = kMapUnknown;
};

struct LooseMove {
	std::string file;
	std::string folder;
};

int BfhpMapKind(const std::string &head);
const char *MapKindFolder(int mapKind);
std::vector<LooseMove> PlanLooseFiles(const std::vector<LooseFile> &rootFiles, bool hasGameFolders, int category);
