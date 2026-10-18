// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>
#include <vector>
void MediaInstall(bool knownExe);
void MediaSetModDirs(const std::vector<std::string> &dirs);
void MediaFrameTick();
bool MediaMovieShowing();
#include <cstdint>
#include <vector>
bool MediaVideoActive();
bool MediaTakeVideoFrame(std::vector<uint32_t> &out, int &w, int &h);
