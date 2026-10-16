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
namespace {
const char *const kIoTagValues[] = {"", "Map", "Scenario", "Campaign", "Conquest", "Balance", "Units", "Civilization",
	"Graphics", "Audio", "Voices", "Interface", "Translation", "Total conversion"};
const char *const kIoTagNames[] = {"All types", "Maps", "Scenarios", "Campaigns", "Conquest", "Rules and balance", "Units",
	"Civilizations", "Graphics", "Audio", "Voices", "Interface", "Translations", "Total conversion"};
const int kIoTagCount = 14;
}
}

namespace modui_impl {
std::string IoTagName(const std::string &tag)
{
	for (int i = 1; i < kIoTagCount; i++) {
		if (tag == kIoTagValues[i]) return kIoTagNames[i];
	}
	static const char *const kCats[][2] = {{"Maps", "Maps"}, {"Scenarios", "Scenarios"}, {"Campaigns", "Campaigns"}, {"Conquest", "Conquest"},
		{"Gameplay & Balance", "Rules and balance"}, {"Units & Heroes", "Units & Heroes"}, {"Civilizations", "Civilizations"},
		{"Graphics", "Graphics"}, {"Audio & Voices", "Audio & Voices"}, {"Interface", "Interface"}, {"Translations", "Translations"},
		{"Total Conversions", "Total conversion"}};
	for (const auto &c : kCats) {
		if (tag == c[0]) return c[1];
	}
	static const char *const kOther[][2] = {{"Gameplay", "Gameplay"}, {"Cosmetic", "Cosmetic"}, {"Spanish", "Spanish"},
		{"English", "English"}, {"Italian", "Italian"}, {"German", "German"}, {"French", "French"}, {"Polish", "Polish"},
		{"Multilingual", "Several languages"}, {"Online tested", "Tested online"}};
	for (const auto &o : kOther) {
		if (tag == o[0]) return o[1];
	}
	if (tag.size() == 1 && isdigit(static_cast<unsigned char>(tag[0]))) return tag + Tr(" players");
	return tag;
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
		const bool bad = m && (!m->forbidden.empty() || !m->manifestError.empty());
		switch (st.filter) {
		case 1: if (!r.active) continue; break;
		case 2: if (r.active) continue; break;
		case 3: if (!m || !m->gameplay) continue; break;
		case 4: if (!m || m->kind != kModContent) continue; break;
		case 5: if (!m || m->kind != kModCosmetic) continue; break;
		case 6: if (m && !bad) continue; break;
		default: break;
		}
		out.push_back(static_cast<int>(i));
	}
	return out;
}

void BuildRows()
{
	g_s.rows.clear();
	for (size_t i = 0; i < g_s.profile.mods.size(); i++) {
		Row r;
		r.folder = g_s.profile.mods[i];
		r.active = true;
		r.priority = static_cast<int>(i);
		g_s.rows.push_back(r);
	}
	for (const ModInfo &m : g_s.mods) {
		if (std::find(g_s.profile.mods.begin(), g_s.profile.mods.end(), m.folder) == g_s.profile.mods.end()) {
			Row r;
			r.folder = m.folder;
			g_s.rows.push_back(r);
		}
	}
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
	if (!DlFindUpdate(m.id, m.version, out, m.gbId)) return false;
	for (const ModInfo &o : st.mods) {
		if (&o != &m && ModsSameMod(o, m) && ModsCompareVersions(o.version, out.version) >= 0) return false;
	}
	return true;
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
	SetStatus(ModsRestartNeeded() ? Tr("Profile \"") + name + Tr("\" selected: applied on restart") : "");
}

