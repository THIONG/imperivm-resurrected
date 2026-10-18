// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <set>
#include <string>
#include <vector>

#include "mods/hidefiles.h"

namespace {

void Normalize()
{
	CHECK_EQ(HiddenNormalize("Data\\Classes\\Shipyard 1.sc.xml"), std::string("data/classes/shipyard 1.sc.xml"));
	CHECK_EQ(HiddenNormalize("data//classes/x.sc.xml  \r"), std::string("data/classes/x.sc.xml"));
	CHECK_EQ(HiddenNormalize("/ui/help/a.bmp"), std::string("ui/help/a.bmp"));
}

void Parse()
{
	const std::vector<std::string> p = HiddenParse("; comment\r\n# another\r\n\r\nDATA\\CLASSES\\BOILDING.SC.XML\r\n  ui/help/x.bmp\n"
												   "../escape.txt\nc:/abs.txt\nmods/other/x.xml\ndata/classes/\ndata/a/../b.xml\nlast.txt",
		"english");
	CHECK_EQ(p.size(), static_cast<size_t>(3));
	if (p.size() == 3) {
		CHECK_EQ(p[0], std::string("data/classes/boilding.sc.xml"));
		CHECK_EQ(p[1], std::string("ui/help/x.bmp"));
		CHECK_EQ(p[2], std::string("last.txt"));
	}
	CHECK(HiddenParse("", "english").empty());
	const std::string sections = "all.txt\n[Spanish]\ncurrentlang/voices/a.wav\n[italian]\ncurrentlang/voices/b.wav\n";
	const std::vector<std::string> es = HiddenParse(sections, "spanish");
	CHECK_EQ(es.size(), static_cast<size_t>(2));
	if (es.size() == 2) CHECK_EQ(es[1], std::string("currentlang/voices/a.wav"));
	const std::vector<std::string> en = HiddenParse(sections, "english");
	CHECK_EQ(en.size(), static_cast<size_t>(1));
	if (en.size() == 1) CHECK_EQ(en[0], std::string("all.txt"));
}

void FindDir()
{
	CHECK_EQ(HiddenFindDir("data/classes"), std::string("data/classes/"));
	CHECK_EQ(HiddenFindDir("Data\\Classes\\"), std::string("data/classes/"));
	CHECK_EQ(HiddenFindDir("Scenarios/*.bfhp"), std::string("scenarios/"));
	CHECK_EQ(HiddenFindDir("*.*"), std::string(""));
	CHECK_EQ(HiddenFindDir("data/classes/Winter"), std::string("data/classes/winter/"));
	CHECK_EQ(HiddenFindDir(nullptr), std::string(""));
}

void Effective()
{
	const std::vector<std::vector<std::string>> hides = {{"data/classes/a.sc.xml", "data/classes/b.sc.xml"}, {"ui/x.bmp", "data/classes/a.sc.xml"}};
	const std::set<std::string> provided = {"data/classes/b.sc.xml"};
	const std::set<std::string> e = HiddenEffective(hides, provided);
	CHECK_EQ(e.size(), static_cast<size_t>(2));
	CHECK(e.count("data/classes/a.sc.xml") == 1);
	CHECK(e.count("ui/x.bmp") == 1);
	CHECK(e.count("data/classes/b.sc.xml") == 0);
	CHECK(HiddenEffective({}, provided).empty());
}

}

int main()
{
	Normalize();
	Parse();
	FindDir();
	Effective();
	return test::Summary("hidefiles");
}
