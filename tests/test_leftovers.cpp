// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include "mods/leftovers.h"

namespace {

void InterruptedDownloadsAreLeftovers()
{
	CHECK(IsDownloadLeftover("Imperivm Greek Civilization Mod-20261017-222633.zip", false));
	CHECK(IsDownloadLeftover("caesar mod-20261018-010203.7z", false));
	CHECK(IsDownloadLeftover("list-20261018-010203.txt", false));
	CHECK(IsDownloadLeftover("update-0.32.14.zip", false));
	CHECK(IsDownloadLeftover("tmp-20261018-010203", true));
	CHECK(IsDownloadLeftover("update-0.32.14", true));
	CHECK(IsDownloadLeftover("room", true));
	CHECK(IsDownloadLeftover("ROOM", true));
}

void StateIsKept()
{
	CHECK(!IsDownloadLeftover("swaps.txt", false));
	CHECK(!IsDownloadLeftover("backup", true));
	CHECK(!IsDownloadLeftover("pending", true));
	CHECK(!IsDownloadLeftover("Caesar Mod", true));
	CHECK(!IsDownloadLeftover("tmp-notes", true));
	CHECK(!IsDownloadLeftover("notes.txt", false));
	CHECK(!IsDownloadLeftover("mod-2026101-222633.zip", false));
	CHECK(!IsDownloadLeftover("mod-20261017x222633.zip", false));
	CHECK(!IsDownloadLeftover("mod-20261017-22a633.zip", false));
	CHECK(!IsDownloadLeftover("mod-20261017-222633", false));
	CHECK(!IsDownloadLeftover("update-.zip", false));
	CHECK(!IsDownloadLeftover("room", false));
	CHECK(!IsDownloadLeftover("", false));
	CHECK(!IsDownloadLeftover("..", true));
}

}

int main()
{
	InterruptedDownloadsAreLeftovers();
	StateIsKept();
	return test::Summary("leftovers");
}
