// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include "core/versioncmp.h"

static void Compare()
{
	CHECK_EQ(CompareVersions("1.0", "1.0"), 0);
	CHECK_EQ(CompareVersions("1.2", "1.10"), -1);
	CHECK_EQ(CompareVersions("1.10", "1.2"), 1);
	CHECK_EQ(CompareVersions("2", "1.9.9"), 1);
	CHECK_EQ(CompareVersions("1.0", "1.0.0"), 0);
	CHECK_EQ(CompareVersions("1.0", "1.0.1"), -1);
	CHECK_EQ(CompareVersions("", ""), 0);
	CHECK_EQ(CompareVersions("", "0.1"), -1);
	CHECK_EQ(CompareVersions("v1.5", "1.5"), 0);
	CHECK_EQ(CompareVersions("1.5-beta", "1.5"), 0);
	CHECK_EQ(CompareVersions("0.31.10", "0.31.9"), 1);
}

static void Matches()
{
	CHECK(VersionMatches("1.0", ""));
	CHECK(VersionMatches("1.0", "*"));
	CHECK(VersionMatches("1.0", "1.0"));
	CHECK(!VersionMatches("1.1", "1.0"));
	CHECK(VersionMatches("1.0", "=1.0"));
	CHECK(VersionMatches("1.2", ">=1.2"));
	CHECK(VersionMatches("1.3", ">=1.2"));
	CHECK(!VersionMatches("1.1", ">=1.2"));
	CHECK(VersionMatches("1.2", "<=1.2"));
	CHECK(!VersionMatches("1.3", "<=1.2"));
	CHECK(VersionMatches("1.3", ">1.2"));
	CHECK(!VersionMatches("1.2", ">1.2"));
	CHECK(VersionMatches("1.1", "<1.2"));
	CHECK(!VersionMatches("1.2", "<1.2"));
	CHECK(VersionMatches("1.10", ">1.9"));
}

static void Release()
{
	CHECK_EQ(CompareReleaseVersions("0.31.53", "0.31.53"), 0);
	CHECK_EQ(CompareReleaseVersions("0.31.54", "0.31.53"), 1);
	CHECK_EQ(CompareReleaseVersions("0.31.9", "0.31.10"), -1);
	CHECK_EQ(CompareReleaseVersions("0.32", "0.31.99"), 1);
	CHECK_EQ(CompareReleaseVersions("1.0", "1.0.0"), 0);
	CHECK_EQ(CompareReleaseVersions("v1.2.3", "1.2.3"), 0);
	CHECK_EQ(CompareReleaseVersions("1.0.0.1", "1.0"), 1);
	CHECK_EQ(CompareReleaseVersions("", ""), 0);
}

int main()
{
	Compare();
	Matches();
	Release();
	return test::Summary("versions");
}
