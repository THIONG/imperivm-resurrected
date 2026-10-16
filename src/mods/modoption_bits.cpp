// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "mods/modoptions.h"

#include <algorithm>

unsigned ModOptionWidth(size_t values)
{
	unsigned w = 1;
	while ((static_cast<size_t>(1) << w) < values) w++;
	return w;
}

bool ModOptionPlace(ModOption &o, int &used)
{
	if (used + static_cast<int>(o.width) > kModOptionBits) return false;
	o.pos = used;
	o.bit = ((1u << o.width) - 1) * (kModOptionFirstBit << used);
	used += static_cast<int>(o.width);
	return true;
}

int ModOptionGet(const ModOption &o, unsigned rules)
{
	return static_cast<int>((rules >> (19 + o.pos)) & ((1u << o.width) - 1));
}

unsigned ModOptionSet(const ModOption &o, unsigned rules, int value)
{
	const int maxValue = o.values.empty() ? 1 : static_cast<int>(o.values.size()) - 1;
	value = (std::max)(0, (std::min)(value, maxValue));
	return (rules & ~o.bit) | (static_cast<unsigned>(value) << (19 + o.pos));
}
