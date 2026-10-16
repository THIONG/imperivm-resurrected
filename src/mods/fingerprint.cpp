// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "mods/fingerprint.h"

#include <algorithm>

#include "core/sha256.h"
#include "mods/modrules.h"

namespace mods_impl {

bool FingerprintIgnores(const std::string &rel)
{
	return rel == "mod.json" || rel == ".fingerprint" || StartsWith(rel, ".");
}

Fingerprint ComputeFingerprint(std::vector<FingerprintFile> files)
{
	std::sort(files.begin(), files.end(), [](const FingerprintFile &a, const FingerprintFile &b) { return a.rel < b.rel; });
	Sha256 all, gameplay;
	bool anyGameplay = false;
	for (const FingerprintFile &f : files) {
		if (FingerprintIgnores(f.rel)) {
			continue;
		}
		const std::string line = f.rel + "\t" + f.sha + "\n";
		all.Add(line);
		if (f.gameplay) {
			gameplay.Add(line);
			anyGameplay = true;
		}
	}
	Fingerprint out;
	out.all = all.Hex();
	out.gameplay = anyGameplay ? gameplay.Hex() : std::string();
	return out;
}

}
