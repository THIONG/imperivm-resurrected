// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>
#include <mutex>
#include "core/guard.h"

#include "ui/modui_internal.h"

using namespace modui_impl;

namespace modui_impl {
void DoAction(int action, int arg)
{
	switch (action) {
	case kDropOpen:
		if (arg == kDropVersions && g_s.drop != kDropVersions) {
			g_s.verHas = false;
			if (g_s.tab == 2) {
				if (g_s.ioDetail >= 0 && g_s.ioDetail < static_cast<int>(g_s.io.mods.size())) {
					g_s.verMod = g_s.io.mods[g_s.ioDetail];
					g_s.verHas = true;
				}
			}
			else if (g_s.selected >= 0 && g_s.selected < static_cast<int>(g_s.rows.size())) {
				const ModInfo *m = FindMod(g_s.rows[g_s.selected].folder);
				const std::shared_ptr<const GbMod> io = m && m->gbId ? GbInfo(m->gbId) : nullptr;
				if (io) {
					g_s.verMod = *io;
					g_s.verHas = true;
				}
			}
			if (g_s.verHas) GbGetFiles(g_s.verMod.id);
		}
		g_s.drop = g_s.drop == arg ? kDropNone : arg;
		g_s.dropScroll = std::max(0, DropCurrent(g_s) - kMenuMaxRows / 2);
		break;
	case kDropClose: g_s.drop = kDropNone; break;
	case kDropPick: PickDrop(arg); break;
	case kSaveAs: StartName(kNameSaveAs); break;
	case kFocusName: FocusField(kNameField); break;
	case kNameOk: FinishName(); break;
	case kNameCancel:
		g_s.nameMode = kNameNone;
		g_s.focus = kNoField;
		break;
	case kFocusSearch: FocusField(kSearchField); break;
	case kDelProfile:
		if (g_s.profile.name == kOriginalName || g_s.profile.name == kNoProfileName) {
			SetStatus("That profile cannot be deleted");
		}
		else {
			const std::string old = g_s.profile.name;
			ModsDeleteProfile(old);
			SwitchProfile(kOriginalName);
			SetStatus(Tr("Profile \"") + old + Tr("\" deleted"));
		}
		break;
	case kSelect: g_s.selected = arg; break;
	case kToggle: {
		g_s.selected = arg;
		if (arg < 0 || arg >= static_cast<int>(g_s.rows.size())) break;
		const Row r = g_s.rows[arg];
		if (Pending(g_s, r.folder)) break;
		auto &mods = g_s.profile.mods;
		if (r.active) {
			mods.erase(std::remove(mods.begin(), mods.end(), r.folder), mods.end());
		}
		else {
			mods.push_back(r.folder);
		}
		CommitProfile();
		SelectFolder(r.folder);
		if (!r.active) {
			const ModIssues &is = g_s.issues[r.folder];
			const ModInfo *m = FindMod(r.folder);
			const std::string name = m ? m->name : r.folder;
			if (!is.problems.empty()) {
				SetStatus(name + ": " + is.problems[0]);
			}
			else if (is.overwrites > 0 && !is.overlapFolders.empty()) {
				SetStatus(OverlapText(g_s, r.folder, is));
			}
		}
		break;
	}
	case kUninstall:
		if (g_s.selected >= 0 && g_s.selected < static_cast<int>(g_s.rows.size())) {
			g_s.confirm = true;
			g_s.confirmKind = kConfirmUninstall;
			g_s.confirmItems.clear();
			g_s.confirmItemsTitle = "It will be removed from these profiles:";
			g_s.confirmList = ListState();
			const auto used = g_s.usedBy.find(g_s.rows[g_s.selected].folder);
			if (used != g_s.usedBy.end()) {
				for (const std::string &n : used->second) g_s.confirmItems.push_back(ModsProfileLabel(n));
			}
		}
		break;
	case kOpenMods: OpenFolderAsync(""); break;
	case kOpenMod:
		if (g_s.selected >= 0 && g_s.selected < static_cast<int>(g_s.rows.size())) {
			OpenFolderAsync(g_s.rows[g_s.selected].folder);
		}
		break;
	case kRescan:
		ModsScan();
		DlRefreshCatalogs();
		Reload();
		SetStatus(Tr("List updated: ") + std::to_string(g_s.mods.size()) + (g_s.mods.size() == 1 ? Tr(" content pack") : Tr(" content packs")));
		break;
	case kApply:
		if (ModsRestartNeeded()) {
			g_s.confirm = true;
			g_s.confirmKind = kConfirmRestart;
		}
		else {
			SetStatus("No changes to apply: the active profile is already loaded");
		}
		break;
	case kClose:
		g_s.open = false;
		break;
	case kConfirmYes:
		if (g_s.confirmKind == kConfirmUninstall) {
			g_s.confirm = false, g_s.confirmItems.clear();
			if (g_s.selected >= 0 && g_s.selected < static_cast<int>(g_s.rows.size())) {
				const std::string folder = g_s.rows[g_s.selected].folder;
				g_s.busy = true;
				SafeThread([folder] {
					const std::string msg = ModsUninstall(folder);
					std::lock_guard<std::mutex> lock(g_lock);
					Reload();
					g_s.busy = false;
					SetStatus(msg);
				}).detach();
			}
			break;
		}
		if (g_s.confirmKind == kConfirmDelete) {
			g_s.confirm = false, g_s.confirmItems.clear();
			DoAction(kDelProfile, 0);
			break;
		}
		LogI("content", "mod manager: restart to apply profile \"%s\"", g_s.profile.name.c_str());
		ModsRestartGame(g_s.restartLargeAddress);
		break;
	case kConfirmNo:
		g_s.confirm = false, g_s.confirmItems.clear();
		if (g_s.confirmKind == kConfirmRestart && g_s.confirmOnly) {
			g_s.open = false;
		}
		g_s.confirmOnly = false;
		break;
	case kIoFilterOpen: g_s.ioFilterOpen = !g_s.ioFilterOpen; break;
	case kIoFilterClose: g_s.ioFilterOpen = false; break;
	case kIoFilterToggle: {
		const int group = arg / kIoFilterGroupStride, bit = arg % kIoFilterGroupStride;
		if (arg < 0 || group > kIoFilterLanguages || bit >= 32) break;
		uint32_t &mask = group == kIoFilterTypes ? g_s.ioFilter.types : group == kIoFilterPlayers ? g_s.ioFilter.players : g_s.ioFilter.languages;
		mask ^= 1u << bit;
		g_s.ioFilter = CatalogNormalize(g_s.ioFilter);
		g_s.ioPage = 0;
		IoSearch();
		break;
	}
	case kIoFilterClear:
		g_s.ioFilter = CatalogFilter();
		g_s.ioPage = 0;
		IoSearch();
		break;
	case kTabInstalled:
		g_s.tab = 0;
		g_s.ioFilterOpen = false;
		break;
	case kTabExplore:
		if (g_s.tab != 2) {
			g_s.tab = 2;
			IoSearch();
		}
		g_s.tab = 2;
		break;
	case kIoFocusSearch: FocusField(kIoSearchField); break;
	case kIoRetry:
		g_s.ioPage = 0;
		IoSearch();
		break;
	case kIoPrev:
		if (g_s.ioPage > 0) {
			g_s.ioPage--;
			IoSearch();
		}
		break;
	case kIoNext:
		if ((g_s.ioPage + 1) * kIoPerPage < g_s.io.total) {
			g_s.ioPage++;
			IoSearch();
		}
		break;
	case kIoOpen:
		g_s.ioDetail = arg;
		g_s.ioShot = -1;
		break;
	case kIoBack:
		g_s.ioDetail = -1;
		g_s.ioShot = -1;
		break;
	case kIoShotOpen: g_s.ioShot = arg; break;
	case kIoShotClose: g_s.ioShot = -1; break;
	case kIoShotStep:
		if (g_s.ioDetail >= 0 && g_s.ioDetail < static_cast<int>(g_s.io.mods.size()) && g_s.ioShot >= 0) {
			const int n = static_cast<int>(g_s.io.mods[g_s.ioDetail].shots.size());
			if (n > 0) g_s.ioShot = ((g_s.ioShot + arg) % n + n) % n;
		}
		break;
	case kIoInstall:
		if (arg >= 0 && arg < static_cast<int>(g_s.io.mods.size())) {
			const GbMod &m = g_s.io.mods[arg];
			GbInstall(m);
		}
		break;
	case kOpenSource:
		if (g_s.selected >= 0 && g_s.selected < static_cast<int>(g_s.rows.size())) {
			const ModInfo *m = FindMod(g_s.rows[g_s.selected].folder);
			const std::shared_ptr<const GbMod> io = m && m->gbId ? GbInfo(m->gbId) : nullptr;
			if (io) OpenWeb(io->page);
		}
		break;
	case kIoPage:
		if (arg >= 0 && arg < static_cast<int>(g_s.io.mods.size())) OpenWeb(g_s.io.mods[arg].page);
		break;
	case kRichLink: {
		std::string link;
		{
			std::lock_guard<std::mutex> lock(g_rich.mx);
			if (arg >= 0 && arg < static_cast<int>(g_rich.blocks.size())) link = g_rich.blocks[arg].link;
		}
		if (!link.empty()) OpenWeb(link);
		break;
	}
	case kShowProblems: {
		const std::vector<std::string> problems = ModsProblems(g_s.profile);
		std::string text;
		for (size_t i = 0; i < problems.size() && i < 5; i++) text += (i ? "\n" : "") + problems[i];
		if (problems.size() > 5) text += "\n(" + Tr("and ") + std::to_string(problems.size() - 5) + Tr(" more)");
		if (!text.empty()) {
			g_s.toast = text;
			g_s.toastErr = true;
			g_s.toastLen = 12000;
			g_s.toastUntil = GetTickCount() + g_s.toastLen;
		}
		break;
	}
	case kUpdateAll: {
		std::vector<const ModInfo *> targets;
		std::vector<CatalogEntry> entries;
		for (const ModInfo &m : g_s.mods) {
			CatalogEntry e;
			if (!FindUpdateFor(g_s, m, e)) continue;
			bool merged = false;
			for (size_t k = 0; k < targets.size(); k++) {
				if (!ModsSameMod(*targets[k], m)) continue;
				merged = true;
				if (ModsCompareVersions(m.version, targets[k]->version) > 0) targets[k] = &m;
			}
			if (!merged) {
				targets.push_back(&m);
				entries.push_back(e);
			}
		}
		for (size_t k = 0; k < targets.size(); k++) DlUpdateMod(targets[k]->folder, entries[k]);
		if (targets.empty()) SetStatus("No pending updates");
		break;
	}
	case kUpdateMod: {
		if (g_s.selected < 0 || g_s.selected >= static_cast<int>(g_s.rows.size())) break;
		const ModInfo *m = FindMod(g_s.rows[g_s.selected].folder);
		CatalogEntry e;
		if (m && FindUpdateFor(g_s, *m, e)) DlUpdateMod(m->folder, e);
		break;
	}
	}
	const bool keepsFocus =
		action == kFocusName || action == kFocusSearch || action == kIoFocusSearch || action == kNone || action == kSaveAs ||
		(action == kNameOk && g_s.nameMode != kNameNone) ||
		(action == kDropPick && g_s.nameMode != kNameNone);
	if (!keepsFocus) {
		g_s.focus = kNoField;
	}
	if (action != kDropOpen && action != kNone) {
		if (action != kDropPick) g_s.drop = kDropNone;
	}
	const int maxScroll = std::max(0, static_cast<int>(VisibleRows(g_s).size()) - kInstalledListH / kRowH);
	g_s.list.scroll = std::max(0, std::min(g_s.list.scroll, maxScroll));
	if (action != kNone && action != kSelect) {
		g_s.restartNeeded = ModsRestartNeeded();
	}
}
}
