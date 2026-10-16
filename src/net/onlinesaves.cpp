// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>
#include <string>


#include "net/onlinesaves.h"
#include "MinHook.h"
#include "hooks/hookcount.h"
#include "core/json.h"
#include "core/log.h"
#include "addr/engine.h"
#include "addr/state.h"
#include "addr/ui.h"
#include "addr/world.h"
#include "core/uiport.h"

namespace {
const int kIdFileList = 0x1015;
const int kTypeScenario = 3;

typedef void(__fastcall *Fill_t)(int *dlg);
typedef void *(__fastcall *GetChild_t)(void *dlg, void *edx, int id, const char *cls);
typedef char *(__cdecl *StrDupPool_t)(const char *);
typedef void(__cdecl *Free_t)(void *);

struct GameString {
	char raw[0x1c];
};
typedef void(__cdecl *RequestLoad_t)(GameString);

Fill_t g_origFill = nullptr;
RequestLoad_t g_origRequestLoad = nullptr;

std::string ReadGameString(const char *s)
{
	const unsigned size = *reinterpret_cast<const unsigned *>(s + 0x14);
	const unsigned cap = *reinterpret_cast<const unsigned *>(s + 0x18);
	const char *text = cap < 0x10 ? s + 4 : *reinterpret_cast<const char *const *>(s + 4);
	return text && size < 4096 ? std::string(text, size) : std::string();
}

std::string GameDir()
{
	char path[MAX_PATH];
	GetModuleFileNameA(nullptr, path, MAX_PATH);
	char *slash = strrchr(path, '\\');
	if (slash) *slash = 0;
	return path;
}

bool SaveIsOnline(std::string rel)
{
	for (char &c : rel) {
		if (c == '/') c = '\\';
	}
	Json j;
	if (!ReadJsonFile(GameDir() + "\\" + rel + ".mods.json", j)) return false;
	return j.GetBool("online", false);
}

void __fastcall HookFillFileList(int *dlg)
{
	g_origFill(dlg);
	if (dlg[0x1f] != kTypeScenario) return;
	void **vt = *reinterpret_cast<void ***>(dlg);
	char *list = static_cast<char *>(reinterpret_cast<GetChild_t>(vt[0x7c / 4])(dlg, nullptr, kIdFileList,
		*reinterpret_cast<const char **>(addr::kClassNameCUISList)));
	if (!list) return;
	const std::string folder = ReadGameString(reinterpret_cast<const char *>(dlg) + 0x60);
	const unsigned count = *reinterpret_cast<unsigned *>(list + 0x58);
	char *items = *reinterpret_cast<char **>(list + 0x94);
	for (unsigned i = 0; items && i < count && i < 512; i++) {
		char **slot = reinterpret_cast<char **>(items + i * 0x14);
		if (!*slot) continue;
		const std::string text = *slot;
		const size_t bar = text.find('|');
		if (bar == std::string::npos) continue;
		if (!SaveIsOnline(folder + text.substr(0, bar) + ".snr")) continue;
		const std::string tagged = text.substr(0, bar + 1) + "[Online] " + text.substr(bar + 1);
		if (char *dup = reinterpret_cast<StrDupPool_t>(addr::kStrDupPool)(tagged.c_str())) *slot = dup;
	}
}

void __cdecl HookRequestLoadGame(GameString s)
{
	const std::string path = ReadGameString(s.raw);
	if (*reinterpret_cast<void **>(addr::kGame) == nullptr && path.size() > 4 && _stricmp(path.c_str() + path.size() - 4, ".snr") == 0 &&
		SaveIsOnline(path)) {
		if (UiHostSave(path)) {
			LogI("saves", "online games: \"%s\" is loaded in a new room", path.c_str());
			if (*reinterpret_cast<unsigned *>(s.raw + 0x18) >= 0x10) {
				reinterpret_cast<Free_t>(addr::kGameFree)(*reinterpret_cast<void **>(s.raw + 4));
			}
			return;
		}
	}
	g_origRequestLoad(s);
}

}

void InstallOnlineSaves(bool knownExe)
{
	if (!knownExe) return;
	void *fill = reinterpret_cast<void *>(addr::kFillFileList);
	if (HookCreate(fill, reinterpret_cast<void *>(&HookFillFileList), reinterpret_cast<void **>(&g_origFill)) == MH_OK) {
		HookEnable(fill);
	}
	void *load = reinterpret_cast<void *>(addr::kRequestLoadGame);
	if (HookCreate(load, reinterpret_cast<void *>(&HookRequestLoadGame), reinterpret_cast<void **>(&g_origRequestLoad)) == MH_OK) {
		HookEnable(load);
	}
	LogI("saves", "online games: marks it in \"Load game\" and loads in a ready room");
}
