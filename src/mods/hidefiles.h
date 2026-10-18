// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <set>
#include <string>
#include <vector>

const char kHiddenFile[] = "hidden.txt";

std::string HiddenNormalize(const std::string &path);
std::vector<std::string> HiddenParse(const std::string &text, const std::string &language);
std::string HiddenFindDir(const char *pattern);
std::set<std::string> HiddenEffective(const std::vector<std::vector<std::string>> &hides, const std::set<std::string> &provided);
