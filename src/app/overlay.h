// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdint>

void DrawPerfOverlay(uint32_t *buf, int w, int h, int fps, int outW, int outH);
bool OverlayNeedsRedraw();
void OverlayInstall(bool knownExe);
void OverlayNoteFrame();
bool OverlayHandleMouse(HWND hwnd, UINT msg, WPARAM wp, LPARAM &lp);
bool OverlayHandleKey(UINT msg, WPARAM wp, LPARAM lp);
