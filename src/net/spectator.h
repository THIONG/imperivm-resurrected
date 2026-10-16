// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>
#include <string>

void InstallSpectator(bool knownExe);
void SpectatorGameThreadTick();

bool SpectatorWatching();
bool SpectatorViewing();
std::string SpectatorStatusText();
int SpectatorProgress();
bool SpectatorWantsWake();
int SpectatorLagMs();
bool SpectatorHostPaused();
int SpectatorDelaySec();

struct SpectatorState {
	bool paused = false, broadcast = false, atLive = false, busy = false;
	bool collapsed = false;
	bool ended = false;
	bool uiHidden = false;
	int speed = 1;
	unsigned cur = 0, total = 0;
	unsigned target = 0;
};
bool SpectatorControls(SpectatorState &out);
void SpectatorControl(int id);
void SpectatorBarPos(double &centerX, double &top, int mode);
void SpectatorSeekFraction(double fraction);
bool SpectatorHandleKey(unsigned msg, unsigned wp, long lp);

int SpectatorCount();
