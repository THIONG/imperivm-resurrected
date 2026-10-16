// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <algorithm>
#include <string>

#include "core/logreport.h"

namespace {

std::string UserName()
{
	char user[256] = {};
	DWORD size = sizeof(user);
	GetUserNameA(user, &size);
	return user;
}

bool Listed(const ReportResult &r, const std::string &name)
{
	return std::any_of(r.files.begin(), r.files.end(), [&](const ReportFile &f) { return f.name == name; });
}

bool Extract(const std::string &zip, const std::string &dest)
{
	char system32[MAX_PATH];
	GetSystemDirectoryA(system32, MAX_PATH);
	CreateDirectoryA(dest.c_str(), nullptr);
	const std::string command = "\"\"" + std::string(system32) + "\\tar.exe\" -x -f \"" + zip + "\" -C \"" + dest + "\"\"";
	return std::system(command.c_str()) == 0;
}

bool Has(const std::string &text, const std::string &part)
{
	return text.find(part) != std::string::npos;
}

void Report(const std::string &dir)
{
	const std::string game = dir + "\\report";
	test::RemoveTree(game);
	const LogPaths p = MakeLogPaths(game);
	const std::string user = UserName();

	test::WriteFile(p.current, "[10:00:00.000] I steam: online ready as Thiong (76561198012345678), from 83.45.12.9, in C:\\Users\\" + user + "\\x\r\n");
	test::WriteFile(p.crashed, "[09:00:00.000] I match: Maria dropped, 76561198000000001 at 5.6.7.8\r\n");
	test::WriteFile(p.Numbered(1), "[08:00:00.000] I lobby: Rafa joined, again 76561198012345678\r\n");
	test::WriteFile(p.Numbered(3), "too old\r\n");
	test::WriteFile(game + "\\imperivm-mod.ini", "[Debug]\r\nLogFiles=0\r\n[Online]\r\nName=Maria\r\nApiKey=secret-value\r\n");
	test::WriteFile(game + "\\Profiles\\Rafa\\profile.xml", "<p/>");
	test::WriteFile(p.logsDir + "\\Maria-12.00.00-5.6.7.8\\vx.log", "crash of Maria at 5.6.7.8\r\nLast savegame loaded: Profiles/Rafa/x.snr\r\n");
	test::WriteFile(p.logsDir + "\\Thiong-01.00.00\\desync.txt", "desync of Thiong\r\n");
	test::WriteFile(p.logsDir + "\\comp_out.txt", "Now compiling 'a.vs'\r\n");
	test::WriteFile(p.modDir + "\\rejoin\\threads.txt", "threads of Thiong\r\n");
	test::WriteFile(p.modDir + "\\rejoin\\objects-host.bin", "binary");

	ReportInput input;
	input.currentLog = p.current;
	input.info = "Mod version: 0.0.1\r\nWindows: test\r\n";
	input.ownName = "Thiong";
	input.otherNames = {"Maria"};
	const ReportResult r = ReportCreate(p, input);
	CHECK(r.ok);
	CHECK(r.error.empty());
	CHECK(Listed(r, "logs\\imperivm-mod.log"));
	CHECK(Listed(r, "logs\\imperivm-mod.crashed.log"));
	CHECK(Listed(r, "logs\\imperivm-mod.1.log"));
	CHECK(!Listed(r, "logs\\imperivm-mod.3.log"));
	CHECK(Listed(r, "settings\\imperivm-mod.ini"));
	CHECK(Listed(r, "game\\vx.log"));
	CHECK(Listed(r, "game\\desync.txt"));
	CHECK(Listed(r, "game\\comp_out.txt"));
	CHECK(Listed(r, "rejoin\\threads.txt"));
	CHECK(!Listed(r, "rejoin\\objects-host.bin"));
	CHECK(r.path.compare(0, ReportFolder(p).size() + 1, ReportFolder(p) + "\\") == 0);
	CHECK(test::Exists(r.path));
	if (!r.zipped) return;
	CHECK(r.path.size() > 4 && r.path.compare(r.path.size() - 4, 4, ".zip") == 0);
	CHECK(!test::Exists(r.path.substr(0, r.path.size() - 4)));

	const std::string out = dir + "\\extracted";
	test::RemoveTree(out);
	CHECK(Extract(r.path, out));
	const std::string log = test::ReadFile(out + "\\logs\\imperivm-mod.log");
	CHECK(Has(log, "online ready as <you> (<steamid1>), from <ip1>, in C:\\Users\\<user>\\x"));
	CHECK(!Has(log, "Thiong"));
	CHECK(!Has(log, "76561198012345678"));
	CHECK(!Has(log, "83.45.12.9"));
	const std::string crashed = test::ReadFile(out + "\\logs\\imperivm-mod.crashed.log");
	CHECK(Has(crashed, "<player1> dropped, <steamid2> at <ip2>"));
	const std::string older = test::ReadFile(out + "\\logs\\imperivm-mod.1.log");
	CHECK(Has(older, "<profile1> joined, again <steamid1>"));
	CHECK(Has(test::ReadFile(out + "\\settings\\imperivm-mod.ini"), "Name=<player1>"));
	CHECK(Has(test::ReadFile(out + "\\settings\\imperivm-mod.ini"), "ApiKey=<hidden>"));
	CHECK(!Has(test::ReadFile(out + "\\settings\\imperivm-mod.ini"), "secret-value"));
	CHECK(Has(test::ReadFile(out + "\\game\\vx.log"), "crash of <player1> at <ip2>"));
	CHECK(Has(test::ReadFile(out + "\\game\\vx.log"), "Profiles/<profile1>/x.snr"));
	CHECK(Has(test::ReadFile(out + "\\game\\desync.txt"), "desync of <you>"));
	CHECK(Has(test::ReadFile(out + "\\game\\comp_out.txt"), "Now compiling 'a.vs'"));
	CHECK(Has(test::ReadFile(out + "\\rejoin\\threads.txt"), "threads of <you>"));
	const std::string info = test::ReadFile(out + "\\report-info.txt");
	CHECK(Has(info, "Mod version: 0.0.1"));
	CHECK(Has(info, "Privacy"));
	CHECK(Has(info, "logs\\imperivm-mod.log"));
	CHECK(Has(info, "was not sent anywhere"));
	test::RemoveTree(out);
}

void Pruning(const std::string &dir)
{
	const std::string game = dir + "\\prune";
	test::RemoveTree(game);
	const LogPaths p = MakeLogPaths(game);
	for (int day = 1; day <= 7; day++) test::WriteFile(ReportFolder(p) + "\\imperivm-report-2020010" + std::to_string(day) + "-000000.zip", "old");
	test::WriteFile(ReportFolder(p) + "\\notes.txt", "mine");

	ReportInput input;
	input.currentLog = p.current;
	const ReportResult r = ReportCreate(p, input);
	CHECK(r.ok);
	CHECK(test::Exists(r.path));
	CHECK(test::Exists(ReportFolder(p) + "\\notes.txt"));
	int kept = 0;
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((ReportFolder(p) + "\\imperivm-report-*").c_str(), &fd);
	if (h != INVALID_HANDLE_VALUE) {
		do kept++;
		while (FindNextFileA(h, &fd));
		FindClose(h);
	}
	CHECK_EQ(kept, kReportsKept);
	CHECK(!test::Exists(ReportFolder(p) + "\\imperivm-report-20200101-000000.zip"));
	CHECK(!test::Exists(ReportFolder(p) + "\\imperivm-report-20200103-000000.zip"));
	CHECK(test::Exists(ReportFolder(p) + "\\imperivm-report-20200107-000000.zip"));
	CHECK(test::Exists(ReportFolder(p) + "\\imperivm-report-20200104-000000.zip"));
}

void NothingToReport(const std::string &dir)
{
	const std::string game = dir + "\\empty";
	test::RemoveTree(game);
	const LogPaths p = MakeLogPaths(game);
	ReportInput input;
	input.info = "Mod version: 0.0.1";
	const ReportResult r = ReportCreate(p, input);
	CHECK(r.ok);
	CHECK(test::Exists(r.path));
	CHECK_EQ(r.files.size(), static_cast<size_t>(0));
}

}

int main()
{
	const std::string dir = test::TempDir("logreport");
	Report(dir);
	Pruning(dir);
	NothingToReport(dir);
	test::RemoveTree(dir);
	return test::Summary("logreport");
}
