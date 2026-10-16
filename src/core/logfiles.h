// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>

struct LogPaths {
	std::string gameDir;
	std::string logsDir;
	std::string modDir;
	std::string current;
	std::string crashed;
	std::string lock;

	std::string Numbered(int n) const;
	std::string ForProcess(unsigned pid) const;
};

struct LogFilesState {
	bool previousUnclean = false;
	bool otherInstance = false;
	std::string previousLog;
	unsigned long long previousStart = 0;
	std::string gameCrashReport;
};

const int kLogFilesKept = 5;
const unsigned long long kLogFolderMaxBytes = 100ull << 20;

LogPaths MakeLogPaths(const std::string &gameDir);
LogFilesState PrepareLogFiles(const LogPaths &paths, int keep = kLogFilesKept);
bool WriteSessionLock(const LogPaths &paths);
void RemoveSessionLock(const LogPaths &paths);
std::string NewestGameCrashFolder(const LogPaths &paths, unsigned long long sinceFileTime);
void PruneLogFolder(const LogPaths &paths, unsigned long long maxBytes, int keep = kLogFilesKept);
void RemoveOldRejoinTraces(const LogPaths &paths);
