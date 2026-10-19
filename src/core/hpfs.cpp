// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "core/hpfs.h"

#include <algorithm>
#include <cstring>

namespace {
const char kMagic[4] = {'H', 'P', 'F', 'S'};
const size_t kRootOffset = 0x1c;
const int kMaxDepth = 4;
const int kMaxDirectoryDepth = 12;
const size_t kMaxEntries = 100000;

uint32_t Le32(const unsigned char *p)
{
	return static_cast<uint32_t>(p[0]) | static_cast<uint32_t>(p[1]) << 8 | static_cast<uint32_t>(p[2]) << 16 | static_cast<uint32_t>(p[3]) << 24;
}

uint16_t Le16(const unsigned char *p) { return static_cast<uint16_t>(p[0] | p[1] << 8); }

char Lower(char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 32) : c; }

bool SameName(const std::string &a, const std::string &b)
{
	if (a.size() != b.size()) return false;
	for (size_t i = 0; i < a.size(); i++) {
		const char x = a[i] == '\\' ? '/' : Lower(a[i]), y = b[i] == '\\' ? '/' : Lower(b[i]);
		if (x != y) return false;
	}
	return true;
}
}

bool HpfsFile::Open(const unsigned char *data, size_t size)
{
	entries_.clear();
	visited_.clear();
	data_ = nullptr;
	size_ = 0;
	if (!data || size < 0x20 || memcmp(data, kMagic, sizeof(kMagic)) != 0) return false;
	const uint32_t pageSize = Le32(data + 4);
	if (pageSize < 64 || pageSize > (1u << 20) || (pageSize & (pageSize - 1)) != 0) return false;
	const uint32_t root = Le32(data + kRootOffset);
	if (static_cast<uint64_t>(root) * pageSize + pageSize > size) return false;
	data_ = data;
	size_ = size;
	pageSize_ = pageSize;
	if (!List(root, "", 0)) {
		entries_.clear();
		data_ = nullptr;
		return false;
	}
	return true;
}

bool HpfsFile::ReadHandle(uint32_t handle, std::vector<unsigned char> &out, uint32_t limit) const
{
	out.clear();
	const uint64_t rootAt = static_cast<uint64_t>(handle) * pageSize_;
	if (!data_ || rootAt + 8 > size_) return false;
	const unsigned char *root = data_ + rootAt;
	const uint32_t length = Le32(root);
	const int depth = Le16(root + 4);
	if (length > limit || depth > kMaxDepth) return false;
	out.assign(length, 0);
	const uint32_t perPage = pageSize_ / 4;
	const uint64_t pages = (static_cast<uint64_t>(length) + pageSize_ - 1) / pageSize_;
	const auto word = [&](uint32_t page, uint32_t index, uint32_t &value) {
		const uint64_t at = static_cast<uint64_t>(page) * pageSize_ + static_cast<uint64_t>(index) * 4;
		if (at + 4 > size_) return false;
		value = Le32(data_ + at);
		return true;
	};
	for (uint64_t p = 0; p < pages; p++) {
		uint32_t idx[kMaxDepth + 1] = {};
		uint64_t q = p;
		for (int i = depth; i >= 1; i--) {
			idx[i] = static_cast<uint32_t>(q % perPage);
			q /= perPage;
		}
		if (q + 2 >= perPage) return false;
		idx[0] = static_cast<uint32_t>(q) + 2;
		uint32_t cur = handle;
		for (int l = 0; l < depth; l++) {
			if (!word(cur, idx[l], cur)) return false;
		}
		uint32_t dataPage = 0;
		if (!word(cur, idx[depth], dataPage)) return false;
		const uint64_t from = static_cast<uint64_t>(dataPage) * pageSize_;
		if (from >= size_) return false;
		const size_t n = static_cast<size_t>(std::min<uint64_t>({pageSize_, length - p * pageSize_, size_ - from}));
		memcpy(out.data() + p * pageSize_, data_ + from, n);
	}
	return true;
}

bool HpfsFile::List(uint32_t handle, const std::string &prefix, int depth)
{
	if (depth > kMaxDirectoryDepth) return false;
	if (std::find(visited_.begin(), visited_.end(), handle) != visited_.end()) return false;
	visited_.push_back(handle);
	std::vector<unsigned char> dir;
	if (!ReadHandle(handle, dir, 16u << 20)) return false;
	size_t at = 0;
	while (at + 8 <= dir.size()) {
		const uint32_t entryHandle = Le32(dir.data() + at);
		const uint16_t attributes = Le16(dir.data() + at + 4);
		const uint16_t nameLength = Le16(dir.data() + at + 6);
		if (!nameLength && !entryHandle) break;
		if (at + 8 + nameLength > dir.size() || entries_.size() >= kMaxEntries) return false;
		HpfsEntry e;
		e.path = prefix + std::string(reinterpret_cast<const char *>(dir.data() + at + 8), nameLength);
		e.handle = entryHandle;
		e.directory = (attributes & 1) != 0;
		at += 8 + nameLength;
		const uint64_t entryAt = static_cast<uint64_t>(entryHandle) * pageSize_;
		if (entryAt + 4 > size_) return false;
		e.size = Le32(data_ + entryAt);
		entries_.push_back(e);
		if (e.directory && !List(entryHandle, e.path + "/", depth + 1)) return false;
	}
	return true;
}

const HpfsEntry *HpfsFile::Find(const std::string &path) const
{
	for (const HpfsEntry &e : entries_) {
		if (!e.directory && SameName(e.path, path)) return &e;
	}
	return nullptr;
}

bool HpfsFile::Read(const HpfsEntry &entry, std::vector<unsigned char> &out, uint32_t limit) const
{
	return !entry.directory && ReadHandle(entry.handle, out, limit);
}
