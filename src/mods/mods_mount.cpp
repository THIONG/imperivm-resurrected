// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>
#include "MinHook.h"
#include "mods/downloads.h"
#include "mods/lang.h"
#include "i18n/tr.h"
#include "core/log.h"
#include "mods/features.h"
#include "mods/patch.h"
#include "addr/engine.h"

#include "mods/mods_internal.h"

using namespace mods_impl;

namespace mods_impl {
namespace {
void *__fastcall RoOpen(void *self, void *edx, const char *path, unsigned mode, void *err)
{
	if (mode & 2) {
		return nullptr;
	}
	typedef void *(__fastcall * Open_t)(void *, void *, const char *, unsigned, void *);
	return reinterpret_cast<Open_t>(g_readOnly.origTransOpen)(self, edx, path, mode, err);
}
const char *__fastcall RoFail1(void *, void *, const char *) { return kReadOnlyError; }
const char *__fastcall RoFail2(void *, void *, const char *, const void *) { return kReadOnlyError; }
const char *__fastcall RoFail3(void *, void *, const char *, const void *, const void *) { return kReadOnlyError; }

void *GameNew(size_t n) { return reinterpret_cast<OperatorNew_t>(addr::kOperatorNew)(n); }

void *CreateLooseLayer(const std::string &prefix)
{
	void *winfs = GameNew(4);
	void *trans = GameNew(0x108);
	if (!winfs || !trans || prefix.size() >= 0x100) {
		return nullptr;
	}
	reinterpret_cast<Ctor0_t>(addr::kCWinFileServiceCtor)(winfs);
	reinterpret_cast<CTransFSCtor_t>(addr::kCTransFSCtor)(trans, nullptr, prefix.c_str(), winfs);
	if (!g_readOnly.vtableReady) {
		memcpy(g_readOnly.vtable, *reinterpret_cast<void ***>(trans), sizeof(g_readOnly.vtable));
		g_readOnly.origTransOpen = g_readOnly.vtable[3];
		g_readOnly.vtable[3] = reinterpret_cast<void *>(&RoOpen);
		g_readOnly.vtable[5] = reinterpret_cast<void *>(&RoFail1);
		g_readOnly.vtable[6] = reinterpret_cast<void *>(&RoFail2);
		g_readOnly.vtable[7] = reinterpret_cast<void *>(&RoFail2);
		g_readOnly.vtable[8] = reinterpret_cast<void *>(&RoFail2);
		g_readOnly.vtable[13] = reinterpret_cast<void *>(&RoFail3);
		g_readOnly.vtableReady = true;
	}
	*reinterpret_cast<void ***>(trans) = g_readOnly.vtable;
	return trans;
}

void ListPushFront(void *list, void *value)
{
	auto node = static_cast<void **>(GameNew(8));
	if (!node) {
		return;
	}
	char *l = static_cast<char *>(list);
	node[0] = value;
	node[1] = *reinterpret_cast<void **>(l + 0x10);
	*reinterpret_cast<void **>(l + 0x10) = node;
	*reinterpret_cast<int *>(l + 0xc) += 1;
}

std::string ReadGameLanguage()
{
	const char *base = reinterpret_cast<const char *>(kLanguageName);
	const unsigned cap = *reinterpret_cast<const unsigned *>(base + 0x18);
	const unsigned size = *reinterpret_cast<const unsigned *>(base + 0x14);
	const char *text = cap < 0x10 ? base + 4 : *reinterpret_cast<const char *const *>(base + 4);
	if (!text || size == 0 || size > 64) {
		return "";
	}
	return Lower(std::string(text, size));
}

std::string ShowStartupPicker(const std::string &current);

void MountProfileLayers(void *list)
{
	ModsApplyPendingSwaps();
	DlApplyPendingInstalls();
	ModsApplyPendingUninstalls();
	ModsScan();
	std::string name = ModsActiveProfileName();
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
		name = ShowStartupPicker(name);
		ModsSetActiveProfileName(name);
	}
	ModProfile p = ModsLoadProfile(name);
	p.mods = ModsLoadOrder(p.mods);
	{
		std::lock_guard<std::mutex> lock(g_lock);
		g_mount.profile = p;
		g_mount.known = true;
	}
	if (p.mods.empty()) {
		LogI("content", "profile \"%s\" (no mods)", p.name.c_str());
		PatchRebuild();
		ModsRebuildFeatures();
		return;
	}
	const std::string lang = LanguageLower();
	std::vector<std::string> looseLayers;
	for (const std::string &folder : p.mods) {
		ModInfo m;
		if (!ModsFind(folder, m)) {
			LogW("content", "the profile uses \"%s\" but it is not in the mods folder", folder.c_str());
			continue;
		}
		if (!m.forbidden.empty()) {
			LogW("content", "%s is NOT loaded: it brings disallowed files (%s...)", folder.c_str(), m.forbidden[0].c_str());
			continue;
		}
		const std::string vfsBase = "mods/" + folder + "/";
		const std::string diskBase = ModsDir() + "\\" + folder + "\\";
		{
			std::lock_guard<std::mutex> lock(g_lock);
			g_mount.dirs.push_back(ModsDir() + "\\" + folder);
		}
		if (IsDir(diskBase + "Local")) {
			g_origMountPakFilesInDir((vfsBase + "Local/").c_str(), list, lang.c_str());
			g_mount.localDirs.push_back(diskBase + "Local");
		}
		if (IsDir(diskBase + "Patches")) {
			g_origMountPakFilesInDir((vfsBase + "Patches/").c_str(), list, nullptr);
		}
		if (IsDir(diskBase + "Packs")) {
			g_origMountPakFilesInDir((vfsBase + "Packs/").c_str(), list, nullptr);
		}
		if (m.hasLoose) {
			looseLayers.push_back(vfsBase);
		}
		LogI("content", "loaded %s %s (%u files%s%s)", m.name.c_str(), m.version.c_str(), m.fileCount,
			m.hasLoose ? ", with loose files" : "", m.gameplay ? ", rules type" : m.kind == kModContent ? ", content type" : ", cosmetic type");
	}
	for (auto it = looseLayers.rbegin(); it != looseLayers.rend(); ++it) {
		if (void *layer = CreateLooseLayer(*it)) {
			ListPushFront(list, layer);
		}
	}
	LogI("content", "profile \"%s\" mounted (%u mods)", p.name.c_str(), static_cast<unsigned>(p.mods.size()));
	PatchRebuild();
	ModsRebuildFeatures();
}
}
}

