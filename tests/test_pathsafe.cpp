// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include "core/names.h"
#include "core/pathsafe.h"

static void Expect(bool got, bool want, const char *what, const char *input)
{
	test::g_checks++;
	if (got != want) {
		std::printf("FAIL %s(\"%s\"): expected %d\n", what, input, want ? 1 : 0);
		test::g_failed++;
	}
}

int main()
{
	const char *okFolders[] = {"Community Mod", "mod-1.2", "Roma (v2)", "ñandú"};
	const char *badFolders[] = {"", ".", "..", "..\\..\\x", "a/b", "a\\b", "C:", "con", "NUL.txt", "lpt1", "x.", "x ", ".hidden", "_downloads", "a:b", "a*b", "a?b"};
	for (const char *s : okFolders) Expect(pathsafe::SafeFolderName(s), true, "SafeFolderName", s);
	for (const char *s : badFolders) Expect(pathsafe::SafeFolderName(s), false, "SafeFolderName", s);

	const char *okRel[] = {"mod.json", "data/units.xml", "a\\b\\c.pak", "Data/Img/photo.png", "COM10.txt"};
	const char *badRel[] = {"", "../x", "a/../b", "/abs", "\\abs", "C:\\x", "a//b", "dir/", "evil.dll.", "evil.dll ", "CON", "a/NUL.xml", "x:y", "a/b/"};
	for (const char *s : okRel) Expect(pathsafe::SafeRelative(s), true, "SafeRelative", s);
	for (const char *s : badRel) Expect(pathsafe::SafeRelative(s), false, "SafeRelative", s);

	Expect(PlayerNameMatches("Alexander", "alexander"), true, "PlayerNameMatches", "same");
	Expect(PlayerNameMatches("Alexander the Great of Macedon", "Alexander the Great"), true, "PlayerNameMatches", "long prefix");
	Expect(PlayerNameMatches("Alex", "Alexander"), false, "PlayerNameMatches", "short prefix");
	Expect(PlayerNameMatches("John", "Peter"), false, "PlayerNameMatches", "different");

	return test::Summary("pathsafe");
}
