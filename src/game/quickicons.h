// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once

void InstallQuickIcons(bool knownExe);
void QuickIconsGameThreadTick();
void QuickIconsDraw(void *surface, int originX, int originY);
bool QuickIconsHit(int viewX, int viewY);
bool QuickIconsMouse(int gameMessage, int viewX, int viewY);
