// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>

#include "core/logfiles.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

bool Exists(const std::string &path)
{
	return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

void Move(const std::string &from, const std::string &to)
{
	MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING);
}

void MoveIfFree(const std::string &from, const std::string &to)
{
	if (Exists(from) && !Exists(to)) MoveFileExA(from.c_str(), to.c_str(), 0);
}

unsigned long long FileTimeValue(const FILETIME &t)
{
	return (static_cast<unsigned long long>(t.dwHighDateTime) << 32) | t.dwLowDateTime;
}

bool ReadLock(const std::string &path, unsigned long &pid, unsigned long long &started)
{
	FILE *f = nullptr;
	if (fopen_s(&f, path.c_str(), "rb") != 0 || !f) return false;
	char text[128] = {};
	const size_t n = fread(text, 1, sizeof(text) - 1, f);
	fclose(f);
	text[n] = 0;
	char *end = nullptr;
	pid = strtoul(text, &end, 10);
	started = _strtoui64(end, nullptr, 10);
	return pid != 0;
}

bool LockHolderAlive(unsigned long pid, unsigned long long started)
{
	HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!process) return false;
	FILETIME created, exited, kernel, user;
	DWORD code = 0;
	const bool alive = GetProcessTimes(process, &created, &exited, &kernel, &user) && FileTimeValue(created) == started &&
		GetExitCodeProcess(process, &code) && code == STILL_ACTIVE;
	CloseHandle(process);
	return alive;
}

void MakeDirs(const std::string &path)
{
	for (size_t i = 3; i <= path.size(); i++) {
		if (i == path.size() || path[i] == '\\') CreateDirectoryA(path.substr(0, i).c_str(), nullptr);
	}
}

}

std::string LogPaths::Numbered(int n) const
{
	return modDir + "\\imperivm-mod." + std::to_string(n) + ".log";
}

std::string LogPaths::ForProcess(unsigned pid) const
{
	return modDir + "\\imperivm-mod.pid" + std::to_string(pid) + ".log";
}

LogPaths MakeLogPaths(const std::string &gameDir)
{
	LogPaths p;
	p.gameDir = gameDir;
	p.logsDir = gameDir + "\\Logs";
	p.modDir = p.logsDir + "\\imperivm";
	p.current = p.modDir + "\\imperivm-mod.log";
	p.crashed = p.modDir + "\\imperivm-mod.crashed.log";
	p.lock = p.modDir + "\\session.lock";
	return p;
}

std::string NewestGameCrashFolder(const LogPaths &p, unsigned long long sinceFileTime)
{
	std::string best;
	unsigned long long bestTime = 0;
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((p.logsDir + "\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return best;
	do {
		const std::string name = fd.cFileName;
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || name == "." || name == ".." || name == "imperivm") continue;
		WIN32_FILE_ATTRIBUTE_DATA a;
		if (!GetFileAttributesExA((p.logsDir + "\\" + name + "\\vx.log").c_str(), GetFileExInfoStandard, &a)) continue;
		const unsigned long long t = FileTimeValue(a.ftLastWriteTime);
		if (t >= sinceFileTime && t > bestTime) {
			bestTime = t;
			best = name;
		}
	} while (FindNextFileA(h, &fd));
	FindClose(h);
	return best;
}

void RemoveOldRejoinTraces(const LogPaths &p)
{
	const std::string dir = p.logsDir + "\\rejoin";
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((dir + "\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return;
	static const char *const kNames[] = {"threads.txt", "ai-history.txt", "host.txt", "return-before.txt", "return.txt", "objects-host.bin", "objects-return.bin"};
	do {
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
		const std::string name = fd.cFileName;
		bool ours = name.compare(0, 7, "return-") == 0;
		for (const char *known : kNames) ours = ours || name == known;
		if (ours) DeleteFileA((dir + "\\" + name).c_str());
	} while (FindNextFileA(h, &fd));
	FindClose(h);
	RemoveDirectoryA(dir.c_str());
}

namespace {

struct FolderFile {
	std::string path;
	unsigned long long size, time;
	bool kept;
};

void CollectFiles(const std::string &dir, std::vector<FolderFile> &out)
{
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((dir + "\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return;
	do {
		const std::string name = fd.cFileName;
		if (name == "." || name == "..") continue;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) CollectFiles(dir + "\\" + name, out);
		else out.push_back({dir + "\\" + name, (static_cast<unsigned long long>(fd.nFileSizeHigh) << 32) | fd.nFileSizeLow, FileTimeValue(fd.ftLastWriteTime), false});
	} while (FindNextFileA(h, &fd));
	FindClose(h);
}

}

void PruneLogFolder(const LogPaths &p, unsigned long long maxBytes, int keep)
{
	std::vector<FolderFile> files;
	CollectFiles(p.modDir, files);
	unsigned long long total = 0;
	for (FolderFile &f : files) {
		total += f.size;
		f.kept = f.path == p.current || f.path == p.crashed || f.path == p.lock;
		for (int n = 1; n < keep; n++) f.kept = f.kept || f.path == p.Numbered(n);
	}
	if (total <= maxBytes) return;
	std::sort(files.begin(), files.end(), [](const FolderFile &a, const FolderFile &b) { return a.time < b.time; });
	for (const FolderFile &f : files) {
		if (total <= maxBytes) break;
		if (f.kept) continue;
		if (DeleteFileA(f.path.c_str())) total -= f.size;
	}
}

LogFilesState PrepareLogFiles(const LogPaths &p, int keep)
{
	LogFilesState state;
	MakeDirs(p.logsDir);
	MakeDirs(p.modDir);

	unsigned long pid = 0;
	unsigned long long started = 0;
	if (Exists(p.lock)) {
		if (ReadLock(p.lock, pid, started) && LockHolderAlive(pid, started)) {
			state.otherInstance = true;
			return state;
		}
		state.previousUnclean = true;
		state.previousStart = started;
	}
	RemoveOldRejoinTraces(p);

	MoveIfFree(p.gameDir + "\\imperivm-mod.log", p.current);
	MoveIfFree(p.gameDir + "\\imperivm-mod.previous.log", p.Numbered(1));
	MoveIfFree(p.gameDir + "\\imperivm-mod.anterior.log", p.Numbered(2));

	if (Exists(p.current)) {
		if (state.previousUnclean) {
			Move(p.current, p.crashed);
		}
		else {
			DeleteFileA(p.Numbered(keep - 1).c_str());
			for (int n = keep - 2; n >= 1; n--) {
				if (Exists(p.Numbered(n))) Move(p.Numbered(n), p.Numbered(n + 1));
			}
			Move(p.current, p.Numbered(1));
		}
	}
	if (state.previousUnclean && Exists(p.crashed)) state.previousLog = p.crashed;
	else if (Exists(p.Numbered(1))) state.previousLog = p.Numbered(1);
	if (state.previousUnclean && state.previousStart) state.gameCrashReport = NewestGameCrashFolder(p, state.previousStart);
	PruneLogFolder(p, kLogFolderMaxBytes, keep);
	return state;
}

bool WriteSessionLock(const LogPaths &p)
{
	FILETIME created, exited, kernel, user;
	if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) return false;
	FILE *f = nullptr;
	if (fopen_s(&f, p.lock.c_str(), "wb") != 0 || !f) return false;
	fprintf(f, "%lu %llu\n", GetCurrentProcessId(), FileTimeValue(created));
	fclose(f);
	return true;
}

void RemoveSessionLock(const LogPaths &p)
{
	DeleteFileA(p.lock.c_str());
}
