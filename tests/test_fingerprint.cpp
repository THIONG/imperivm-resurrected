// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <algorithm>

#include "core/sha256.h"
#include "mods/fingerprint.h"

using namespace mods_impl;

static std::vector<FingerprintFile> Sample()
{
	return {{"units/a.xml", "11", true}, {"sound/m.ogg", "22", false}, {"data/const.ini", "33", true}};
}

static void Basics()
{
	const Fingerprint f = ComputeFingerprint(Sample());
	CHECK_EQ(f.all.size(), size_t(64));
	CHECK_EQ(f.gameplay.size(), size_t(64));
	CHECK(f.all != f.gameplay);
	CHECK_EQ(f.all, HashString("data/const.ini\t33\nsound/m.ogg\t22\nunits/a.xml\t11\n"));
	CHECK_EQ(f.gameplay, HashString("data/const.ini\t33\nunits/a.xml\t11\n"));
}

static void Order()
{
	std::vector<FingerprintFile> reversed = Sample();
	std::reverse(reversed.begin(), reversed.end());
	const Fingerprint a = ComputeFingerprint(Sample()), b = ComputeFingerprint(reversed);
	CHECK_EQ(a.all, b.all);
	CHECK_EQ(a.gameplay, b.gameplay);
}

static void Sensitivity()
{
	const Fingerprint base = ComputeFingerprint(Sample());
	std::vector<FingerprintFile> changed = Sample();
	changed[1].sha = "23";
	const Fingerprint cosmetic = ComputeFingerprint(changed);
	CHECK(cosmetic.all != base.all);
	CHECK_EQ(cosmetic.gameplay, base.gameplay);
	changed = Sample();
	changed[0].sha = "12";
	const Fingerprint rules = ComputeFingerprint(changed);
	CHECK(rules.all != base.all);
	CHECK(rules.gameplay != base.gameplay);
	changed = Sample();
	changed[1].rel = "sound/n.ogg";
	CHECK(ComputeFingerprint(changed).all != base.all);
}

static void Ignored()
{
	CHECK(FingerprintIgnores("mod.json"));
	CHECK(FingerprintIgnores(".fingerprint"));
	CHECK(FingerprintIgnores(".hidden/file.txt"));
	CHECK(!FingerprintIgnores("units/mod.json"));
	CHECK(!FingerprintIgnores("data/.keep"));
	const Fingerprint base = ComputeFingerprint(Sample());
	std::vector<FingerprintFile> extra = Sample();
	extra.push_back({"mod.json", "99", true});
	extra.push_back({".fingerprint", "98", false});
	extra.push_back({".git/config", "97", true});
	const Fingerprint f = ComputeFingerprint(extra);
	CHECK_EQ(f.all, base.all);
	CHECK_EQ(f.gameplay, base.gameplay);
}

static void NoRules()
{
	const Fingerprint f = ComputeFingerprint({{"sound/m.ogg", "22", false}});
	CHECK_EQ(f.all, HashString("sound/m.ogg\t22\n"));
	CHECK_EQ(f.gameplay, "");
	const Fingerprint none = ComputeFingerprint({});
	CHECK_EQ(none.all, HashString(""));
	CHECK_EQ(none.gameplay, "");
}

int main()
{
	Basics();
	Order();
	Sensitivity();
	Ignored();
	NoRules();
	return test::Summary("fingerprint");
}
