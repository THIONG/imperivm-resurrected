// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct Json;

const uint64_t kGbSoundFlag = 1ull << 40;

inline bool GbIsSound(uint64_t key) { return (key & kGbSoundFlag) != 0; }
inline uint64_t GbRow(uint64_t key) { return key & ~kGbSoundFlag; }
inline uint64_t GbSoundKey(uint64_t row) { return row | kGbSoundFlag; }

struct GbPicture {
	std::string big, card, thumb;
};

const char *GbApiBase();
const char *GbModel(uint64_t key);
std::string GbApiPath(uint64_t key);
std::string GbKeyText(uint64_t key);
std::string GbCatalogId(uint64_t key);
uint64_t GbKeyFromUrl(const std::string &url);
uint64_t GbKeyFromJson(const Json &o);
void GbKeyToJson(Json &o, uint64_t key);
std::vector<GbPicture> GbPictures(const Json &submission, bool webp);
