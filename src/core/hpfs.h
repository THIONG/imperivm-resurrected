// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct HpfsEntry {
	std::string path;
	uint32_t handle = 0;
	uint32_t size = 0;
	bool directory = false;
};

class HpfsFile {
public:
	bool Open(const unsigned char *data, size_t size);
	const std::vector<HpfsEntry> &Entries() const { return entries_; }
	const HpfsEntry *Find(const std::string &path) const;
	bool Read(const HpfsEntry &entry, std::vector<unsigned char> &out, uint32_t limit = 64u << 20) const;

private:
	bool ReadHandle(uint32_t handle, std::vector<unsigned char> &out, uint32_t limit) const;
	bool List(uint32_t handle, const std::string &prefix, int depth);

	const unsigned char *data_ = nullptr;
	size_t size_ = 0;
	uint32_t pageSize_ = 0;
	std::vector<HpfsEntry> entries_;
	std::vector<uint32_t> visited_;
};
