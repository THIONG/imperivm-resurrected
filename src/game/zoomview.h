// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <windows.h>

void InstallZoomView(bool knownExe);
bool ZoomViewHandleMouse(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
