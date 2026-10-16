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

static void Current()
{
	CHECK(VersionIsCurrent("11.3", "11.2.6"));
	CHECK(VersionIsCurrent("11.2.6", "11.2.6"));
	CHECK(VersionIsCurrent("1.0", "1.0.0"));
	CHECK(VersionIsCurrent("1.10", "1.9"));
	CHECK(!VersionIsCurrent("11.2.6", "11.3"));
	CHECK(!VersionIsCurrent("1.9", "1.10"));
	CHECK(!VersionIsCurrent("", "0.1"));
	CHECK(VersionIsCurrent("1.0", ""));
	CHECK(VersionIsCurrent("", ""));
}

static void Dates()
{
	CHECK(IsDateVersion("2026.10.16"));
	CHECK(!IsDateVersion("1.2.3"));
	CHECK(!IsDateVersion("2026.1.16"));
	CHECK(!IsDateVersion("1999.10.16"));
	CHECK(!IsDateVersion("2026.10.16a"));
	CHECK(UpdateIsNewer("1.1", "1.0"));
	CHECK(!UpdateIsNewer("1.0", "1.0"));
	CHECK(!UpdateIsNewer("", "1.0"));
	CHECK(!UpdateIsNewer("2026.10.16", "1.0"));
	CHECK(!UpdateIsNewer("1.0", "2026.10.16"));
	CHECK(UpdateIsNewer("2026.10.17", "2026.10.16"));
	CHECK(!UpdateIsNewer("2026.10.16", "2026.10.17"));
	CHECK(VersionIsCurrent("1.0", "2026.10.16"));
}

static void Titles()
{
	CHECK(TitleHasVersion("1.1", "1.1"));
	CHECK(TitleHasVersion("Version 1.1", "1.1"));
	CHECK(TitleHasVersion("v1.1: new maps", "1.1"));
	CHECK(TitleHasVersion("Update 1.1 (hotfix)", "1.1"));
	CHECK(!TitleHasVersion("1.10", "1.1"));
	CHECK(!TitleHasVersion("11.1", "1.1"));
	CHECK(!TitleHasVersion("1.1.2", "1.1"));
	CHECK(TitleHasVersion("1.1.", "1.1"));
	CHECK(!TitleHasVersion("", "1.1"));
	CHECK(!TitleHasVersion("1.1", ""));
}

static void Newest()
{
	CHECK(FileIsNewer("11.3", 100, "11.2.6", 200) == true);
	CHECK(FileIsNewer("11.2.6", 200, "11.3", 100) == false);
	CHECK(FileIsNewer("1.0", 200, "1.0", 100) == true);
	CHECK(FileIsNewer("1.0", 100, "1.0", 200) == false);
	CHECK(FileIsNewer("1.0", 100, "", 200) == true);
	CHECK(FileIsNewer("", 200, "1.0", 100) == false);
	CHECK(FileIsNewer("", 200, "", 100) == true);
	CHECK(FileIsNewer("", 100, "", 200) == false);
	CHECK(FileIsNewer("1.10", 1, "1.9", 2) == true);
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
	Current();
	Newest();
	Dates();
	Titles();
	Matches();
	Release();
	return test::Summary("versions");
}
