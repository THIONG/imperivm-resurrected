// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "core/log.h"

void LogInit() {}
void LogShutdown() {}
void Log(const char *, ...) {}
void LogAt(LogLevel, const char *, const char *, ...) {}
void LogT(const char *, const char *, ...) {}
void LogD(const char *, const char *, ...) {}
void LogI(const char *, const char *, ...) {}
void LogW(const char *, const char *, ...) {}
void LogE(const char *, const char *, ...) {}
void LogF(const char *, const char *, ...) {}
void LogEvery(LogLevel, const char *, unsigned, const char *, ...) {}
void LogOnce(LogLevel, const char *, const char *, ...) {}
bool LogEnabled(LogLevel, const char *) { return false; }
void LogDumpContext(const char *) {}
const char *LogGameDir() { return ""; }
const char *LogFilePath() { return ""; }
const char *LogPreviousFilePath() { return ""; }
