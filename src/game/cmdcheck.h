// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>
#include <vector>

struct GameCommand {
	std::string name, method, param;
};

std::string GameStringAt(const void *at);
std::vector<GameCommand> GameCommands();
bool CanExecCommand(unsigned handle, const char *command);
bool OffersCommand(unsigned handle, const char *command);
