// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>

#include "core/logfiles.h"

namespace {

void NewSession(const LogPaths &p, const std::string &name)
{
	test::WriteFile(p.current, name);
}

void Rotation(const std::string &dir)
{
	const LogPaths p = MakeLogPaths(dir + "\\rotation");
	LogFilesState state = PrepareLogFiles(p);
	CHECK(!state.previousUnclean);
	CHECK(!state.otherInstance);
	CHECK(state.previousLog.empty());
	CHECK(test::Exists(p.logsDir));
	CHECK(test::Exists(p.modDir));
	for (const std::string &file : {p.current, p.crashed, p.lock, p.Numbered(1), p.Numbered(4), p.ForProcess(7)}) {
		CHECK(file.compare(0, p.modDir.size() + 1, p.modDir + "\\") == 0);
	}

	for (int i = 1; i <= 7; i++) {
		NewSession(p, "session " + std::to_string(i));
		state = PrepareLogFiles(p);
		CHECK(!state.previousUnclean);
		CHECK_EQ(state.previousLog, p.Numbered(1));
	}
	CHECK_EQ(test::ReadFile(p.Numbered(1)), "session 7");
	CHECK_EQ(test::ReadFile(p.Numbered(2)), "session 6");
	CHECK_EQ(test::ReadFile(p.Numbered(3)), "session 5");
	CHECK_EQ(test::ReadFile(p.Numbered(4)), "session 4");
	CHECK(!test::Exists(p.Numbered(5)));
	CHECK(!test::Exists(p.current));
}

void Unclean(const std::string &dir)
{
	const LogPaths p = MakeLogPaths(dir + "\\unclean");
	PrepareLogFiles(p);
	NewSession(p, "clean one");
	PrepareLogFiles(p);
	NewSession(p, "the bad one");
	test::WriteFile(p.lock, "3999999 12345\n");

	LogFilesState state = PrepareLogFiles(p);
	CHECK(state.previousUnclean);
	CHECK(!state.otherInstance);
	CHECK_EQ(state.previousLog, p.crashed);
	CHECK_EQ(test::ReadFile(p.crashed), "the bad one");
	CHECK_EQ(test::ReadFile(p.Numbered(1)), "clean one");
	CHECK(!test::Exists(p.current));

	NewSession(p, "next session");
	test::WriteFile(p.lock, "garbage");
	state = PrepareLogFiles(p);
	CHECK(state.previousUnclean);
	CHECK_EQ(test::ReadFile(p.crashed), "next session");

	RemoveSessionLock(p);
	NewSession(p, "fine again");
	state = PrepareLogFiles(p);
	CHECK(!state.previousUnclean);
	CHECK_EQ(state.previousLog, p.Numbered(1));
	CHECK_EQ(test::ReadFile(p.crashed), "next session");
}

void Alive(const std::string &dir)
{
	const LogPaths p = MakeLogPaths(dir + "\\alive");
	PrepareLogFiles(p);
	NewSession(p, "running now");
	CHECK(WriteSessionLock(p));
	const LogFilesState state = PrepareLogFiles(p);
	CHECK(state.otherInstance);
	CHECK(!state.previousUnclean);
	CHECK_EQ(test::ReadFile(p.current), "running now");
	CHECK(!test::Exists(p.Numbered(1)));
	RemoveSessionLock(p);
	CHECK(!test::Exists(p.lock));
	CHECK(p.ForProcess(42).find("imperivm-mod.pid42.log") != std::string::npos);
}

void Legacy(const std::string &dir)
{
	const std::string game = dir + "\\legacy";
	test::WriteFile(game + "\\imperivm-mod.log", "old current");
	test::WriteFile(game + "\\imperivm-mod.previous.log", "old previous");
	test::WriteFile(game + "\\imperivm-mod.anterior.log", "old anterior");
	test::WriteFile(game + "\\imperivm-mod-amigo.log", "not ours");
	const LogPaths p = MakeLogPaths(game);
	const LogFilesState state = PrepareLogFiles(p);
	CHECK_EQ(state.previousLog, p.Numbered(1));
	CHECK_EQ(test::ReadFile(p.Numbered(1)), "old current");
	CHECK_EQ(test::ReadFile(p.Numbered(2)), "old previous");
	CHECK_EQ(test::ReadFile(p.Numbered(3)), "old anterior");
	CHECK(!test::Exists(game + "\\imperivm-mod.log"));
	CHECK(!test::Exists(game + "\\imperivm-mod.previous.log"));
	CHECK(!test::Exists(game + "\\imperivm-mod.anterior.log"));
	CHECK_EQ(test::ReadFile(game + "\\imperivm-mod-amigo.log"), "not ours");

	NewSession(p, "new one");
	PrepareLogFiles(p);
	CHECK_EQ(test::ReadFile(p.Numbered(1)), "new one");
	CHECK_EQ(test::ReadFile(p.Numbered(2)), "old current");
	CHECK_EQ(test::ReadFile(p.Numbered(4)), "old anterior");
}

const unsigned long long kBase = 133000000000000000ull;
const unsigned long long kSecond = 10000000ull;

void CrashFolders(const std::string &dir)
{
	const LogPaths p = MakeLogPaths(dir + "\\crashes");
	PrepareLogFiles(p);
	const std::string first = p.logsDir + "\\PLAYER-01.00.00\\vx.log", second = p.logsDir + "\\PLAYER-02.00.00\\vx.log";
	test::WriteFile(first, "old");
	test::WriteFile(second, "new");
	test::WriteFile(p.logsDir + "\\PLAYER-03.00.00-192.168.1.2\\desync.txt", "not a crash");
	test::SetFileTimeValue(first, kBase);
	test::SetFileTimeValue(second, kBase + 100 * kSecond);
	CHECK_EQ(NewestGameCrashFolder(p, kBase + 50 * kSecond), "PLAYER-02.00.00");
	CHECK_EQ(NewestGameCrashFolder(p, kBase), "PLAYER-02.00.00");
	CHECK_EQ(NewestGameCrashFolder(p, kBase + 200 * kSecond), "");

	NewSession(p, "died");
	test::WriteFile(p.lock, "3999999 " + std::to_string(kBase + 60 * kSecond) + "\n");
	LogFilesState state = PrepareLogFiles(p);
	CHECK(state.previousUnclean);
	CHECK_EQ(state.previousStart, kBase + 60 * kSecond);
	CHECK_EQ(state.gameCrashReport, "PLAYER-02.00.00");

	NewSession(p, "died again");
	test::WriteFile(p.lock, "3999999 " + std::to_string(kBase + 300 * kSecond) + "\n");
	state = PrepareLogFiles(p);
	CHECK(state.previousUnclean);
	CHECK(state.gameCrashReport.empty());
}

void OldTraces(const std::string &dir)
{
	const LogPaths p = MakeLogPaths(dir + "\\traces");
	for (const char *name : {"threads.txt", "ai-history.txt", "host.txt", "return-before.txt", "return.txt", "objects-host.bin", "return-desync.txt"}) {
		test::WriteFile(p.logsDir + "\\rejoin\\" + name, "x");
	}
	PrepareLogFiles(p);
	CHECK(!test::Exists(p.logsDir + "\\rejoin"));

	test::WriteFile(p.logsDir + "\\rejoin\\threads.txt", "x");
	test::WriteFile(p.logsDir + "\\rejoin\\mine.txt", "keep me");
	PrepareLogFiles(p);
	CHECK(!test::Exists(p.logsDir + "\\rejoin\\threads.txt"));
	CHECK_EQ(test::ReadFile(p.logsDir + "\\rejoin\\mine.txt"), "keep me");
}

void Prune(const std::string &dir)
{
	const LogPaths p = MakeLogPaths(dir + "\\prune");
	PrepareLogFiles(p);
	NewSession(p, std::string(1000, 'c'));
	test::WriteFile(p.crashed, std::string(1000, 'k'));
	test::WriteFile(p.Numbered(1), std::string(1000, '1'));
	const std::string oldest = p.modDir + "\\old.dmp", middle = p.modDir + "\\sub\\mid.txt", newest = p.modDir + "\\new.dmp";
	test::WriteFile(oldest, std::string(1000, 'a'));
	test::WriteFile(middle, std::string(1000, 'b'));
	test::WriteFile(newest, std::string(1000, 'd'));
	test::SetFileTimeValue(oldest, kBase);
	test::SetFileTimeValue(middle, kBase + kSecond);
	test::SetFileTimeValue(newest, kBase + 2 * kSecond);

	PruneLogFolder(p, 10000);
	CHECK(test::Exists(oldest));
	PruneLogFolder(p, 5500);
	CHECK(!test::Exists(oldest));
	CHECK(test::Exists(middle));
	PruneLogFolder(p, 4000);
	CHECK(!test::Exists(middle));
	CHECK(test::Exists(newest));
	PruneLogFolder(p, 1);
	CHECK(!test::Exists(newest));
	CHECK(test::Exists(p.current) && test::Exists(p.crashed) && test::Exists(p.Numbered(1)));
}

}

int main()
{
	const std::string dir = test::TempDir("logfiles");
	Rotation(dir);
	Unclean(dir);
	Alive(dir);
	Legacy(dir);
	CrashFolders(dir);
	OldTraces(dir);
	Prune(dir);
	test::RemoveTree(dir);
	return test::Summary("logfiles");
}
