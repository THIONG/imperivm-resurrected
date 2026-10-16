// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <string>

struct Sha256 {
	BCRYPT_ALG_HANDLE alg = nullptr;
	BCRYPT_HASH_HANDLE h = nullptr;
	Sha256()
	{
		if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0) {
			BCryptCreateHash(alg, &h, nullptr, 0, nullptr, 0, 0);
		}
	}
	~Sha256()
	{
		if (h) BCryptDestroyHash(h);
		if (alg) BCryptCloseAlgorithmProvider(alg, 0);
	}
	void Add(const void *data, size_t n)
	{
		if (h) BCryptHashData(h, static_cast<PUCHAR>(const_cast<void *>(data)), static_cast<ULONG>(n), 0);
	}
	void Add(const std::string &s) { Add(s.data(), s.size()); }
	std::string Hex()
	{
		unsigned char out[32] = {};
		if (h) BCryptFinishHash(h, out, sizeof(out), 0);
		static const char *digits = "0123456789abcdef";
		std::string s;
		for (unsigned char c : out) {
			s += digits[c >> 4];
			s += digits[c & 15];
		}
		return s;
	}
};

std::string HashString(const std::string &s);

std::string HashFile(const std::string &path);
