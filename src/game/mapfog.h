// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
void InstallZoomFog(bool knownExe);
long long ZoomFogGameThreadTick();
void ZoomFogNoteImage();
bool ZoomPixelToWorld(int px, int py, int *world);
bool ZoomBitmapSize(int *width, int *height);
