// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>
#include <mutex>
#include "core/guard.h"
#include "i18n/tr.h"

#include "ui/modui_internal.h"

using namespace modui_impl;

namespace modui_impl {
std::string IoTagsText(const GbMod &m)
{
	std::vector<std::string> parts;
	if (m.facts.category >= 0) parts.push_back(Tr(CatalogCategoryLabel(m.facts.category)));
	else if (!m.category.empty()) parts.push_back(m.category);
	if (m.facts.players) parts.push_back(CatalogPlayersText(m.facts.players) + Tr(" players"));
	for (int l = 0; l < kLangCount; l++) {
		if ((m.facts.languages >> l) & 1) parts.push_back(Tr(CatalogLanguageLabel(l)));
	}
	for (const std::string &t : m.tags) {
		if (t == m.category || CatalogCategoryFromName(t) >= 0 || CatalogPlayersFromTag(t) || CatalogLanguagesFromTag(t)) continue;
		parts.push_back(Tr(t));
	}
	std::string out;
	for (const std::string &s : parts) out += (out.empty() ? "" : ", ") + s;
	return out;
}
}

namespace modui_impl {
namespace {
const int kFilterCount = 7;
}
}

namespace modui_impl {
std::mutex g_lock;
State g_s;

RichTextState g_rich;
ListGeom g_listGeomOut;
DropListState g_drop;
}

