// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once

#include <string>
#include <vector>

#include "mods/patchxml.h"

void PatchRebuild();

bool PatchHas(const std::string &rel);

bool PatchApply(const std::string &rel, std::string &data);

void PatchMissingTarget(const std::string &rel);

std::vector<std::string> PatchErrorsFor(const std::string &folder);

std::vector<std::vector<PatchedValue>> PatchResolve(const std::string &rel, const std::vector<std::string> &patches);

void PatchReadBases(const std::vector<std::string> &rels);
