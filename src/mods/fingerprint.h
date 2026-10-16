// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <string>
#include <vector>

namespace mods_impl {

struct FingerprintFile {
	std::string rel;
	std::string sha;
	bool gameplay = false;
};

struct Fingerprint {
	std::string all;
	std::string gameplay;
};

bool FingerprintIgnores(const std::string &rel);

Fingerprint ComputeFingerprint(std::vector<FingerprintFile> files);

}
