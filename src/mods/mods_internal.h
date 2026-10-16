// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <windows.h>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <cstdint>
#include "MinHook.h"
#include "core/sha256.h"
#include "core/versioncmp.h"
#include "mods/modrules.h"
#include "mods/mods.h"
#include "mods/savelib.h"
#include "addr/state.h"

namespace mods_impl {

const char kOriginal[] = "Original";

const char kNoProfile[] = "No profile";

const uintptr_t kLanguageName = addr::kLanguageSettings + 0x20;

typedef void(__cdecl *MountPakFilesInDir_t)(const char *, void *, const char *);

typedef void *(__cdecl *OperatorNew_t)(size_t);

typedef void *(__fastcall *Ctor0_t)(void *self);

typedef void *(__fastcall *CTransFSCtor_t)(void *self, void *edx, const char *prefix, void *target);

extern MountPakFilesInDir_t g_origMountPakFilesInDir;
extern std::mutex g_lock;
extern std::vector<ModInfo> g_mods;
extern std::string g_language;
struct MountedState {
	ModProfile profile;
	bool known = false;
	std::vector<std::string> localDirs;
	std::vector<std::string> dirs;
};
extern MountedState g_mount;
extern std::atomic<bool> g_hashing;
std::string GameDir();
std::string ModsDir();
std::string ProfilesDir();
std::string IniPath();
bool IsDir(const std::string &p);
struct ScanFile {
	std::string rel;
	uint64_t size = 0;
	uint64_t mtime = 0;
	bool gameplay = false;
	bool content = false;
};

extern std::map<std::string, std::vector<ScanFile>> g_scanFiles;
void Walk(const std::string &base, const std::string &rel, std::vector<ScanFile> &out, int depth = 0);
std::string LanguageLower();
std::string LocalizedText(const Json &j, const char *key, const std::string &def);
ModInfo ScanMod(const std::string &folder, std::vector<ScanFile> &files);
void StartHashing();
ModProfile ReadProfileFile(const std::string &path);
const char kReadOnlyError[] = "MODREADONLY";

struct ReadOnlyLayerState {
	void *vtable[40];
	bool vtableReady = false;
	void *origTransOpen = nullptr;
};
extern ReadOnlyLayerState g_readOnly;
void __cdecl HookMountPakFilesInDir(const char *dir, void *list, const char *lang);

}