namespace modui_impl {
namespace {
std::string LowerAscii(std::string s)
{
	for (char &c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
	return s;
}
}
}

namespace modui_impl {
const ModInfo *FindModIn(const std::vector<ModInfo> &mods, const std::string &folder)
{
	for (const ModInfo &m : mods) {
		if (m.folder == folder) return &m;
	}
	return nullptr;
}

const ModInfo *FindMod(const std::string &folder) { return FindModIn(g_s.mods, folder); }

std::string OverlapText(const State &st, const std::string &folder, const ModIssues &is)
{
	std::vector<std::string> group = is.overlapFolders;
	group.push_back(folder);
	std::string used = folder;
	for (const std::string &f : ModsLoadOrder(st.profile.mods)) {
		if (std::find(group.begin(), group.end(), f) != group.end()) {
			used = f;
			break;
		}
	}
	const ModInfo *m = FindModIn(st.mods, folder);
	const ModInfo *u = FindModIn(st.mods, used);
	std::string with;
	for (const std::string &o : is.overlapWith) with += (with.empty() ? "" : ", ") + o;
	return TrFmt("%s changes the same files as %s; the files of %s are used", {m ? m->name : folder, with, u ? u->name : used});
}

bool Pending(const State &st, const std::string &folder)
{
	return std::find(st.pendingUninstall.begin(), st.pendingUninstall.end(), folder) != st.pendingUninstall.end();
}

std::vector<int> VisibleRows(const State &st)
{
	std::vector<int> out;
	const std::string q = LowerAscii(st.searchText);
	for (size_t i = 0; i < st.rows.size(); i++) {
		const Row &r = st.rows[i];
		const ModInfo *m = FindModIn(st.mods, r.folder);
		if (!q.empty()) {
			const std::string hay = LowerAscii((m ? m->name : "") + " " + r.folder);
			if (hay.find(q) == std::string::npos) continue;
		}
		const auto issues = st.issues.find(r.folder);
		const bool bad = !m || (issues != st.issues.end() && !issues->second.problems.empty());
		switch (st.filter) {
		case 1: if (!r.active) continue; break;
		case 2: if (r.active) continue; break;
		case 3: if (!m || !m->gameplay) continue; break;
		case 4: if (!m || m->kind != kModContent) continue; break;
		case 5: if (!m || m->kind != kModCosmetic) continue; break;
		case 6: if (!bad) continue; break;
		default: break;
		}
		out.push_back(static_cast<int>(i));
	}
	return out;
}

void BuildRows()
{
	g_s.rows.clear();
	const auto isActive = [](const std::string &folder) {
		return std::find(g_s.profile.mods.begin(), g_s.profile.mods.end(), folder) != g_s.profile.mods.end();
	};
	for (const ModInfo &m : g_s.mods) {
		Row r;
		r.folder = m.folder;
		r.active = isActive(m.folder);
		g_s.rows.push_back(r);
	}
	for (const std::string &folder : g_s.profile.mods) {
		if (FindMod(folder)) continue;
		Row r;
		r.folder = folder;
		r.active = true;
		g_s.rows.push_back(r);
	}
	const auto key = [](const Row &r) {
		const ModInfo *m = FindMod(r.folder);
		std::string k = (m ? m->name : r.folder) + "\x01" + r.folder;
		for (char &c : k) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
		return k;
	};
	std::sort(g_s.rows.begin(), g_s.rows.end(), [&](const Row &a, const Row &b) { return key(a) < key(b); });
	if (g_s.selected >= static_cast<int>(g_s.rows.size())) {
		g_s.selected = static_cast<int>(g_s.rows.size()) - 1;
	}
	if (g_s.selected < 0) {
		g_s.selected = 0;
	}
}

void BuildUsage()
{
	g_s.usedBy.clear();
	g_s.pendingUninstall.clear();
	for (const ModInfo &m : g_s.mods) {
		g_s.usedBy[m.folder] = ModsProfilesUsing(m.folder);
		if (ModsUninstallPending(m.folder)) g_s.pendingUninstall.push_back(m.folder);
	}
	g_s.issues.clear();
	for (const ModInfo &m : g_s.mods) g_s.issues[m.folder] = ModsIssuesFor(g_s.profile, m.folder);
}

void Reload()
{
	g_s.mods = ModsList();
	g_s.profileNames = ModsProfileNames();
	g_s.profile = ModsLoadProfile(ModsActiveProfileName());
	BuildRows();
	BuildUsage();
	g_s.lastRefresh = GetTickCount();
	g_s.restartNeeded = ModsRestartNeeded();
}

int FieldOfAction(int action)
{
	return action == kFocusSearch ? kSearchField : action == kIoFocusSearch ? kIoSearchField : action == kFocusName ? kNameField : kNoField;
}
int ActionOfField(int field)
{
	return field == kSearchField ? kFocusSearch : field == kIoSearchField ? kIoFocusSearch : field == kNameField ? kFocusName : -1;
}
std::string &FieldRef(int field)
{
	return field == kSearchField ? g_s.searchText : field == kIoSearchField ? g_s.ioSearch : g_s.nameText;
}
void FocusField(int field)
{
	g_s.focus = field;
	g_s.tabAction = -1;
	if (field != kNoField) EditToEnd(FieldRef(field), g_s.edit);
}

void SetStatus(const std::string &s)
{
	if (s.empty()) return;
	g_s.toast = s;
	g_s.toastErr = false;
	g_s.toastLen = 6000;
	g_s.toastUntil = GetTickCount() + g_s.toastLen;
	LogI("ui", "notice: %s", TrSource(s).c_str());
}

bool FindUpdateFor(const State &st, const ModInfo &m, CatalogEntry &out)
{
	return DlUpdateFor(m, st.mods, out);
}
}

namespace modui_impl {
namespace {
bool ProfileExists(const std::string &name)
{
	for (const std::string &n : g_s.profileNames) {
		if (_stricmp(n.c_str(), name.c_str()) == 0) return true;
	}
	return false;
}

std::string UniqueProfileName(const std::string &base)
{
	for (int n = 1;; n++) {
		std::string name = n == 1 ? base : base + " " + std::to_string(n);
		if (!ProfileExists(name)) return name;
	}
}
}
}

namespace modui_impl {
void SwitchProfile(const std::string name)
{
	ModsSetActiveProfileName(name);
	Reload();
	SetStatus(ModsRestartNeeded() ? Tr("Profile \"") + ModsProfileLabel(name) + Tr("\" selected: applied on restart") : "");
}

void CommitProfile()
{
	if (g_s.profile.name == kOriginalName) {
		ModProfile mine = ModsLoadProfile(kNoProfileName);
		for (const std::string &f : g_s.profile.mods) {
			const bool has = std::find_if(mine.mods.begin(), mine.mods.end(), [&](const std::string &m) { return _stricmp(m.c_str(), f.c_str()) == 0; }) != mine.mods.end();
			if (!has) mine.mods.push_back(f);
		}
		g_s.profile = mine;
		SetStatus("Enabled in \"My content\". The Original always stays without content.");
	}
	ModsSaveProfile(g_s.profile);
	ModsSetActiveProfileName(g_s.profile.name);
	g_s.profileNames = ModsProfileNames();
	BuildRows();
	BuildUsage();
}

void SelectFolder(const std::string &folder)
{
	for (size_t i = 0; i < g_s.rows.size(); i++) {
		if (g_s.rows[i].folder == folder) g_s.selected = static_cast<int>(i);
	}
}

void FinishName()
{
	std::string name = g_s.nameText;
	name.erase(0, name.find_first_not_of(" \t"));
	name.erase(name.find_last_not_of(" \t") + 1);
	if (name.empty()) {
		SetStatus("Enter a name for the profile");
		return;
	}
	if (name.find_first_of("\\/:*?\"<>|") != std::string::npos) {
		SetStatus("The name cannot contain \\ / : * ? \" < > |");
		return;
	}
	if (ModsIsReservedProfileName(name) ||
		(ProfileExists(name) && !(g_s.nameMode == kNameRename && _stricmp(name.c_str(), g_s.profile.name.c_str()) == 0))) {
		SetStatus(Tr("There is already a profile named \"") + name + "\"");
		return;
	}
	const int mode = g_s.nameMode;
	g_s.nameMode = kNameNone;
	g_s.focus = kNoField;
	if (mode == kNameRename) {
		const std::string old = g_s.profile.name;
		if (old == name) return;
		if (ModsRenameProfile(old, name)) {
			Reload();
			SetStatus(Tr("Profile \"") + old + Tr("\" renamed to \"") + name + "\"");
		}
		else {
			SetStatus("Could not rename the profile");
		}
		return;
	}
	ModProfile p;
	p.name = name;
	if (mode == kNameSaveAs) p.mods = g_s.profile.mods;
	ModsSaveProfile(p);
	SwitchProfile(name);
	SetStatus(mode == kNameNew ? Tr("Profile \"") + name + Tr("\" created (empty): enable its content in the list")
		: Tr("Profile \"") + name + Tr("\" saved"));
}

void StartName(int mode)
{
	g_s.nameMode = mode;
	g_s.focus = kNameField;
	g_s.tabAction = -1;
	g_s.drop = kDropNone;
	g_s.nameText = mode == kNameRename ? g_s.profile.name
		: UniqueProfileName(Tr("Profile"));
	EditSelectAll(g_s.nameText, g_s.edit);
}
}

namespace modui_impl {
namespace {
const ModInfo *InstalledVersionOf(const State &st, const GbMod &m, const std::string &version)
{
	for (const ModInfo &mi : st.mods) {
		const bool same = m.id && mi.gbId == m.id;
		if (same && mi.version == version) return &mi;
	}
	return nullptr;
}

std::string DateText(uint64_t unix)
{
	if (!unix) return "";
	const __time64_t t = static_cast<__time64_t>(unix);
	tm local = {};
	if (_localtime64_s(&local, &t) != 0) return "";
	char buf[16];
	strftime(buf, sizeof(buf), "%d/%m/%Y", &local);
	return buf;
}
}
}

namespace modui_impl {
std::vector<std::string> DropItems(const State &st)
{
	std::vector<std::string> items;
	if (st.drop == kDropVersions) {
		if (!st.verHas) return {"Loading versions..."};
		const GbFiles f = GbGetFiles(st.verMod.id);
		if (f.loading) return {"Loading versions..."};
		if (!f.error.empty()) return {"Could not read the versions"};
		if (f.files.empty()) return {"This content pack has no published versions"};
		const GbFile *latest = GbLatestFile(f.files);
		for (size_t i = 0; i < f.files.size(); i++) {
			const GbFile &v = f.files[i];
			std::string s = v.version.empty() ? std::string("No version") : v.version;
			if (v.bytes) s += "  ·  " + HumanBytes(v.bytes);
			const std::string d = DateText(v.date);
			if (!d.empty()) s += "  ·  " + d;
			if (&v == latest) s += "  ·  latest";
			if (InstalledVersionOf(st, st.verMod, v.version)) s += "  ·  installed";
			items.push_back(s);
		}
		return items;
	}
	if (st.drop == kDropProfile) {
		for (const std::string &n : st.profileNames) items.push_back(ModsProfileLabel(n));
		items.push_back("+ New profile");
	}
	else if (st.drop == kDropMenu) {
		items = {"Rename", "Remove"};
	}
	else if (st.drop == kDropFilter) {
		for (int i = 0; i < kFilterCount; i++) items.push_back(kFilterNames[i]);
	}
	else if (st.drop == kDropIoSort) {
		for (const char *s : kIoSortNames) items.push_back(s);
	}
	return items;
}

void IoSearch()
{
	GbQuery q;
	q.search = g_s.ioSearch;
	q.filter = g_s.ioFilter;
	q.sort = g_s.ioSort;
	q.page = g_s.ioPage;
	q.perPage = kIoPerPage;
	g_s.ioDetail = -1;
	g_s.ioRequested = true;
	GbSearch(q);
}
}

namespace modui_impl {
namespace {
HWND GameWindow()
{
	HWND h = GetForegroundWindow();
	DWORD pid = 0;
	GetWindowThreadProcessId(h, &pid);
	return pid == GetCurrentProcessId() ? h : nullptr;
}
}
}

namespace modui_impl {
void OpenWeb(const std::string &url)
{
	if (url.compare(0, 8, "https://") != 0) return;
	HWND game = GameWindow();
	SafeThread([url, game] {
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (game && !(GetWindowLongA(game, GWL_STYLE) & WS_CAPTION)) PostMessageA(game, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		CoUninitialize();
	}).detach();
}

void OpenFolderAsync(const std::string &folder)
{
	HWND game = GameWindow();
	SafeThread([folder, game] {
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (game && !(GetWindowLongA(game, GWL_STYLE) & WS_CAPTION)) PostMessageA(game, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		ModsOpenFolder(folder);
		CoUninitialize();
	}).detach();
}

void PickDrop(int index)
{
	const int drop = g_s.drop;
	g_s.drop = kDropNone;
	if (drop == kDropProfile) {
		const int n = static_cast<int>(g_s.profileNames.size());
		if (index == n) StartName(kNameNew);
		else if (index >= 0 && index < n) SwitchProfile(g_s.profileNames[index]);
	}
	else if (drop == kDropMenu) {
		if (index == 0) StartName(kNameRename);
		else if (index == 1) {
			g_s.confirm = true;
			g_s.confirmKind = kConfirmDelete;
		}
	}
	else if (drop == kDropFilter) {
		g_s.filter = std::max(0, std::min(kFilterCount - 1, index));
		g_s.list.scroll = 0;
	}
	else if (drop == kDropVersions) {
		if (!g_s.verHas) return;
		const GbFiles f = GbGetFiles(g_s.verMod.id);
		if (index < 0 || index >= static_cast<int>(f.files.size())) return;
		const GbFile &v = f.files[index];
		if (InstalledVersionOf(g_s, g_s.verMod, v.version)) {
			SetStatus(Tr("You already have version ") + v.version + Tr(" of ") + g_s.verMod.name);
			return;
		}
		GbInstallVersion(g_s.verMod, v);
	}
	else if (drop == kDropIoSort) {
		g_s.ioSort = std::max(0, std::min(3, index));
		g_s.ioPage = 0;
		IoSearch();
	}
}


ListGeom g_confirmGeomOut;

Canvas g_canvas;
Canvas g_toastCanvas;
}

namespace modui_impl {
namespace {
}
}

namespace modui_impl {
HoverState g_hover;

SpectatorBarState g_spec;
}

void ModUiDrawHoverCard(uint32_t *buf, int w, int h, const std::string &text, COLORREF accent, int atX, int centerY)
{
	static Canvas canvas;
	const float s = std::max(0.6f, std::min(1.6f, h / 1080.0f * 1.2f));
	Card card;
	card.text = text;
	card.accent = accent;
	card.atX = atX;
	card.atCenterY = centerY;
	DrawCard(canvas, buf, w, h, card, s, 235);
}

void ModUiOpen()
{
	std::lock_guard<std::mutex> lock(g_lock);
	Reload();
	g_s.open = true;
	g_s.confirm = false, g_s.confirmItems.clear();
	g_s.confirmOnly = false;
	g_s.drop = kDropNone;
	g_s.nameMode = kNameNone;
	g_s.dlFinished = DlStatus().finished;
	if (!g_s.catalogRequested) {
		g_s.catalogRequested = true;
		DlRefreshCatalogs();
	}
	g_s.restartReason.clear();
	LogI("content", "mod manager: open (profile \"%s\", %u mods installed)", g_s.profile.name.c_str(),
		static_cast<unsigned>(g_s.mods.size()));
}

void ModUiAskRestart(const std::string &reason, int largeAddress, const std::vector<std::string> &items, const std::string &itemsTitle)
{
	std::lock_guard<std::mutex> lock(g_lock);
	Reload();
	if (!g_s.open) {
		g_s.open = true;
		g_s.confirmOnly = true;
	}
	g_s.confirm = true;
	g_s.confirmKind = kConfirmRestart;
	g_s.restartReason = reason;
	g_s.restartLargeAddress = largeAddress;
	g_s.confirmItems = items;
	g_s.confirmItemsTitle = itemsTitle;
	g_s.confirmList = ListState();
}

int ModUiUpdatesAvailable()
{
	return UpdatesAvailable();
}

void ModUiResetUpdates()
{
	ResetUpdatesCache();
}

bool ModUiIsOpen()
{
	std::lock_guard<std::mutex> lock(g_lock);
	return g_s.open;
}

void ModUiToast(const std::string &text, bool error)
{
	std::lock_guard<std::mutex> lock(g_lock);
	g_s.toast = text;
	g_s.toastErr = error;
	g_s.toastLen = error ? 10000 : 6000;
	g_s.toastUntil = GetTickCount() + g_s.toastLen;
	LogI("ui", "notice: %s", TrSource(text).c_str());
}
