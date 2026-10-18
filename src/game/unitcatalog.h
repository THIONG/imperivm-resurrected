// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>
#include <vector>

struct EditTreeSpot {
	std::string folder, leaf, leafContext;
};

bool EditTreeUnit(const std::string &path, EditTreeSpot &out);

struct CatalogClass {
	std::string id;
	std::string name;
	std::string leafName;
	std::string groupName;
	int groupOrder = 1000;
	bool hero = false;
};

struct CatalogUnit {
	std::string id, name;
	int group = 0;
	bool hero = false;
};

struct UnitCatalog {
	std::vector<std::string> groups;
	std::vector<CatalogUnit> units;
};

UnitCatalog BuildUnitCatalog(const std::vector<CatalogClass> &classes);
