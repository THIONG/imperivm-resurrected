// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once

bool InstallRenderHooks();
void ShutdownRender();
void LogOverlayStatus(const char *when);

#include <cstdint>
#include <vector>
bool RenderCopyFrame(std::vector<uint32_t> &out, int &w, int &h);
