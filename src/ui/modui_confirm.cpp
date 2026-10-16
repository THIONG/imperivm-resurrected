// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>
#include "addr/state.h"

#include "ui/modui_internal.h"

using namespace modui_impl;

namespace modui_impl {
namespace {
std::vector<ListLine> ConfirmLines(const State &st)
{
	std::vector<ListLine> lines;
	if (st.confirmItems.empty()) return lines;
	if (!st.confirmItemsTitle.empty()) lines.push_back({Tr(st.confirmItemsTitle), kGold, true});
	for (const std::string &item : st.confirmItems) {
		if (item.compare(0, 2, "##") == 0) lines.push_back({Tr(item.substr(2)), kGold, true});
		else lines.push_back({"•  " + item, kText, false});
	}
	return lines;
}
}
}

namespace modui_impl {
void ConfirmSize(const State &st, int &cw, int &ch)
{
	cw = kConfirmW;
	ch = kConfirmH;
	if (st.confirmItems.empty()) return;
	const int rows = std::min<int>(kConfirmListRows, static_cast<int>(ConfirmLines(st).size()));
	cw = 600;
	ch = kConfirmH + rows * kConfirmRowH + 8;
}

void PaintConfirm(Painter &p, const State &st)
{
	const int mx = st.mouseX, my = st.mouseY;
	g_confirmGeomOut = ListGeom();
	if (!st.confirm) return;
	int cw, ch;
	ConfirmSize(st, cw, ch);
	std::string title, text, yes;
	if (st.confirmKind == kConfirmUninstall) {
		std::string folder = st.selected >= 0 && st.selected < static_cast<int>(st.rows.size()) ? st.rows[st.selected].folder : "";
		const ModInfo *m = FindModIn(st.mods, folder);
		const std::string name = m ? m->name : folder;
		bool mounted = false;
		for (const std::string &f : ModsMountedProfile().mods) {
			if (f == folder) mounted = true;
		}
		title = Tr("Uninstall ") + name;
		text = Tr("Its folder will be deleted from disk (no recycle bin)") + std::string(mounted ? " when the game restarts (it is currently loaded)." : ".");
		yes = "Uninstall";
	}
	else if (st.confirmKind == kConfirmConflict) {
		const ModInfo *m = FindModIn(st.mods, st.conflictFolder);
		std::string with;
		for (const std::string &f : st.conflictWith) {
			const ModInfo *o = FindModIn(st.mods, f);
			with += (with.empty() ? "" : ", ") + (o ? o->name : f);
		}
		title = "Which one wins?";
		text = Tr("You enabled ") + (m ? m->name : st.conflictFolder) + Tr(", which includes ") + std::to_string(st.conflictFiles) +
			Tr(st.confirmItems.empty() ? " files also included in " : " files that these content packs also contain:") + (st.confirmItems.empty() ? with : std::string()) +
			Tr(". Only one version of each is used: the one from the content higher in the list. You can change it later by dragging the content.");
		yes = "New one wins";
	}
	else if (st.confirmKind == kConfirmDelete) {
		title = "Delete profile";
		text = Tr("Delete the profile \"") + st.profile.name + Tr("\"? Content is not uninstalled, only the profile's list.");
		yes = "Remove";
	}
	else {
		title = "Restart the game";
		const bool inMatch = *reinterpret_cast<void **>(addr::kGame) != nullptr;
		std::string why;
		if (!st.restartReason.empty()) why = Tr(st.restartReason);
		else if (st.profile.name == "Original") why = Tr("The game will restart without additional content.");
		else if (st.profile.name == "No profile") why = Tr("The game will restart to apply the content changes.");
		else why = Tr("The game will restart to apply the profile ") + "\"" + st.profile.name + "\".";
		text = why + (inMatch ? Tr(" Anything you have not saved will be lost.") : std::string());
		yes = "Restart now";
	}
	const Dialog d = DialogBegin(p, st.confirmOnly ? cw : kDesignW, st.confirmOnly ? ch : kDesignH, cw, ch, title);
	p.Font(0);
	const int textH = st.confirmItems.empty() ? 70 : 62;
	p.TextWrapped(d.BodyX(), d.BodyTop(), d.BodyW(), textH, text, kText);
	if (!st.confirmItems.empty()) {
		const std::vector<ListLine> lines = ConfirmLines(st);
		const int rows = std::min<int>(kConfirmListRows, static_cast<int>(lines.size()));
		ListState ls = st.confirmList;
		ListPaint(p, d.BodyX(), d.BodyTop() + textH + 4, d.BodyW(), rows * kConfirmRowH + 8, lines, ls, g_confirmGeomOut, 0, kConfirmRowH, mx, my);
	}
	if (st.confirmKind == kConfirmConflict) {
		DialogButtons(p, d, {{yes, kConfirmYes, 0, true}, {st.conflictWith.size() > 1 ? "Others win" : "Other one wins", kConflictOld}}, mx, my);
		return;
	}
	DialogButtons(p, d, {{yes, kConfirmYes, 0, true}, {st.confirmKind == kConfirmRestart ? "Later" : "Cancel", kConfirmNo}}, mx, my);
}
}
