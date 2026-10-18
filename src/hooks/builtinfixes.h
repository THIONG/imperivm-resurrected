// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once

#include <string>
#include <vector>

struct BuiltinPatch {
	std::string path;
	std::string text;
	std::string gameFileSha256;
};

void BuiltinFixesInstall(bool knownExe);

bool BuiltinFilePath(const std::string &rel, std::string &diskRel);

std::vector<BuiltinPatch> BuiltinPatches();
