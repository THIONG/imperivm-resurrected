// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct ZipLimits {
	size_t maxEntries = 0;
	uint64_t maxBytes = 0;
};

using ZipProgress = std::function<bool(uint64_t done, uint64_t total)>;

bool IsZipFile(const std::string &path);
bool ZipEntryPath(const std::string &name, std::string &rel);
std::string ZipExtract(const std::string &zip, const std::string &dest, const ZipLimits &limits, const ZipProgress &progress = nullptr);
bool ZipNotUnderstood(const std::string &error);
bool ZipCreate(const std::string &zip, const std::string &folder, const std::vector<std::string> &names);
