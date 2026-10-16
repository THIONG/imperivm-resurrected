// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "hooks/gamefiles.h"

#include "addr/engine.h"

namespace {

typedef void *(__cdecl *VfsOpen_t)(const char *path, unsigned mode, void *err);
typedef const char *(__fastcall *CFileRead_t)(void *self, void *edx, void *buf, unsigned len, unsigned *read);
typedef void(__fastcall *CFileDelete_t)(void *self, void *edx, int flags);

}

bool ReadGameFile(const char *path, unsigned mode, std::vector<char> &out)
{
	auto open = *reinterpret_cast<VfsOpen_t *>(addr::kVfsOpenFilePtr);
	char err[64] = {};
	void *file = open ? open(path, mode, err) : nullptr;
	if (!file) {
		return false;
	}
	void **vtable = *reinterpret_cast<void ***>(file);
	auto read = reinterpret_cast<CFileRead_t>(vtable[0xc / 4]);
	auto destroy = reinterpret_cast<CFileDelete_t>(vtable[0x8 / 4]);
	out.clear();
	char buf[65536];
	for (;;) {
		unsigned got = 0;
		const char *e = read(file, nullptr, buf, sizeof(buf), &got);
		if (got) {
			out.insert(out.end(), buf, buf + got);
		}
		if (e || got == 0) {
			break;
		}
	}
	destroy(file, nullptr, 1);
	return true;
}
