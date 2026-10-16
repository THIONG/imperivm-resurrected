// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>
#include <vector>

#include "core/logfiles.h"

struct ReportInput {
	std::string currentLog;
	std::string info;
	std::string ownName;
	std::vector<std::string> otherNames;
};

struct ReportFile {
	std::string name;
	size_t bytes = 0;
	size_t replaced = 0;
	std::string note;
};

struct ReportResult {
	bool ok = false;
	bool zipped = false;
	std::string path;
	std::string error;
	std::vector<ReportFile> files;
};

const int kReportsKept = 5;

std::string ReportFolder(const LogPaths &paths);
ReportResult ReportCreate(const LogPaths &paths, const ReportInput &input);
