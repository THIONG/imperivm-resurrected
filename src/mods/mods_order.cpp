// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>
#include <algorithm>
#include "MinHook.h"
#include "i18n/tr.h"
#include "mods/features.h"
#include "mods/patch.h"

#include "mods/mods_internal.h"

using namespace mods_impl;

namespace mods_impl {
namespace {
bool HasTag(const ModInfo &m, const char *tag)
{
	for (const std::string &t : m.tags) {
		if (_stricmp(t.c_str(), tag) == 0) return true;
	}
	return false;
}

int OrderRank(const ModInfo *m)
{
	if (!m) return 3;
	static const struct {
		const char *name;
		int rank;
	} kRanks[] = {{"Translations", 0}, {"Interface", 0}, {"Graphics", 1}, {"Audio & Voices", 1}, {"Maps", 2}, {"Scenarios", 2},
		{"Campaigns", 2}, {"Conquest", 2}, {"Units & Heroes", 3}, {"Civilizations", 3}, {"Gameplay & Balance", 3},
		{"Total Conversions", 4}};
	for (const auto &r : kRanks) {
		if (_stricmp(m->category.c_str(), r.name) == 0) return r.rank;
	}
	if (HasTag(*m, "Total conversion") || HasTag(*m, "Total Conversions")) return 4;
	if (HasTag(*m, "Interface") || HasTag(*m, "Translation") || HasTag(*m, "Translations") || HasTag(*m, "Voices")) return 0;
	if (m->kind == kModContent) return 2;
	if (m->gameplay) {
		bool onlyMaps = !m->paths.empty();
		for (const std::string &p : m->paths) {
			if (p.compare(0, 10, "scenarios/") != 0) {
				onlyMaps = false;
				break;
			}
		}
		return onlyMaps ? 2 : 3;
	}
	{
		bool onlyUi = !m->paths.empty();
		for (const std::string &p : m->paths) {
			if (p.compare(0, 6, "local/") != 0 && p.compare(0, 3, "ui/") != 0 && p.compare(0, 12, "currentlang/") != 0) {
				onlyUi = false;
				break;
			}
		}
		return onlyUi ? 0 : 1;
	}
	return 2;
}

bool SameId(const std::string &a, const std::string &b) { return !a.empty() && _stricmp(a.c_str(), b.c_str()) == 0; }

bool MustBeAbove(const ModInfo *a, const ModInfo *b)
{
	if (!a || !b || a == b) return false;
	for (const ModDependency &d : a->requires) {
		if (SameId(d.id, b->id)) return true;
	}
	for (const std::string &x : a->loadAfter) {
		if (SameId(x, b->id)) return true;
	}
	for (const std::string &x : b->loadBefore) {
		if (SameId(x, a->id)) return true;
	}
	return false;
}
}
}

std::vector<std::string> ModsAutoOrder(const std::vector<std::string> &folders)
{
	const std::vector<ModInfo> all = ModsList();
	const size_t n = folders.size();
	std::vector<const ModInfo *> info(n, nullptr);
	for (size_t i = 0; i < n; i++) {
		for (const ModInfo &m : all) {
			if (_stricmp(m.folder.c_str(), folders[i].c_str()) == 0) info[i] = &m;
		}
	}
	std::vector<size_t> base(n);
	for (size_t i = 0; i < n; i++) base[i] = i;
	std::stable_sort(base.begin(), base.end(), [&](size_t a, size_t b) {
		const int ra = OrderRank(info[a]), rb = OrderRank(info[b]);
		if (ra != rb) return ra < rb;
		const unsigned fa = info[a] ? info[a]->fileCount : 0, fb = info[b] ? info[b]->fileCount : 0;
		return fa < fb;
	});
	std::vector<size_t> basePos(n);
	for (size_t k = 0; k < n; k++) basePos[base[k]] = k;
	std::vector<std::string> out;
	std::vector<bool> placed(n, false);
	for (size_t step = 0; step < n; step++) {
		size_t best = n;
		for (size_t i = 0; i < n; i++) {
			if (placed[i]) continue;
			bool blocked = false;
			for (size_t j = 0; j < n && !blocked; j++) {
				if (!placed[j] && j != i && MustBeAbove(info[j], info[i])) blocked = true;
			}
			if (!blocked && (best == n || basePos[i] < basePos[best])) best = i;
		}
		if (best == n) {
			for (size_t k = 0; k < n; k++) {
				if (!placed[base[k]]) {
					out.push_back(folders[base[k]]);
					placed[base[k]] = true;
				}
			}
			break;
		}
		placed[best] = true;
		out.push_back(folders[best]);
	}
	return out;
}

size_t ModsInsertPosition(const std::vector<std::string> &folders, const std::string &folder)
{
	const std::vector<ModInfo> all = ModsList();
	auto find = [&](const std::string &f) -> const ModInfo * {
		for (const ModInfo &m : all) {
			if (_stricmp(m.folder.c_str(), f.c_str()) == 0) return &m;
		}
		return nullptr;
	};
	const ModInfo *added = find(folder);
	const int rank = OrderRank(added);
	size_t lo = 0, hi = folders.size(), pos = folders.size();
	for (size_t i = 0; i < folders.size(); i++) {
		const ModInfo *other = find(folders[i]);
		if (MustBeAbove(other, added)) lo = (std::max)(lo, i + 1);
		if (MustBeAbove(added, other)) hi = (std::min)(hi, i);
		if (pos == folders.size() && OrderRank(other) > rank) pos = i;
	}
	return (std::max)(lo, (std::min)(pos, (std::max)(lo, hi)));
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
