// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <map>
#include <string>
#include <vector>

class LogRedactor {
public:
	void AddName(const std::string &name, const std::string &label = std::string());
	void AddUserName(const std::string &name);
	std::string Apply(const std::string &text);
	size_t Replacements() const { return replacements_; }

private:
	struct Known {
		std::string lower;
		std::string label;
	};
	std::vector<Known> names_;
	std::map<std::string, int> steamIds_;
	std::map<std::string, int> addresses_;
	std::string userPath_;
	int unnamed_ = 0;
	size_t replacements_ = 0;
};

std::string RedactIniSecrets(const std::string &ini);
