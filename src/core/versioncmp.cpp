// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "core/versioncmp.h"

#include <cctype>
#include <cstdlib>
#include <cstring>

namespace {

bool StartsWith(const std::string &s, const char *prefix)
{
	return s.compare(0, strlen(prefix), prefix) == 0;
}

}

int CompareVersions(const std::string &a, const std::string &b)
{
	size_t i = 0, j = 0;
	while (i < a.size() || j < b.size()) {
		size_t si = i, sj = j;
		while (si < a.size() && a[si] != '.' && !isdigit(static_cast<unsigned char>(a[si]))) si++;
		while (sj < b.size() && b[sj] != '.' && !isdigit(static_cast<unsigned char>(b[sj]))) sj++;
		long x = strtol(a.c_str() + si, nullptr, 10), y = strtol(b.c_str() + sj, nullptr, 10);
		if (x != y) {
			return x < y ? -1 : 1;
		}
		i = a.find('.', i);
		j = b.find('.', j);
		i = i == std::string::npos ? a.size() : i + 1;
		j = j == std::string::npos ? b.size() : j + 1;
	}
	return 0;
}

bool VersionMatches(const std::string &have, const std::string &spec)
{
	if (spec.empty() || spec == "*") {
		return true;
	}
	if (StartsWith(spec, ">=")) return CompareVersions(have, spec.substr(2)) >= 0;
	if (StartsWith(spec, "<=")) return CompareVersions(have, spec.substr(2)) <= 0;
	if (StartsWith(spec, ">")) return CompareVersions(have, spec.substr(1)) > 0;
	if (StartsWith(spec, "<")) return CompareVersions(have, spec.substr(1)) < 0;
	if (StartsWith(spec, "=")) return CompareVersions(have, spec.substr(1)) == 0;
	return CompareVersions(have, spec) == 0;
}

int CompareReleaseVersions(const std::string &a, const std::string &b)
{
	size_t i = 0, j = 0;
	while (i < a.size() || j < b.size()) {
		unsigned long x = 0, y = 0;
		while (i < a.size() && !isdigit(static_cast<unsigned char>(a[i]))) i++;
		while (j < b.size() && !isdigit(static_cast<unsigned char>(b[j]))) j++;
		while (i < a.size() && isdigit(static_cast<unsigned char>(a[i]))) x = x * 10 + (a[i++] - '0');
		while (j < b.size() && isdigit(static_cast<unsigned char>(b[j]))) y = y * 10 + (b[j++] - '0');
		if (x != y) return x < y ? -1 : 1;
	}
	return 0;
}
