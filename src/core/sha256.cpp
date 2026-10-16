// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "core/sha256.h"

#include <vector>

std::string HashString(const std::string &s)
{
	Sha256 sha;
	sha.Add(s);
	return sha.Hex();
}

std::string HashFile(const std::string &path)
{
	HANDLE f = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (f == INVALID_HANDLE_VALUE) {
		return "";
	}
	Sha256 sha;
	std::vector<char> buf(1 << 20);
	DWORD n = 0;
	while (ReadFile(f, buf.data(), static_cast<DWORD>(buf.size()), &n, nullptr) && n) {
		sha.Add(buf.data(), n);
	}
	CloseHandle(f);
	return sha.Hex();
}
