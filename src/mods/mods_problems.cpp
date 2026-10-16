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
	return active.name != mounted.name || active.mods != mounted.mods;
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
	std::lock_guard<std::mutex> lock(g_lock);
	auto find = [&](const std::string &key, bool byId) -> const ModInfo * {
		for (const ModInfo &m : g_mods) {
			if (_stricmp((byId ? m.id : m.folder).c_str(), key.c_str()) == 0) return &m;
		}
		return nullptr;
	};
	std::vector<const ModInfo *> active;
	for (const std::string &folder : p.mods) {
		const ModInfo *m = find(folder, false);
		if (!m) {
			out.push_back(Tr("Missing content pack \"") + folder + Tr("\" (not in the content folder)"));
			continue;
		}
		active.push_back(m);
		if (!m->forbidden.empty()) {
			out.push_back(m->name + Tr(": contains disallowed files (") + m->forbidden[0] + Tr("), it will not be loaded"));
		}
		if (!m->manifestError.empty()) {
			out.push_back(m->name + Tr(": mod.json has errors (") + m->manifestError + ")");
		}
	}
	for (size_t i = 0; i < active.size(); i++) {
		for (size_t j = i + 1; j < active.size(); j++) {
			if (_stricmp(active[i]->id.c_str(), active[j]->id.c_str()) == 0) {
				out.push_back(active[i]->name + Tr(": two versions are active (") + active[i]->version + Tr(" and ") + active[j]->version +
					Tr("); keep only one"));
			}
		}
	}
	for (const ModInfo *m : active) {
		for (const ModDependency &d : m->requires) {
			const ModInfo *dep = nullptr;
			for (const ModInfo *a : active) {
				if (_stricmp(a->id.c_str(), d.id.c_str()) == 0) dep = a;
			}
			if (!dep) {
				out.push_back(m->name + Tr(" needs \"") + d.id + "\"" + (d.version.empty() ? "" : " " + d.version) +
					(find(d.id, true) ? Tr(" (installed but not enabled)") : Tr(" (not installed)")));
			}
			else if (!VersionMatches(dep->version, d.version)) {
				out.push_back(m->name + Tr(" needs ") + d.id + " " + d.version + Tr(" (you have ") + dep->version + ")");
			}
		}
		for (const std::string &x : m->incompatible) {
			for (const ModInfo *a : active) {
				if (a != m && _stricmp(a->id.c_str(), x.c_str()) == 0) {
					out.push_back(m->name + Tr(" is not compatible with ") + a->name);
				}
			}
		}
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