namespace mods_impl {
void __cdecl HookMountPakFilesInDir(const char *dir, void *list, const char *lang)
{
	static bool done = false;
	if (!done && dir && _stricmp(dir, "Patches/") == 0 && list) {
		done = true;
		std::string gameLang = ReadGameLanguage();
		if (!gameLang.empty()) {
			g_language = gameLang;
		}
		MountProfileLayers(list);
	}
	g_origMountPakFilesInDir(dir, list, lang);
	if (dir && list && lang && _stricmp(dir, "Local/") == 0) {
		const std::string base = LangFallbackFor(Lower(lang));
		if (!base.empty()) {
			g_origMountPakFilesInDir(dir, list, base.c_str());
			LogI("language", "%s does not come with the game; whatever it lacks is taken from %s", Lower(lang).c_str(), base.c_str());
		}
	}
}
}

namespace mods_impl {
namespace {
struct PickerState {
	HWND list = nullptr;
	bool done = false;
	int result = -1;
};
PickerState *g_picker = nullptr;

LRESULT CALLBACK PickerProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_COMMAND:
		if (LOWORD(wp) == 1 || (LOWORD(wp) == 10 && HIWORD(wp) == LBN_DBLCLK)) {
			g_picker->result = static_cast<int>(SendMessageA(g_picker->list, LB_GETCURSEL, 0, 0));
			g_picker->done = true;
		}
		else if (LOWORD(wp) == 2) {
			g_picker->done = true;
		}
		return 0;
	case WM_CLOSE:
		g_picker->done = true;
		return 0;
	}
	return DefWindowProcA(hwnd, msg, wp, lp);
}

std::string ShowStartupPicker(const std::string &current)
{
	const std::vector<std::string> names = ModsProfileNames();
	WNDCLASSA wc = {};
	wc.lpfnWndProc = PickerProc;
	wc.hInstance = GetModuleHandleA(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
	wc.lpszClassName = "ImperivmModPicker";
	RegisterClassA(&wc);
	const int w = 380, h = 330;
	HWND hwnd = CreateWindowExA(WS_EX_TOPMOST, wc.lpszClassName, Tr("Imperivm - Content profile").c_str(),
		WS_CAPTION | WS_SYSMENU | WS_VISIBLE, (GetSystemMetrics(SM_CXSCREEN) - w) / 2,
		(GetSystemMetrics(SM_CYSCREEN) - h) / 2, w, h, nullptr, nullptr, wc.hInstance, nullptr);
	if (!hwnd) {
		return current;
	}
	HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
	PickerState st;
	g_picker = &st;
	HWND label = CreateWindowA("STATIC", Tr("Choose which content profile to start with:").c_str(), WS_CHILD | WS_VISIBLE, 12, 10, 340,
		20, hwnd, nullptr, nullptr, nullptr);
	st.list = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY, 12, 34,
		340, 210, hwnd, reinterpret_cast<HMENU>(10), nullptr, nullptr);
	HWND ok = CreateWindowA("BUTTON", Tr("Play").c_str(), WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 152, 252, 96, 28, hwnd,
		reinterpret_cast<HMENU>(1), nullptr, nullptr);
	HWND cancel = CreateWindowA("BUTTON", Tr("No changes").c_str(), WS_CHILD | WS_VISIBLE, 256, 252, 96, 28, hwnd,
		reinterpret_cast<HMENU>(2), nullptr, nullptr);
	for (HWND c : {label, st.list, ok, cancel}) {
		SendMessageA(c, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
	}
	int sel = 0;
	for (size_t i = 0; i < names.size(); i++) {
		const std::string label = ModsProfileLabel(names[i]);
		SendMessageA(st.list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
		if (names[i] == current) {
			sel = static_cast<int>(i);
		}
	}
	SendMessageA(st.list, LB_SETCURSEL, sel, 0);
	SetForegroundWindow(hwnd);
	SetFocus(st.list);
	MSG m;
	while (!st.done && GetMessageA(&m, nullptr, 0, 0) > 0) {
		if (!IsDialogMessageA(hwnd, &m)) {
			TranslateMessage(&m);
			DispatchMessageA(&m);
		}
	}
	DestroyWindow(hwnd);
	g_picker = nullptr;
	if (st.result >= 0 && st.result < static_cast<int>(names.size())) {
		LogD("content", "profile chosen at startup: %s", names[st.result].c_str());
		return names[st.result];
	}
	return current;
}
}
}