void PlaceConflict(bool newWins)
{
	auto &mods = g_s.profile.mods;
	const std::string folder = g_s.conflictFolder;
	const auto it = std::find(mods.begin(), mods.end(), folder);
	if (it == mods.end()) return;
	mods.erase(it);
	size_t lo = mods.size(), hi = 0;
	bool any = false;
	for (size_t i = 0; i < mods.size(); i++) {
		if (std::find(g_s.conflictWith.begin(), g_s.conflictWith.end(), mods[i]) == g_s.conflictWith.end()) continue;
		lo = (std::min)(lo, i);
		hi = (std::max)(hi, i + 1);
		any = true;
	}
	mods.insert(mods.begin() + static_cast<std::ptrdiff_t>(!any ? mods.size() : newWins ? lo : hi), folder);
	CommitProfile();
	SelectFolder(folder);
	const ModInfo *m = FindMod(folder);
	std::string with;
	for (const std::string &f : g_s.conflictWith) {
		const ModInfo *o = FindMod(f);
		with += (with.empty() ? "" : ", ") + (o ? o->name : f);
	}
	const std::string name = m ? m->name : folder;
	SetStatus(newWins ? name + Tr(" takes priority over ") + with : with + Tr(" takes priority over ") + name);
}

void CommitProfile()
{
	if (g_s.profile.name == kOriginalName) {
		g_s.profile.name = kNoProfileName;
		SetStatus("Changes saved to \"No profile\" (the Original is not changed). Click \"Save as profile\" to name it.");
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
	if (_stricmp(name.c_str(), kOriginalName) == 0 || _stricmp(name.c_str(), kNoProfileName) == 0 ||
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
	if (mode == kNameSaveAs || mode == kNameDup) p.mods = g_s.profile.mods;
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
		: mode == kNameDup			   ? UniqueProfileName(g_s.profile.name + Tr(" (copy)"))
		: mode == kNameNew			   ? UniqueProfileName("Profile")
									   : UniqueProfileName("My content");
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
		size_t newest = 0;
		for (size_t i = 1; i < f.files.size(); i++) {
			if (ModsCompareVersions(f.files[i].version, f.files[newest].version) > 0) newest = i;
		}
		for (size_t i = 0; i < f.files.size(); i++) {
			const GbFile &v = f.files[i];
			std::string s = v.version.empty() ? std::string("No version") : v.version;
			if (v.bytes) s += "  ·  " + HumanBytes(v.bytes);
			const std::string d = DateText(v.date);
			if (!d.empty()) s += "  ·  " + d;
			if (i == newest) s += "  ·  latest";
			if (InstalledVersionOf(st, st.verMod, v.version)) s += "  ·  installed";
			items.push_back(s);
		}
		return items;
	}
	if (st.drop == kDropProfile) {
		items = st.profileNames;
		items.push_back("+ New profile");
	}
	else if (st.drop == kDropMenu) {
		items = {"Rename", "Duplicar", "Remove"};
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
	for (int i = 0; i < kIoFilterCount; i++) {
		if (g_s.ioFilterMask & (1u << i)) q.tag += std::string(q.tag.empty() ? "" : ",") + kIoFilters[i].value + "|" + kIoFilters[i].label;
	}
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
		else if (index == 1) StartName(kNameDup);
		else if (index == 2) {
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

void MoveActive(int fromRow, int toRow)
{
	if (fromRow < 0 || toRow < 0 || fromRow >= static_cast<int>(g_s.rows.size()) || toRow >= static_cast<int>(g_s.rows.size())) return;
	const Row from = g_s.rows[fromRow], to = g_s.rows[toRow];
	if (!from.active || !to.active || from.priority == to.priority) return;
	auto &mods = g_s.profile.mods;
	const std::string folder = mods[from.priority];
	mods.erase(mods.begin() + from.priority);
	mods.insert(mods.begin() + to.priority, folder);
	CommitProfile();
	SelectFolder(folder);
}

void DiscardPendingChanges()
{
	const ModProfile mounted = ModsMountedProfile();
	if (mounted.name == kOriginalName) {
		if (ModsActiveProfileName() == kNoProfileName) ModsDeleteProfile(kNoProfileName);
	}
	else {
		ModsSaveProfile(mounted);
	}
	ModsSetActiveProfileName(mounted.name);
	Reload();
}


ListGeom g_confirmGeomOut;

Canvas g_canvas;
Canvas g_toastCanvas;
}

namespace modui_impl {
namespace {
Canvas g_specBarCanvas;
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
	g_s.dragRow = -1;
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
