// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

void ModUiOpen();
void ModUiAskRestart(const std::string &reason = "", int largeAddress = -1, const std::vector<std::string> &items = {},
	const std::string &itemsTitle = "");
bool ModUiIsOpen();
int ModUiUpdatesAvailable();
void ModUiToast(const std::string &text, bool error = false);

bool ModUiHandleMessage(UINT msg, WPARAM wp, LPARAM lp);
bool ModUiHandleCornerMessage(UINT msg, WPARAM wp, LPARAM lp);

bool ModUiNeedsDraw();
void ModUiDraw(uint32_t *buf, int w, int h);

void ModUiDrawHoverCard(uint32_t *buf, int w, int h, const std::string &text, COLORREF accent, int atX, int centerY);
