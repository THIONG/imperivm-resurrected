// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct GbCacheEntry {
	std::string path;
	uint64_t bytes = 0;
	int64_t lastUse = 0;
};

int64_t GbCacheNow();
std::string GbCacheKey(const std::string &url);
std::string GbCachePath(const std::string &dir, const std::string &url);
bool GbCacheIsFresh(int64_t fetched, int64_t now, int64_t ttlSeconds);
bool GbCacheRead(const std::string &dir, const std::string &url, int64_t ttlSeconds, std::string &body, bool &stale, int64_t now);
bool GbCacheWrite(const std::string &dir, const std::string &url, const std::string &body, int64_t now);
void GbCacheRemove(const std::string &dir, const std::string &url);
std::vector<std::string> GbCachePlanPrune(std::vector<GbCacheEntry> entries, uint64_t maxBytes, int64_t maxIdleSeconds, int64_t now);
uint64_t GbCachePrune(const std::string &dir, uint64_t maxBytes, int64_t maxIdleSeconds, int64_t now);
void GbCacheClear(const std::string &dir);
