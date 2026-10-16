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

bool IsDateVersion(const std::string &v)
{
	if (v.size() != 10 || v[4] != '.' || v[7] != '.') return false;
	for (size_t i : {0u, 1u, 2u, 3u, 5u, 6u, 8u, 9u}) {
		if (!isdigit(static_cast<unsigned char>(v[i]))) return false;
	}
	const int year = atoi(v.c_str());
	return year >= 2000 && year <= 2100;
}

bool UpdateIsNewer(const std::string &available, const std::string &installed)
{
	if (available.empty()) return false;
	if (IsDateVersion(available) != IsDateVersion(installed)) return false;
	return CompareVersions(available, installed) > 0;
}

bool VersionIsCurrent(const std::string &installed, const std::string &available)
{
	return !UpdateIsNewer(available, installed);
}

bool TitleHasVersion(const std::string &title, const std::string &version)
{
	if (title.empty() || version.empty()) return false;
	const auto partOfNumber = [](char c) { return isdigit(static_cast<unsigned char>(c)) || c == '.'; };
	for (size_t at = title.find(version); at != std::string::npos; at = title.find(version, at + 1)) {
		const bool startOk = at == 0 || !partOfNumber(title[at - 1]);
		const size_t end = at + version.size();
		const bool endOk = end == title.size() || !isdigit(static_cast<unsigned char>(title[end])) &&
			!(title[end] == '.' && end + 1 < title.size() && isdigit(static_cast<unsigned char>(title[end + 1])));
		if (startOk && endOk) return true;
	}
	return false;
}

bool FileIsNewer(const std::string &versionA, unsigned long long dateA, const std::string &versionB, unsigned long long dateB)
{
	if (versionA.empty() != versionB.empty()) return !versionA.empty();
	if (!versionA.empty()) {
		const int order = CompareVersions(versionA, versionB);
		if (order != 0) return order > 0;
	}
	return dateA > dateB;
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
