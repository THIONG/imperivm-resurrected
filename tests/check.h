// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace test {

inline int g_checks = 0;
inline int g_failed = 0;

inline std::string Show(const std::string &s)
{
	std::string out = "\"";
	for (char c : s) {
		if (c == '\n') out += "\\n";
		else if (c == '\r') out += "\\r";
		else if (c == '\t') out += "\\t";
		else out += c;
	}
	return out + "\"";
}

inline std::string Show(int v) { return std::to_string(v); }
inline std::string Show(unsigned v) { return std::to_string(v); }
inline std::string Show(long v) { return std::to_string(v); }
inline std::string Show(unsigned long v) { return std::to_string(v); }
inline std::string Show(unsigned long long v) { return std::to_string(v); }
inline std::string Show(bool v) { return v ? "true" : "false"; }
inline std::string Show(const char *s) { return Show(std::string(s ? s : "(null)")); }

inline std::string Show(const std::vector<std::string> &list)
{
	std::string out = "[";
	for (size_t i = 0; i < list.size(); i++) out += (i ? ", " : "") + Show(list[i]);
	return out + "]";
}

inline std::string TempDir(const char *name)
{
	char base[MAX_PATH];
	GetTempPathA(MAX_PATH, base);
	const std::string dir = std::string(base) + "imperivm-test-" + name + "-" + std::to_string(GetCurrentProcessId());
	CreateDirectoryA(dir.c_str(), nullptr);
	return dir;
}

inline void WriteFile(const std::string &path, const std::string &data)
{
	for (size_t i = 3; i < path.size(); i++) {
		if (path[i] == '\\' || path[i] == '/') CreateDirectoryA(path.substr(0, i).c_str(), nullptr);
	}
	FILE *f = nullptr;
	if (fopen_s(&f, path.c_str(), "wb") == 0 && f) {
		fwrite(data.data(), 1, data.size(), f);
		fclose(f);
	}
}

inline std::string ReadFile(const std::string &path)
{
	std::string out;
	FILE *f = nullptr;
	if (fopen_s(&f, path.c_str(), "rb") != 0 || !f) return out;
	char buf[4096];
	size_t n;
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out.append(buf, n);
	fclose(f);
	return out;
}

inline void SetFileTimeValue(const std::string &path, unsigned long long value)
{
	HANDLE f = CreateFileA(path.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (f == INVALID_HANDLE_VALUE) return;
	FILETIME ft;
	ft.dwLowDateTime = static_cast<DWORD>(value & 0xffffffff);
	ft.dwHighDateTime = static_cast<DWORD>(value >> 32);
	SetFileTime(f, nullptr, nullptr, &ft);
	CloseHandle(f);
}

inline bool Exists(const std::string &path)
{
	return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

inline void RemoveTree(const std::string &dir)
{
	std::string pattern = dir + "\\*";
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA(pattern.c_str(), &fd);
	if (h != INVALID_HANDLE_VALUE) {
		do {
			const std::string name = fd.cFileName;
			if (name == "." || name == "..") continue;
			const std::string full = dir + "\\" + name;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) RemoveTree(full);
			else DeleteFileA(full.c_str());
		} while (FindNextFileA(h, &fd));
		FindClose(h);
	}
	RemoveDirectoryA(dir.c_str());
}

inline int Summary(const char *name)
{
	std::printf("%s: %d checks, %d failed\n", name, g_checks, g_failed);
	return g_failed ? 1 : 0;
}

}

inline void Check(bool ok, const std::string &what)
{
	test::g_checks++;
	if (ok) return;
	test::g_failed++;
	std::printf("FAIL: %s\n", what.c_str());
}

#define CHECK(cond)                                                                      \
	do {                                                                                 \
		test::g_checks++;                                                                \
		if (!(cond)) {                                                                   \
			test::g_failed++;                                                            \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                  \
		}                                                                                \
	} while (0)

#define CHECK_EQ(actual, expected)                                                                                         \
	do {                                                                                                                   \
		test::g_checks++;                                                                                                  \
		const auto &a_ = (actual);                                                                                         \
		const auto &e_ = (expected);                                                                                       \
		if (!(a_ == e_)) {                                                                                                 \
			test::g_failed++;                                                                                              \
			std::printf("FAIL %s:%d: %s\n  got      %s\n  expected %s\n", __FILE__, __LINE__, #actual, test::Show(a_).c_str(), \
				test::Show(e_).c_str());                                                                                   \
		}                                                                                                                  \
	} while (0)
