// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>
#include <vector>

struct TechCommand {
	std::string command, method, param;
};

struct TechCondition {
	bool player = false;
	std::string var, value;
};

struct TechStep {
	std::string command;
	std::string name;
	bool playerName = false;
	std::vector<TechCondition> choices;
};

std::vector<std::string> TechParamTokens(const std::string &param);
std::vector<TechStep> TechPlan(const std::vector<TechCommand> &commands);
