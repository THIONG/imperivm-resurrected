// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "mods/pak.h"

#include <cstdio>
#include <cstring>

bool ReadPakIndex(const std::string &path, std::vector<PakEntry> &out)
{
	out.clear();
	FILE *f = nullptr;
	if (fopen_s(&f, path.c_str(), "rb") != 0 || !f) {
		return false;
	}
	unsigned char head[0x28];
	if (fread(head, 1, sizeof(head), f) != sizeof(head) || memcmp(head, "HMMSYS PackFile", 15) != 0) {
		fclose(f);
		return false;
	}
	unsigned count = 0;
	memcpy(&count, head + 0x20, 4);
	std::string prev;
	bool ok = true;
	for (unsigned i = 0; i < count; i++) {
		int len = fgetc(f), shared = fgetc(f);
		if (len < 0 || shared < 0 || shared > len || static_cast<size_t>(shared) > prev.size()) {
			ok = false;
			break;
		}
		PakEntry e;
		e.name = prev.substr(0, shared);
		e.name.resize(len);
		uint32_t tail[2];
		if (fread(&e.name[shared], 1, len - shared, f) != static_cast<size_t>(len - shared) ||
			fread(tail, 4, 2, f) != 2) {
			ok = false;
			break;
		}
		e.offset = tail[0];
		e.size = tail[1];
		prev = e.name;
		out.push_back(std::move(e));
	}
	fclose(f);
	return ok;
}
