// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include "mods/modoptions.h"

static ModOption Make(const std::string &id, size_t values)
{
	ModOption o;
	o.id = id;
	for (size_t i = 0; i < values; i++) o.values.push_back("v" + std::to_string(i));
	o.width = ModOptionWidth(values);
	return o;
}

static void Width()
{
	CHECK_EQ(ModOptionWidth(0), 1u);
	CHECK_EQ(ModOptionWidth(2), 1u);
	CHECK_EQ(ModOptionWidth(3), 2u);
	CHECK_EQ(ModOptionWidth(4), 2u);
	CHECK_EQ(ModOptionWidth(5), 3u);
	CHECK_EQ(ModOptionWidth(8), 3u);
	CHECK_EQ(ModOptionWidth(9), 4u);
	CHECK_EQ(ModOptionWidth(16), 4u);
}

static void Placement()
{
	int used = 0;
	ModOption a = Make("a", 0), b = Make("b", 4), c = Make("c", 3);
	CHECK(ModOptionPlace(a, used));
	CHECK_EQ(a.pos, 0);
	CHECK_EQ(a.bit, 0x80000u);
	CHECK_EQ(used, 1);
	CHECK(ModOptionPlace(b, used));
	CHECK_EQ(b.pos, 1);
	CHECK_EQ(b.bit, 0x300000u);
	CHECK_EQ(used, 3);
	CHECK(ModOptionPlace(c, used));
	CHECK_EQ(c.pos, 3);
	CHECK_EQ(c.bit, 0xC00000u);
	CHECK_EQ(used, 5);
	CHECK_EQ(a.bit & b.bit, 0u);
	CHECK_EQ(b.bit & c.bit, 0u);
	CHECK_EQ(a.bit & c.bit, 0u);
}

static void Capacity()
{
	int used = 0;
	ModOption wide = Make("wide", 16), more = Make("more", 16), flag = Make("flag", 0);
	CHECK(ModOptionPlace(wide, used));
	CHECK_EQ(used, 4);
	CHECK(ModOptionPlace(more, used));
	CHECK_EQ(used, 8);
	const unsigned before = flag.bit;
	CHECK(!ModOptionPlace(flag, used));
	CHECK_EQ(used, 8);
	CHECK_EQ(flag.bit, before);
	CHECK_EQ(kModOptionBits, 8);
}

static void GetSet()
{
	int used = 0;
	ModOption flag = Make("flag", 0), mode = Make("mode", 4), level = Make("level", 3);
	ModOptionPlace(flag, used);
	ModOptionPlace(mode, used);
	ModOptionPlace(level, used);

	unsigned rules = 0;
	rules = ModOptionSet(flag, rules, 1);
	CHECK_EQ(ModOptionGet(flag, rules), 1);
	CHECK_EQ(ModOptionGet(mode, rules), 0);
	CHECK_EQ(rules, flag.bit);

	rules = ModOptionSet(mode, rules, 3);
	CHECK_EQ(ModOptionGet(mode, rules), 3);
	CHECK_EQ(ModOptionGet(flag, rules), 1);
	CHECK_EQ(ModOptionGet(level, rules), 0);

	rules = ModOptionSet(level, rules, 2);
	CHECK_EQ(ModOptionGet(level, rules), 2);
	CHECK_EQ(ModOptionGet(mode, rules), 3);

	rules = ModOptionSet(mode, rules, 1);
	CHECK_EQ(ModOptionGet(mode, rules), 1);
	CHECK_EQ(ModOptionGet(level, rules), 2);
	CHECK_EQ(ModOptionGet(flag, rules), 1);

	rules = ModOptionSet(flag, rules, 0);
	CHECK_EQ(ModOptionGet(flag, rules), 0);
	CHECK_EQ(ModOptionGet(mode, rules), 1);
}

static void Clamp()
{
	int used = 0;
	ModOption flag = Make("flag", 0), level = Make("level", 3);
	ModOptionPlace(flag, used);
	ModOptionPlace(level, used);
	CHECK_EQ(ModOptionGet(flag, ModOptionSet(flag, 0, 5)), 1);
	CHECK_EQ(ModOptionGet(flag, ModOptionSet(flag, 0, -3)), 0);
	CHECK_EQ(ModOptionGet(level, ModOptionSet(level, 0, 7)), 2);
	CHECK_EQ(ModOptionGet(level, ModOptionSet(level, 0, -1)), 0);
	CHECK_EQ((ModOptionSet(level, 0, 7) & ~(level.bit | flag.bit)), 0u);
}

static void OtherBitsKept()
{
	int used = 0;
	ModOption flag = Make("flag", 0);
	ModOptionPlace(flag, used);
	const unsigned others = 0x7FFFF | 0xF0000000u;
	CHECK_EQ(ModOptionSet(flag, others, 1), others | flag.bit);
	CHECK_EQ(ModOptionSet(flag, others | flag.bit, 0), others);
}

int main()
{
	Width();
	Placement();
	Capacity();
	GetSet();
	Clamp();
	OtherBitsKept();
	return test::Summary("options");
}
