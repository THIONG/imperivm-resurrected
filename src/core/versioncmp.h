// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>

int CompareVersions(const std::string &a, const std::string &b);

bool VersionMatches(const std::string &have, const std::string &spec);

int CompareReleaseVersions(const std::string &a, const std::string &b);
