// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>
#include <vector>

struct RichBlock {
	int kind = 0;
	std::string text;
	std::string marker;
	std::string link;
	int indent = 0;
	bool bold = false;
};

std::vector<RichBlock> RichFromHtml(const std::string &html);
std::vector<RichBlock> RichFromPlain(const std::string &text);
