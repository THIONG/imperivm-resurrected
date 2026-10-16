// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once

void InstallNetTune(bool knownExe);

void NetTuneAfterAdvance(void *transport);

struct NetTurnSecond {
	int count = 0;
	int avgMs = -1;
	int maxMs = 0;
};

NetTurnSecond NetTuneTakeSecond();
