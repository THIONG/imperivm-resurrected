// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>
#include <atomic>
#include "MinHook.h"

#include "i18n/tr.h"
#include "mods/mods_internal.h"

using namespace mods_impl;

namespace {
const bool g_languageSourceSet = (TrSetLanguageSource(&ModsLanguage), true);
}

namespace mods_impl {
MountPakFilesInDir_t g_origMountPakFilesInDir = nullptr;

std::mutex g_lock;
std::vector<ModInfo> g_mods;
}

namespace mods_impl {
namespace {
std::string g_gameDir;
}
}

namespace mods_impl {
std::string g_language;
MountedState g_mount;
std::atomic<bool> g_hashing{false};

std::string GameDir()
{
	if (g_gameDir.empty()) {
		char path[MAX_PATH];
		GetModuleFileNameA(nullptr, path, MAX_PATH);
		char *slash = strrchr(path, '\\');
		if (slash) {
			*slash = 0;
		}
		g_gameDir = path;
	}
	return g_gameDir;
}

std::string ModsDir() { return GameDir() + "\\mods"; }
std::string ProfilesDir() { return ModsDir() + "\\_profiles"; }
std::string IniPath() { return GameDir() + "\\imperivm-mod.ini"; }

ReadOnlyLayerState g_readOnly;

}
