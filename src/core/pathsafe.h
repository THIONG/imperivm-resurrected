// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstring>
#include <string>
#include <vector>

namespace pathsafe {

inline bool ReservedDeviceName(const std::string &component)
{
	std::string base = component.substr(0, component.find('.'));
	while (!base.empty() && base.back() == ' ') base.pop_back();
	for (char &c : base) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
	if (base == "CON" || base == "PRN" || base == "AUX" || base == "NUL" || base == "CONIN$" || base == "CONOUT$") return true;
	return base.size() == 4 && (base.compare(0, 3, "COM") == 0 || base.compare(0, 3, "LPT") == 0) && base[3] >= '1' && base[3] <= '9';
}

inline bool SafeComponent(const std::string &name)
{
	if (name.empty() || name.size() > 200 || name == "." || name == "..") return false;
	if (name.back() == '.' || name.back() == ' ') return false;
	for (unsigned char c : name) {
		if (c < 32 || strchr("<>:\"/\\|?*", c)) return false;
	}
	return !ReservedDeviceName(name);
}

inline bool SafeFolderName(const std::string &name)
{
	return SafeComponent(name) && name[0] != '.' && name[0] != '_' && name[0] != ' ';
}

inline bool SafeRelative(const std::string &rel)
{
	if (rel.empty() || rel.size() > 400) return false;
	size_t start = 0;
	while (start <= rel.size()) {
		size_t end = rel.find_first_of("/\\", start);
		if (end == std::string::npos) end = rel.size();
		if (!SafeComponent(rel.substr(start, end - start))) return false;
		start = end + 1;
		if (end == rel.size()) break;
	}
	return true;
}

}
