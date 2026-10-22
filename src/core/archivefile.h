// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>

#include "core/zipfile.h"

bool IsArchiveFile(const std::string &path);
std::string ArchiveExtract(const std::string &archive, const std::string &dest, const ZipLimits &limits, const ZipProgress &progress = nullptr);
bool ArchiveNotUnderstood(const std::string &error);
