// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>
#include <algorithm>
#include "MinHook.h"
#include "i18n/tr.h"

#include "mods/mods_internal.h"

using namespace mods_impl;

std::string ModsActiveProfileName()
{
	char buf[256] = {};
	GetPrivateProfileStringA("Mods", "Profile", kOriginal, buf, sizeof(buf), IniPath().c_str());
	return buf[0] ? buf : kOriginal;
}

void ModsSetActiveProfileName(const std::string &name)
{
	WritePrivateProfileStringA("Mods", "Profile", name.c_str(), IniPath().c_str());
	const ModProfile p = ModsLoadProfile(name);
	std::lock_guard<std::mutex> lock(g_lock);
	if (g_mount.known && g_mount.profile.name != p.name && ModsSameSet(p.mods, g_mount.profile.mods)) g_mount.profile.name = p.name;
}

bool ModsScenarioProvider(const std::string &scenarioPath, std::string &id, std::string &version, std::string &name)
{
	const std::string want = NormPath(scenarioPath) + ".bfhp";
	std::lock_guard<std::mutex> lock(g_lock);
	for (const std::string &folder : g_mount.profile.mods) {
		for (const ModInfo &m : g_mods) {
			if (m.folder != folder || std::find(m.paths.begin(), m.paths.end(), want) == m.paths.end()) continue;
			id = m.id;
			version = m.version;
			name = m.name;
			return true;
		}
	}
	return false;
}

bool ModsMountedHasExact(const std::string &id, const std::string &version)
{
	std::lock_guard<std::mutex> lock(g_lock);
	for (const std::string &folder : g_mount.profile.mods) {
		for (const ModInfo &m : g_mods) {
			if (m.folder == folder && _stricmp(m.id.c_str(), id.c_str()) == 0 && m.version == version) return true;
		}
	}
	return false;
}

const ModProfile &ModsMountedProfile()
{
	static ModProfile original;
	if (original.name.empty()) {
		original.name = kOriginal;
	}
	std::lock_guard<std::mutex> lock(g_lock);
	return g_mount.known ? g_mount.profile : original;
}

bool ModsRestartNeeded()
{
	const ModProfile active = ModsLoadProfile(ModsActiveProfileName());
	const ModProfile &mounted = ModsMountedProfile();
	return !ModsSameSet(active.mods, mounted.mods);
}

bool ModsSameSet(std::vector<std::string> a, std::vector<std::string> b)
{
	for (std::string &s : a) s = Lower(s);
	for (std::string &s : b) s = Lower(s);
	std::sort(a.begin(), a.end());
	std::sort(b.begin(), b.end());
	return a == b;
}

std::vector<ModConflict> ModsConflicts(const ModProfile &p)
{
	std::map<std::string, std::vector<std::string>> owners;
	std::lock_guard<std::mutex> lock(g_lock);
	for (const std::string &folder : p.mods) {
		for (const ModInfo &m : g_mods) {
			if (m.folder == folder) {
				for (const std::string &path : m.paths) {
					owners[path].push_back(folder);
				}
			}
		}
	}
	std::vector<ModConflict> out;
	for (auto &o : owners) {
		if (o.second.size() > 1) {
			out.push_back({o.first, o.second});
		}
	}
	return out;
}

std::vector<std::string> ModsProblems(const ModProfile &p)
{
	std::vector<std::string> out;
	for (const std::string &folder : p.mods) {
		ModInfo m;
		if (!ModsFind(folder, m)) {
			out.push_back(Tr("Missing content pack \"") + folder + Tr("\" (not in the content folder)"));
			continue;
		}
		for (const std::string &problem : ModsIssuesFor(p, folder).problems) out.push_back(m.name + ": " + problem);
	}
	return out;
}

std::string ModsProfileGameplayHash(const ModProfile &p)
{
	std::vector<RulesPart> parts;
	std::lock_guard<std::mutex> lock(g_lock);
	for (const std::string &folder : p.mods) {
		for (const ModInfo &m : g_mods) {
			if (m.folder == folder && m.gameplay) {
				parts.push_back({m.id, m.gameplayHash});
			}
		}
	}
	return RulesFingerprint(parts);
}
