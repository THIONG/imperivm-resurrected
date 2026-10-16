// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct PakEntry {
	std::string name;
	uint32_t offset = 0;
	uint32_t size = 0;
};

bool ReadPakIndex(const std::string &path, std::vector<PakEntry> &out);
