// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>
#include <vector>

std::string DescribeWindows();

std::string DescribeCpu();

std::string DescribeMemory();

std::string DescribeLanguage();

std::vector<std::string> DescribeMonitors();

unsigned BuildStamp();

std::string BuildStampText(unsigned stamp);
