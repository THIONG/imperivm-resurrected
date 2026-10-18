// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>

#include "MinHook.h"
#include "addr/engine.h"
#include "core/log.h"
#include "hooks/hookcount.h"
#include "mods/hidefiles.h"
#include "mods/vfs.h"

namespace {
typedef int(__cdecl *FileExists_t)(const char *path);
typedef const char *(__cdecl *FindFirst_t)(void *find, const char *pattern);
typedef const char *(__cdecl *FindNext_t)(void *find);
typedef int(__cdecl *FindClose_t)(void *find);

FileExists_t g_origExists = nullptr;
FindFirst_t g_origFindFirst = nullptr;
FindNext_t g_origFindNext = nullptr;
FindClose_t g_origFindClose = nullptr;

const size_t kFindNameOffset = 0x10;

std::mutex g_lock;
std::shared_ptr<const std::set<std::string>> g_hidden;
std::map<void *, std::string> g_findDirs;

std::shared_ptr<const std::set<std::string>> Hidden()
{
	std::lock_guard<std::mutex> lock(g_lock);
	return g_hidden;
}

bool FoundIsHidden(void *find, const std::set<std::string> &hidden)
{
	const char *name = *reinterpret_cast<const char **>(static_cast<char *>(find) + kFindNameOffset);
	if (!name) return false;
	std::string dir;
	{
		std::lock_guard<std::mutex> lock(g_lock);
		const auto it = g_findDirs.find(find);
		if (it == g_findDirs.end()) return false;
		dir = it->second;
	}
	return hidden.count(dir + HiddenNormalize(name)) != 0;
}

const char *SkipHidden(void *find, const char *err)
{
	const std::shared_ptr<const std::set<std::string>> hidden = Hidden();
	if (!hidden || hidden->empty()) return err;
	while (!err && FoundIsHidden(find, *hidden)) err = g_origFindNext(find);
	return err;
}

int __cdecl HookFileExists(const char *path)
{
	if (path) {
		const std::shared_ptr<const std::set<std::string>> hidden = Hidden();
		if (hidden && !hidden->empty() && hidden->count(HiddenNormalize(path))) return 0;
	}
	return g_origExists(path);
}

const char *__cdecl HookFindFirst(void *find, const char *pattern)
{
	const char *err = g_origFindFirst(find, pattern);
	const std::shared_ptr<const std::set<std::string>> hidden = Hidden();
	if (!hidden || hidden->empty()) return err;
	{
		std::lock_guard<std::mutex> lock(g_lock);
		g_findDirs[find] = HiddenFindDir(pattern);
	}
	if (err) return err;
	const char *after = SkipHidden(find, err);
	if (after) {
		g_origFindClose(find);
		std::lock_guard<std::mutex> lock(g_lock);
		g_findDirs.erase(find);
	}
	return after;
}

const char *__cdecl HookFindNext(void *find)
{
	return SkipHidden(find, g_origFindNext(find));
}

int __cdecl HookFindClose(void *find)
{
	{
		std::lock_guard<std::mutex> lock(g_lock);
		g_findDirs.erase(find);
	}
	return g_origFindClose(find);
}

bool Hook(uintptr_t at, void *detour, void **orig)
{
	void *target = reinterpret_cast<void *>(at);
	return HookCreate(target, detour, orig) == MH_OK && HookEnable(target) == MH_OK;
}
}

void VfsSetHidden(const std::set<std::string> &paths)
{
	auto copy = std::make_shared<const std::set<std::string>>(paths);
	std::lock_guard<std::mutex> lock(g_lock);
	g_hidden = copy;
}

bool VfsIsHidden(const std::string &rel)
{
	const std::shared_ptr<const std::set<std::string>> hidden = Hidden();
	return hidden && !hidden->empty() && hidden->count(rel) != 0;
}

void InstallHiddenHooks()
{
	const bool ok = Hook(addr::kVfsFileExists, reinterpret_cast<void *>(&HookFileExists), reinterpret_cast<void **>(&g_origExists)) &&
		Hook(addr::kVfsFindFirst, reinterpret_cast<void *>(&HookFindFirst), reinterpret_cast<void **>(&g_origFindFirst)) &&
		Hook(addr::kVfsFindNext, reinterpret_cast<void *>(&HookFindNext), reinterpret_cast<void **>(&g_origFindNext)) &&
		Hook(addr::kVfsFindClose, reinterpret_cast<void *>(&HookFindClose), reinterpret_cast<void **>(&g_origFindClose));
	if (ok) LogD("vfs", "content packs can hide game files");
	else LogE("vfs", "could not hook the file search: content packs cannot hide game files");
}
