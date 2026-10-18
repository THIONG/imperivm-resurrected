// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>
#include <functional>
#include <string>

const wchar_t *DownloadUserAgent();

struct DownloadHooks {
	std::function<void(uint64_t done, uint64_t total)> progress;
	std::function<bool()> cancelled;
};

std::string DownloadFile(const std::string &url, uint64_t expectedBytes, const std::string &toFile, const std::string &cacheDir,
	bool allowOtherServers, const DownloadHooks &hooks, bool &usedOtherServers);
