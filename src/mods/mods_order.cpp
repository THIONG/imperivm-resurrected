// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>
#include <algorithm>
#include "MinHook.h"
#include "i18n/tr.h"
#include "mods/features.h"
#include "mods/modorder.h"
#include "mods/patch.h"

#include "mods/mods_internal.h"

using namespace mods_impl;

namespace mods_impl {
namespace {
bool SameId(const std::string &a, const std::string &b) { return !a.empty() && _stricmp(a.c_str(), b.c_str()) == 0; }
}
}

std::vector<std::string> ModsLoadOrder(const std::vector<std::string> &folders)
{
	const std::vector<ModInfo> all = ModsList();
	std::vector<OrderItem> items;
	for (const std::string &folder : folders) {
		OrderItem item;
		item.folder = folder;
		for (const ModInfo &m : all) {
			if (_stricmp(m.folder.c_str(), folder.c_str()) != 0) continue;
			item.id = m.id;
			item.kind = m.kind;
			for (const ModDependency &d : m.requires) item.above.push_back(d.id);
			item.above.insert(item.above.end(), m.loadAfter.begin(), m.loadAfter.end());
			item.below = m.loadBefore;
			break;
		}
		items.push_back(item);
	}
	return CanonicalOrder(items);
}

ModIssues ModsIssuesFor(const ModProfile &p, const std::string &folder)
{
	ModIssues out;
	const std::vector<ModConflict> conflicts = ModsConflicts(p);
	const std::vector<ModInfo> all = ModsList();
	auto byFolder = [&](const std::string &f) -> const ModInfo * {
		for (const ModInfo &m : all) {
			if (_stricmp(m.folder.c_str(), f.c_str()) == 0) return &m;
		}
		return nullptr;
	};
	const ModInfo *m = byFolder(folder);
	if (!m) return out;
	const bool active = std::find(p.mods.begin(), p.mods.end(), folder) != p.mods.end();
	if (!m->forbidden.empty()) out.problems.push_back(Tr("Contains disallowed files (") + m->forbidden[0] + Tr("): it will not be loaded"));
	if (!m->manifestError.empty()) out.problems.push_back(Tr("mod.json has errors: ") + m->manifestError);
	for (const std::string &e : PatchErrorsFor(m->folder)) {
		if (out.problems.size() >= 8) break;
		out.problems.push_back(Tr("Patch ") + e);
	}
	for (const std::string &e : ModsFeatureProblems(m->folder)) out.problems.push_back(e);
	if (!active) return out;
	std::vector<const ModInfo *> on;
	for (const std::string &f : p.mods) {
		if (const ModInfo *a = byFolder(f)) on.push_back(a);
	}
	for (const ModDependency &d : m->requires) {
		const ModInfo *dep = nullptr;
		for (const ModInfo *a : on) {
			if (SameId(a->id, d.id)) dep = a;
		}
		bool installed = false;
		for (const ModInfo &a : all) {
			if (SameId(a.id, d.id)) installed = true;
		}
		if (!dep) out.problems.push_back(Tr("Needs \"") + d.id + "\"" + (installed ? Tr(" (enable it)") : Tr(" (it is not installed)")));
		else if (!VersionMatches(dep->version, d.version)) out.problems.push_back(Tr("Needs ") + d.id + " " + d.version + Tr(" (you have ") + dep->version + ")");
	}
	for (const ModInfo *a : on) {
		if (a == m) continue;
		if (ModsSameMod(*a, *m)) out.problems.push_back(Tr("Another version of this content pack is active (") + a->version + Tr("): keep only one"));
		for (const std::string &x : m->incompatible) {
			if (SameId(x, a->id)) out.problems.push_back(Tr("Not compatible with ") + a->name);
		}
		for (const std::string &x : a->incompatible) {
			if (SameId(x, m->id)) out.problems.push_back(a->name + Tr(" is not compatible with this content pack"));
		}
	}
	for (const ModConflict &c : conflicts) {
		if (std::find(c.folders.begin(), c.folders.end(), folder) == c.folders.end()) continue;
		out.overwrites++;
		for (const std::string &f : c.folders) {
			if (f == folder) continue;
			const ModInfo *o = byFolder(f);
			const std::string name = o ? o->name : f;
			if (std::find(out.overlapWith.begin(), out.overlapWith.end(), name) == out.overlapWith.end()) out.overlapWith.push_back(name);
			if (std::find(out.overlapFolders.begin(), out.overlapFolders.end(), f) == out.overlapFolders.end()) out.overlapFolders.push_back(f);
		}
	}
	return out;
}
