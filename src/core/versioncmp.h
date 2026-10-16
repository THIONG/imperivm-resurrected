// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>

int CompareVersions(const std::string &a, const std::string &b);

bool IsDateVersion(const std::string &v);

bool UpdateIsNewer(const std::string &available, const std::string &installed);

bool VersionIsCurrent(const std::string &installed, const std::string &available);

bool TitleHasVersion(const std::string &title, const std::string &version);

bool FileIsNewer(const std::string &versionA, unsigned long long dateA, const std::string &versionB, unsigned long long dateB);

bool VersionMatches(const std::string &have, const std::string &spec);

int CompareReleaseVersions(const std::string &a, const std::string &b);
