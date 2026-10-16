// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>

#include "mods/patch.h"
#include "hooks/builtinfixes.h"
#include "core/log.h"
#include "mods/mods.h"
#include "mods/patchxml.h"
#include "i18n/tr.h"

#include <algorithm>
#include <map>
#include <mutex>
#include <set>
#include "core/uiport.h"

namespace {

struct Source {
	std::string folder;
	std::string path;
	std::string text;
};

std::mutex g_lock;
bool g_ready = false;
std::map<std::string, std::vector<Source>> g_patches;
std::map<std::string, std::vector<std::string>> g_errors;
std::set<std::string> g_toasted;

std::string GameDirPath()
{
	char path[MAX_PATH];
	GetModuleFileNameA(nullptr, path, MAX_PATH);
	char *slash = strrchr(path, '\\');
	if (slash) *slash = 0;
	return path;
}

std::string Lower(std::string s)
{
	for (char &c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
	return s;
}

void Walk(const std::string &base, const std::string &sub, const std::string &folder, std::map<std::string, std::vector<Source>> &out)
{
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((base + sub + "*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return;
	do {
		const std::string name = fd.cFileName;
		if (name == "." || name == "..") continue;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			Walk(base, sub + name + "\\", folder, out);
			continue;
		}
		std::string rel = Lower(sub + name);
		std::replace(rel.begin(), rel.end(), '\\', '/');
		out[rel].push_back({folder, base + sub + name});
	} while (FindNextFileA(h, &fd));
	FindClose(h);
}

bool ReadFileText(const std::string &path, std::string &out)
{
	HANDLE h = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (h == INVALID_HANDLE_VALUE) return false;
	const DWORD size = GetFileSize(h, nullptr);
	out.assign(size == INVALID_FILE_SIZE ? 0 : size, '\0');
	DWORD got = 0;
	const BOOL ok = out.empty() || ReadFile(h, &out[0], size, &got, nullptr);
	CloseHandle(h);
	out.resize(got);
	return ok != FALSE;
}

void AddError(const std::string &folder, const std::string &rel, const std::string &message)
{
	const std::string who = folder.empty() ? "mod\\patches" : folder;
	const std::string line = rel + ": " + message;
	bool first = false, toast = false;
	{
		std::lock_guard<std::mutex> lock(g_lock);
		std::vector<std::string> &list = g_errors[who];
		first = std::find(list.begin(), list.end(), line) == list.end();
		if (first && list.size() < 50) list.push_back(line);
		toast = first && g_toasted.insert(who).second;
	}
	if (first) LogW("patch", "(%s) %s", who.c_str(), line.c_str());
	if (toast) UiToast(TrFmt("Patch with errors in %s: %s. There are more details in the content manager and the log.", {who, line}), true);
}

}

void PatchRebuild()
{
	std::map<std::string, std::vector<Source>> found;
	std::vector<std::string> dirs = ModsMountedDirs();
	for (auto it = dirs.rbegin(); it != dirs.rend(); ++it) {
		const std::string folder = it->substr(it->find_last_of('\\') + 1);
		Walk(*it + "\\patches\\", "", folder, found);
	}
	Walk(GameDirPath() + "\\mod\\patches\\", "", "", found);
	for (const auto &b : BuiltinPatches()) found[b.first].insert(found[b.first].begin(), {"Imperivm Resurrected", "", b.second});
	unsigned files = 0;
	for (const auto &kv : found) files += static_cast<unsigned>(kv.second.size());
	const size_t targets = found.size();
	{
		std::lock_guard<std::mutex> lock(g_lock);
		g_patches.swap(found);
		g_ready = true;
	}
	if (files) LogI("patch", "%u patch files for %u game files", files, static_cast<unsigned>(targets));
}

bool PatchHas(const std::string &rel)
{
	std::lock_guard<std::mutex> lock(g_lock);
	return g_ready && !g_patches.empty() && g_patches.count(rel) != 0;
}

bool PatchApply(const std::string &rel, std::string &data)
{
	std::vector<Source> sources;
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto it = g_patches.find(rel);
		if (it == g_patches.end()) return false;
		sources = it->second;
	}
	const bool isIni = rel.size() > 4 && rel.compare(rel.size() - 4, 4, ".ini") == 0;
	const bool isXml = rel.size() > 4 && rel.compare(rel.size() - 4, 4, ".xml") == 0;
	const bool isVs = rel.size() > 3 && rel.compare(rel.size() - 3, 3, ".vs") == 0;
	bool changed = false;
	for (const Source &s : sources) {
		if (!isIni && !isXml && !isVs) {
			AddError(s.folder, rel, Tr("only .xml, .ini and .vs files can be patched"));
			continue;
		}
		std::string text = s.text;
		if (text.empty() && !ReadFileText(s.path, text)) {
			AddError(s.folder, rel, Tr("the patch file cannot be read"));
			continue;
		}
		std::vector<std::string> errors;
		std::string work = data;
		const bool ok = isIni ? PatchIniText(work, text, errors) : isVs ? PatchScriptText(work, text, errors) : PatchXmlText(work, text, errors);
		for (const std::string &e : errors) AddError(s.folder, rel, e);
		if (ok) {
			data.swap(work);
			changed = true;
			LogI("patch", "(%s) %s applied%s", s.folder.empty() ? "mod\\patches" : s.folder.c_str(), rel.c_str(),
				errors.empty() ? "" : (" (" + std::to_string(errors.size()) + " operations with errors)").c_str());
		}
	}
	return changed;
}

void PatchMissingTarget(const std::string &rel)
{
	std::vector<Source> sources;
	{
		std::lock_guard<std::mutex> lock(g_lock);
		auto it = g_patches.find(rel);
		if (it == g_patches.end()) return;
		sources = it->second;
	}
	for (const Source &s : sources) AddError(s.folder, rel, Tr("the game has no such file (check the patch path)"));
}

std::vector<std::string> PatchErrorsFor(const std::string &folder)
{
	std::lock_guard<std::mutex> lock(g_lock);
	auto it = g_errors.find(folder);
	return it == g_errors.end() ? std::vector<std::string>() : it->second;
}
