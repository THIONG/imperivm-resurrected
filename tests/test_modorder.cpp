// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <algorithm>
#include <string>
#include <vector>

#include "mods/modorder.h"

namespace {

const int kCosmetic = 0, kContent = 1, kRules = 2;

OrderItem Item(const std::string &id, int kind, std::vector<std::string> above = {}, std::vector<std::string> below = {})
{
	OrderItem m;
	m.folder = id + "-folder";
	m.id = id;
	m.kind = kind;
	m.above = above;
	m.below = below;
	return m;
}

std::vector<std::string> Folders(const std::vector<std::string> &ids)
{
	std::vector<std::string> out;
	for (const std::string &id : ids) out.push_back(id + "-folder");
	return out;
}

void ByKind()
{
	const std::vector<OrderItem> items = {Item("rules", kRules), Item("map", kContent), Item("skin", kCosmetic)};
	CHECK_EQ(CanonicalOrder(items), Folders({"skin", "map", "rules"}));
}

void SameResultWhateverTheInput()
{
	std::vector<OrderItem> items = {Item("b-rules", kRules), Item("a-rules", kRules), Item("voices", kCosmetic), Item("c-map", kContent),
		Item("a-map", kContent)};
	const std::vector<std::string> expected = Folders({"voices", "a-map", "c-map", "a-rules", "b-rules"});
	CHECK_EQ(CanonicalOrder(items), expected);
	std::reverse(items.begin(), items.end());
	CHECK_EQ(CanonicalOrder(items), expected);
	std::rotate(items.begin(), items.begin() + 2, items.end());
	CHECK_EQ(CanonicalOrder(items), expected);
}

void IdsIgnoreCase()
{
	const std::vector<OrderItem> items = {Item("Beta", kRules), Item("alpha", kRules)};
	CHECK_EQ(CanonicalOrder(items), Folders({"alpha", "Beta"}));
}

void Dependencies()
{
	CHECK_EQ(CanonicalOrder({Item("addon", kRules, {"base"}), Item("base", kRules)}), Folders({"addon", "base"}));
	CHECK_EQ(CanonicalOrder({Item("a", kRules, {"z"}), Item("z", kRules)}), Folders({"a", "z"}));
	CHECK_EQ(CanonicalOrder({Item("a", kRules), Item("z", kRules, {"A"})}), Folders({"z", "a"}));
	CHECK_EQ(CanonicalOrder({Item("a", kRules, {}, {"z"}), Item("z", kRules)}), Folders({"z", "a"}));
	CHECK_EQ(CanonicalOrder({Item("map", kContent, {}, {"rules"}), Item("rules", kRules)}), Folders({"rules", "map"}));
	CHECK_EQ(CanonicalOrder({Item("skin", kCosmetic), Item("base", kRules, {"skin"})}), Folders({"base", "skin"}));
}

void Chains()
{
	const std::vector<OrderItem> items = {Item("c", kRules, {"b"}), Item("a", kRules), Item("b", kRules, {"a"})};
	CHECK_EQ(CanonicalOrder(items), Folders({"c", "b", "a"}));
}

void Cycles()
{
	const std::vector<OrderItem> items = {Item("x", kRules, {"y"}), Item("y", kRules, {"x"}), Item("skin", kCosmetic)};
	const std::vector<std::string> out = CanonicalOrder(items);
	CHECK_EQ(out.size(), static_cast<size_t>(3));
	CHECK_EQ(out, Folders({"skin", "x", "y"}));
}

void Missing()
{
	OrderItem gone;
	gone.folder = "Old Mod";
	const std::vector<OrderItem> items = {gone, Item("rules", kRules), Item("skin", kCosmetic)};
	CHECK_EQ(CanonicalOrder(items), (std::vector<std::string>{"skin-folder", "rules-folder", "Old Mod"}));
	CHECK(CanonicalOrder({}).empty());
}

void TwoCopiesOfOneMod()
{
	OrderItem a = Item("mod", kRules), b = Item("mod", kRules);
	a.folder = "Mod 1.1";
	b.folder = "Mod 1.0";
	CHECK_EQ(CanonicalOrder({a, b}), (std::vector<std::string>{"Mod 1.0", "Mod 1.1"}));
}

}

int main()
{
	ByKind();
	SameResultWhateverTheInput();
	IdsIgnoreCase();
	Dependencies();
	Chains();
	Cycles();
	Missing();
	TwoCopiesOfOneMod();
	return test::Summary("modorder");
}
