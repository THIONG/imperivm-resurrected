// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once

#include <string>
#include <vector>

bool PatchXmlText(std::string &data, const std::string &patch, std::vector<std::string> &errors);

bool PatchIniText(std::string &data, const std::string &patch, std::vector<std::string> &errors);

bool PatchScriptText(std::string &data, const std::string &patch, std::vector<std::string> &errors);
