// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <atomic>
#include <exception>
#include <thread>
#include <utility>

#include "core/log.h"

inline void GuardReport(const char *where, const char *what)
{
	static std::atomic<int> reported{0};
	if (reported.fetch_add(1) < 60) LogE("crash", "EXCEPTION in %s: %s", where, what);
}

template <class F> bool Guard(const char *where, F &&f)
{
	try {
		f();
		return true;
	}
	catch (const std::exception &e) {
		GuardReport(where, e.what());
	}
	catch (...) {
		GuardReport(where, "unknown exception");
	}
	return false;
}

template <class F, class... A> std::thread SafeThread(F f, A... args)
{
	return std::thread([f, args...]() mutable {
		Guard("background thread", [&] { f(args...); });
	});
}
