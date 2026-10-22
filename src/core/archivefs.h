// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <ctime>
#include <string>

#include <windows.h>

namespace archivefs {
std::wstring Widen(const std::string &s, UINT codepage);
std::string Utf8(const std::wstring &w);
std::wstring LongPath(const std::string &path);
void MakeDirs(const std::wstring &path);
FILETIME FileTimeOf(time_t t);
}
