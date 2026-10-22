// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>

#include "core/zipfile.h"

bool IsRarFile(const std::string &path);
std::string RarExtract(const std::string &rar, const std::string &dest, const ZipLimits &limits, const ZipProgress &progress = nullptr, std::string *detail = nullptr);
