// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>

struct TransferRate {
	static const uint32_t kWindowMs = 700;

	uint64_t lastBytes = 0;
	uint32_t lastTick = 0;
	bool started = false;
	double perSecond = 0;

	void Reset() { *this = TransferRate(); }

	void Sample(uint64_t bytes, uint32_t nowMs)
	{
		if (!started || bytes < lastBytes) {
			if (started) perSecond = 0;
			started = true;
			lastBytes = bytes;
			lastTick = nowMs;
			return;
		}
		const uint32_t elapsed = nowMs - lastTick;
		if (elapsed < kWindowMs) return;
		const double now = static_cast<double>(bytes - lastBytes) * 1000.0 / elapsed;
		perSecond = perSecond <= 0 ? now : perSecond * 0.6 + now * 0.4;
		lastBytes = bytes;
		lastTick = nowMs;
	}
};
