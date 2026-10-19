// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>

void InstallUpdater();
int UpdaterModeGet();
void UpdaterModeSet(int mode);
void UpdaterMenuReached();

struct UpdaterInfo {
	enum State { kNone, kAvailable, kDownloading, kReady, kFailed } state = kNone;
	std::string current, newVersion, notes, error;
	int percent = 0;
};
UpdaterInfo UpdaterGetInfo();
void UpdaterStartInstall();
void UpdaterRestartNow();
