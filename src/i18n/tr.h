// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once

#include <string>
#include <vector>

std::string Tr(const std::string &english);

void TrSetLanguageSource(const std::string &(*source)());

std::wstring TrW(const std::wstring &english);

std::string TrFmt(const std::string &english, const std::vector<std::string> &values);

std::vector<std::string> TrAllLanguages(const std::string &english);

std::string TrSource(const std::string &text);
