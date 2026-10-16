// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>

#include <algorithm>
#include <mutex>
#include <set>
#include <string>
#include <map>

#include "mods/vfs.h"
#include "MinHook.h"
#include "hooks/hookcount.h"
#include "hooks/builtinfixes.h"
#include "core/config.h"
#include "mods/lang.h"
#include "core/log.h"
#include "mods/mods.h"
#include "mods/modvars.h"
#include "hooks/gamefiles.h"
#include "hooks/gamestate.h"
#include "hooks/tweaks.h"
#include "mods/pak.h"
#include "mods/patch.h"
#include "mods/savelib.h"
#include "i18n/tr.h"
#include "addr/engine.h"
#include "core/version.h"

namespace {
typedef void *(__cdecl *VfsOpen_t)(const char *path, unsigned mode, void *err);
VfsOpen_t g_origOpen = nullptr;

typedef const char *(__fastcall *CFileRead_t)(void *self, void *edx, void *buf, unsigned len, unsigned *read);
typedef void(__fastcall *CFileDelete_t)(void *self, void *edx, int flags);

std::string g_gameDir;
std::mutex g_lock;
std::set<std::string> g_dumped;
std::set<std::string> g_reportedOverrides;
std::set<std::string> g_missingSounds;

std::string Normalize(const char *path)
{
	std::string s(path);
	std::replace(s.begin(), s.end(), '\\', '/');
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
	while (!s.empty() && s[0] == '/') {
		s.erase(0, 1);
	}
	return s;
}

std::string DiskPath(const std::string &folder, const std::string &rel)
{
	std::string p = g_gameDir + "\\" + folder + "\\" + rel;
	std::replace(p.begin(), p.end(), '/', '\\');
	return p;
}

bool DiskFileExists(const std::string &p)
{
	DWORD a = GetFileAttributesA(p.c_str());
	return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

void MakeDirsFor(const std::string &file)
{
	for (size_t i = g_gameDir.size() + 1; i < file.size(); i++) {
		if (file[i] == '\\') {
			CreateDirectoryA(file.substr(0, i).c_str(), nullptr);
		}
	}
}

bool ShouldDump(const std::string &rel)
{
	const std::string &list = GetConfig().dumpFiles;
	if (list.empty()) {
		return false;
	}
	if (list == "*") {
		return true;
	}
	size_t start = 0;
	while (start <= list.size()) {
		size_t end = list.find(',', start);
		std::string item = Normalize(list.substr(start, end == std::string::npos ? std::string::npos : end - start).c_str());
		item.erase(0, item.find_first_not_of(' '));
		if (!item.empty() && item == rel) {
			return true;
		}
		if (end == std::string::npos) {
			break;
		}
		start = end + 1;
	}
	return false;
}

void DumpAndClose(void *file, const std::string &rel)
{
	void **vtable = *reinterpret_cast<void ***>(file);
	auto read = reinterpret_cast<CFileRead_t>(vtable[0xc / 4]);
	auto destroy = reinterpret_cast<CFileDelete_t>(vtable[0x8 / 4]);

	std::vector<char> data;
	char buf[16384];
	for (;;) {
		unsigned got = 0;
		const char *err = read(file, nullptr, buf, sizeof(buf), &got);
		if (got) {
			data.insert(data.end(), buf, buf + got);
		}
		if (err || got == 0) {
			break;
		}
	}
	destroy(file, nullptr, 1);

	std::string out = DiskPath("mod-dump", rel);
	MakeDirsFor(out);
	FILE *f = nullptr;
	if (fopen_s(&f, out.c_str(), "wb") == 0 && f) {
		fwrite(data.data(), 1, data.size(), f);
		fclose(f);
		LogD("vfs", "dump %s (%u bytes) -> mod-dump\\", rel.c_str(), static_cast<unsigned>(data.size()));
	}
}

typedef std::map<std::string, std::vector<std::string>> VoiceFiles;
std::map<std::string, VoiceFiles> g_voiceDirs;
bool g_voiceIndexLoaded = false;
std::map<std::string, std::vector<std::string>> g_voiceUsable;

bool IsSoundName(const std::string &n)
{
	return n.size() > 4 && (n.compare(n.size() - 4, 4, ".wav") == 0 || n.compare(n.size() - 4, 4, ".ogg") == 0);
}

void AddVoice(const std::string &name)
{
	const std::string prefix = "currentlang/voices/";
	std::string unitAndFile;
	if (name.compare(0, prefix.size(), prefix) == 0) {
		unitAndFile = name.substr(prefix.size());
	}
	else {
		unitAndFile = name;
	}
	size_t slash = unitAndFile.find('/');
	if (!IsSoundName(name) || slash == std::string::npos || slash != unitAndFile.rfind('/')) {
		return;
	}
	auto &paths = g_voiceDirs[unitAndFile.substr(0, slash)][unitAndFile.substr(slash + 1)];
	for (const std::string &p : {prefix + unitAndFile, "currentlang/" + unitAndFile, unitAndFile}) {
		if (std::find(paths.begin(), paths.end(), p) == paths.end()) {
			paths.push_back(p);
		}
	}
}

void LoadPackIndex(const std::string &pak)
{
	std::vector<PakEntry> entries;
	ReadPakIndex(pak, entries);
	for (const PakEntry &e : entries) {
		AddVoice(Normalize(e.name.c_str()));
	}
	LogD("audio", "voices: index of %s (%u entries)", pak.c_str() + g_gameDir.size() + 1, static_cast<unsigned>(entries.size()));
}

void LoadPackIndexDir(const std::string &dir)
{
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((dir + "\\*.pak").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) {
		return;
	}
	do {
		LoadPackIndex(dir + "\\" + fd.cFileName);
	} while (FindNextFileA(h, &fd));
	FindClose(h);
}

void LoadVoiceIndex()
{
	if (g_voiceIndexLoaded) {
		return;
	}
	g_voiceIndexLoaded = true;
	for (const std::string &dir : ModsMountedLocalDirs()) {
		LoadPackIndexDir(dir);
	}
	LoadPackIndexDir(g_gameDir + "\\local");
}

size_t EditDistance(const std::string &a, const std::string &b)
{
	std::vector<size_t> row(b.size() + 1);
	for (size_t j = 0; j <= b.size(); j++) {
		row[j] = j;
	}
	for (size_t i = 1; i <= a.size(); i++) {
		size_t diag = row[0];
		row[0] = i;
		for (size_t j = 1; j <= b.size(); j++) {
			size_t up = row[j];
			row[j] = (std::min)({row[j] + 1, row[j - 1] + 1, diag + (a[i - 1] != b[j - 1] ? 1u : 0u)});
			diag = up;
		}
	}
	return row[b.size()];
}

std::vector<std::string> ResolveMissingVoice(const std::string &rel, unsigned mode, void *err)
{
	const std::string prefix = "currentlang/voices/";
	if (rel.compare(0, prefix.size(), prefix) != 0) {
		return {};
	}
	size_t slash = rel.find('/', prefix.size());
	if (slash == std::string::npos) {
		return {};
	}
	const std::string unit = rel.substr(prefix.size(), slash - prefix.size());
	const std::string reqFile = rel.substr(slash + 1);
	const int n = atoi(rel.c_str() + slash + 1);
	std::vector<std::string> first;
	if (ModsLanguage() == "english" || LangFallback() == "english") first.push_back(unit + "/" + reqFile);

	LoadVoiceIndex();
	auto it = g_voiceDirs.find(unit);
	if (it == g_voiceDirs.end()) {
		size_t best = 3;
		for (auto d = g_voiceDirs.begin(); d != g_voiceDirs.end(); ++d) {
			size_t dist = EditDistance(unit, d->first);
			if (dist < best) {
				best = dist;
				it = d;
			}
		}
	}
	if (it == g_voiceDirs.end() || it->second.empty()) {
		return first;
	}
	auto usable = g_voiceUsable.find(it->first);
	if (usable == g_voiceUsable.end()) {
		std::vector<std::string> ok;
		std::vector<std::vector<std::string>> numericOnly;
		for (const auto &file : it->second) {
			bool numeric = file.first.size() > 4;
			for (size_t k = 0; k + 4 < file.first.size(); k++) numeric = numeric && isdigit(static_cast<unsigned char>(file.first[k]));
			if (numeric) {
				numericOnly.push_back(file.second);
				continue;
			}
			for (const std::string &p : file.second) {
				if (void *f = g_origOpen(p.c_str(), mode, err)) {
					auto destroy = reinterpret_cast<CFileDelete_t>((*reinterpret_cast<void ***>(f))[0x8 / 4]);
					destroy(f, nullptr, 1);
					ok.push_back(p);
					break;
				}
			}
		}
		if (ok.empty()) {
			for (const auto &paths : numericOnly) {
				for (const std::string &p : paths) {
					if (void *f = g_origOpen(p.c_str(), mode, err)) {
						auto destroy = reinterpret_cast<CFileDelete_t>((*reinterpret_cast<void ***>(f))[0x8 / 4]);
						destroy(f, nullptr, 1);
						ok.push_back(p);
						break;
					}
				}
			}
		}
		usable = g_voiceUsable.emplace(it->first, std::move(ok)).first;
	}
	if (usable->second.empty()) {
		return first;
	}
	first.push_back(usable->second[n > 0 ? static_cast<size_t>(n - 1) % usable->second.size() : 0]);
	return first;
}

std::string ResurrectedCredits()
{
	const std::string bar = "==================================================";
	const std::string nl = "\r\n";
	std::string t;
	t += std::string("Imperivm Resurrected ") + ModVersionText() + nl;
	t += Tr("Mod for Imperivm HD / Great Battles of Rome") + nl + nl + nl;
	t += bar + nl + nl + nl;
	t += "Imperivm Resurrected" + nl;
	t += "THIONG" + nl + nl;
	t += Tr("Modern rendering and audio, Steam multiplayer, content manager with GameBanana,") + nl;
	t += Tr("saved-games screens and much more") + nl + nl + nl;
	t += Tr("Thanks to") + nl + nl;
	t += "Syncrash - AlvaroPeyleth" + nl;
	t += "ImperivmVoicesPatch - Upercat" + nl + nl + nl;
	t += Tr("Libraries") + nl + nl;
	t += "MinHook - Tsuda Kageyu" + nl;
	t += "miniaudio - David Reid" + nl;
	t += "Steamworks SDK - Valve" + nl;
	t += nl + nl;
	t += bar + nl + nl + nl;
	return t;
}

std::vector<char> ReadAllAndClose(void *file)
{
	void **vtable = *reinterpret_cast<void ***>(file);
	auto read = reinterpret_cast<CFileRead_t>(vtable[0xc / 4]);
	auto destroy = reinterpret_cast<CFileDelete_t>(vtable[0x8 / 4]);
	std::vector<char> data;
	char buf[16384];
	for (;;) {
		unsigned got = 0;
		const char *e = read(file, nullptr, buf, sizeof(buf), &got);
		if (got) data.insert(data.end(), buf, buf + got);
		if (e || got == 0) break;
	}
	destroy(file, nullptr, 1);
	return data;
}

void *OpenMergedCredits(void *original, const char *path, unsigned mode, void *err)
{
	std::vector<char> data = ReadAllAndClose(original);
	std::string out = DiskPath("mod", "_cache/credits.txt");
	bool written = false;
	if (!data.empty()) {
		MakeDirsFor(out);
		FILE *f = nullptr;
		if (fopen_s(&f, out.c_str(), "wb") == 0 && f) {
			const std::string head = "\xEF\xBB\xBF" + ResurrectedCredits();
			const size_t skip = data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0xEF && static_cast<unsigned char>(data[1]) == 0xBB &&
				static_cast<unsigned char>(data[2]) == 0xBF ? 3 : 0;
			written = fwrite(head.data(), 1, head.size(), f) == head.size() &&
				fwrite(data.data() + skip, 1, data.size() - skip, f) == data.size() - skip;
			fclose(f);
		}
	}
	if (written) {
		if (void *merged = g_origOpen("mod/_cache/credits.txt", mode, err)) {
			LogI("vfs", "credits: Imperivm Resurrected block added (%u bytes of the game)", static_cast<unsigned>(data.size()));
			return merged;
		}
	}
	LogW("vfs", "credits: could not add the block; the game's are shown");
	return g_origOpen(path, mode, err);
}

void *OpenMergedLoc(void *original, const char *path, unsigned mode, void *err)
{
	const std::vector<std::string> patches = LangPatchFiles();
	if (patches.empty() && ModsLanguage() == "english") return original;
	std::string base;
	if (original) {
		const std::vector<char> data = ReadAllAndClose(original);
		base.assign(data.begin(), data.end());
	}
	std::string merged;
	if (LangMergeLoc(base, patches, merged)) {
		const std::string out = DiskPath("mod", "_cache/loc/game.loc.xml");
		MakeDirsFor(out);
		FILE *f = nullptr;
		bool written = false;
		if (fopen_s(&f, out.c_str(), "wb") == 0 && f) {
			written = fwrite(merged.data(), 1, merged.size(), f) == merged.size();
			fclose(f);
		}
		if (written) {
			if (void *m = g_origOpen("mod/_cache/loc/game.loc.xml", mode, err)) return m;
		}
		LogW("language", "could not write %s; only the game's texts are used", out.c_str());
	}
	return original ? g_origOpen(path, mode, err) : nullptr;
}

std::recursive_mutex g_patchLock;
std::map<std::string, std::string> g_patchedPath;

void *OpenPatched(void *original, const std::string &rel, const char *path, unsigned mode, void *err)
{
	std::lock_guard<std::recursive_mutex> lock(g_patchLock);
	auto done = g_patchedPath.find(rel);
	if (done != g_patchedPath.end()) {
		if (done->second.empty() || !original) return original;
		if (void *m = g_origOpen(done->second.c_str(), mode, err)) {
			ReadAllAndClose(original);
			return m;
		}
		return original;
	}
	if (!original) {
		PatchMissingTarget(rel);
		g_patchedPath[rel] = "";
		return original;
	}
	const std::vector<char> raw = ReadAllAndClose(original);
	std::string data(raw.begin(), raw.end());
	std::string result;
	if (PatchApply(rel, data)) {
		const std::string out = DiskPath("mod", "_cache/patched/" + rel);
		MakeDirsFor(out);
		FILE *f = nullptr;
		if (fopen_s(&f, out.c_str(), "wb") == 0 && f) {
			if (fwrite(data.data(), 1, data.size(), f) == data.size()) result = "mod/_cache/patched/" + rel;
			fclose(f);
		}
		if (result.empty()) LogW("patch", "could not write %s; the game's file is used", out.c_str());
	}
	g_patchedPath[rel] = result;
	if (!result.empty()) {
		if (void *m = g_origOpen(result.c_str(), mode, err)) return m;
	}
	return g_origOpen(path, mode, err);
}

void *__cdecl HookVfsOpen(const char *path, unsigned mode, void *err)
{
	if (!path) {
		return g_origOpen(path, mode, err);
	}
	const std::string rel = Normalize(path);
	const bool reading = (mode & 2) == 0;

	if (reading && GetConfig().modOverrides && DiskFileExists(DiskPath("mod", rel))) {
		std::string alt = "mod/" + rel;
		void *f = g_origOpen(alt.c_str(), mode, err);
		std::lock_guard<std::mutex> lock(g_lock);
		if (g_reportedOverrides.insert(rel).second) {
			LogAt(f ? kLogDebug : kLogWarn, "content", "mod file %s -> %s", rel.c_str(), f ? "replaced by mod\\" : "could NOT open the mod\\ version");
		}
		if (f) {
			if (rel == "currentlang/game.loc.xml") f = OpenMergedLoc(f, alt.c_str(), mode, err);
			if (PatchHas(rel)) f = OpenPatched(f, rel, alt.c_str(), mode, err);
			return f;
		}
	}

	void *f = nullptr;
	std::string opened = path;
	if (reading && rel.compare(0, 12, "currentlang/") == 0) {
		const std::string alt = LangLooseFile(rel);
		if (!alt.empty() && (f = g_origOpen(alt.c_str(), mode, err)) != nullptr) opened = alt;
	}
	if (!f) f = g_origOpen(path, mode, err);
	if (!f && reading) {
		std::string builtin;
		if (BuiltinFilePath(rel, builtin)) f = g_origOpen(builtin.c_str(), mode, err);
	}
	if (!f && reading && rel.compare(0, 5, "mods/") != 0) {
		for (const std::string &dir : ModsMountedDirs()) {
			std::string disk = dir + "\\" + rel;
			std::replace(disk.begin(), disk.end(), '/', '\\');
			if (!DiskFileExists(disk)) continue;
			const size_t slash = dir.find_last_of("\\/");
			const std::string alt = "mods/" + (slash == std::string::npos ? dir : dir.substr(slash + 1)) + "/" + rel;
			if ((f = g_origOpen(alt.c_str(), mode, err)) != nullptr) {
				opened = alt;
				std::lock_guard<std::mutex> lock(g_lock);
				if (g_reportedOverrides.insert(alt).second) LogW("content", "mod: %s served by its folder (the loose-files layer did not find it)", alt.c_str());
				break;
			}
		}
	}
	if (f && reading && rel == "currentlang/credits.txt") f = OpenMergedCredits(f, opened.c_str(), mode, err);
	if (reading && rel == "currentlang/game.loc.xml") f = OpenMergedLoc(f, opened.c_str(), mode, err);
	if (reading && PatchHas(rel)) f = OpenPatched(f, rel, opened.c_str(), mode, err);
	if (GetConfig().logFiles) {
		LogD("vfs", "opens %s (mode %x)%s", rel.c_str(), mode, f ? "" : " -> does not exist");
	}
	if (!f && reading && rel.size() > 4 &&
		(rel.compare(rel.size() - 4, 4, ".ogg") == 0 || rel.compare(rel.size() - 4, 4, ".wav") == 0)) {
		std::lock_guard<std::mutex> lock(g_lock);
		std::vector<std::string> alts;
		if (GetConfig().fixVoices) {
			alts = ResolveMissingVoice(rel, mode, err);
		}
		if (rel.size() > 6 && isdigit(static_cast<unsigned char>(rel[rel.size() - 5])) &&
			isdigit(static_cast<unsigned char>(rel[rel.size() - 6]))) {
			for (int n = 1; n <= 9; n++) {
				std::string v = rel;
				v[v.size() - 6] = '0';
				v[v.size() - 5] = static_cast<char>('0' + n);
				if (v != rel) alts.push_back(v);
			}
		}
		std::string used;
		for (const std::string &alt : alts) {
			f = g_origOpen(alt.c_str(), mode, err);
			if (f) {
				used = alt;
				break;
			}
		}
		if (g_missingSounds.insert(rel).second) {
			if (f) {
				LogW("audio", "sound: %s does not exist -> using %s", rel.c_str(), used.c_str());
			}
			else {
				LogW("vfs", "sound NOT found: %s%s%s", rel.c_str(), alts.empty() ? "" : " (tested:",
					alts.empty() ? "" : (alts[0] + ")").c_str());
			}
		}
	}
	const bool saveFile = rel.size() > 4 && (rel.compare(rel.size() - 4, 4, ".snr") == 0 ||
		rel.compare(rel.size() - 4, 4, ".adv") == 0 || rel.compare(rel.size() - 4, 4, ".con") == 0);
	if (f && !reading && rel.size() > 15 && rel.compare(0, 10, "scenarios/") == 0 && rel.compare(rel.size() - 5, 5, ".bfhp") == 0) {
		ModsOnMapWritten(rel);
	}
	if (f && saveFile && rel.compare(0, 6, "packs/") != 0 && rel.compare(0, 12, "currentgame/") != 0) {
		if (!reading) {
			ModsOnSaveWritten(rel, GameIsMultiplayer());
		}
		else {
			SaveSessionOnLoad(rel);
			ModVarsPendingFromSave(rel);
			static std::map<std::string, DWORD> warned;
			const std::string warning = SaveSessionLoadsIgnored() ? "" : ModsCheckSaveOnLoad(rel);
			std::lock_guard<std::mutex> lock(g_lock);
			if (!warning.empty() && GetTickCount() - warned[rel] > 30000) {
				warned[rel] = GetTickCount();
				LogW("vfs", "saved game %s: %s", rel.c_str(), warning.c_str());
			}
		}
	}
	if (f && reading && ShouldDump(rel)) {
		bool first;
		{
			std::lock_guard<std::mutex> lock(g_lock);
			first = g_dumped.insert(rel).second;
		}
		if (first) {
			DumpAndClose(f, rel);
			f = g_origOpen(opened.c_str(), mode, err);
			if (f && rel == "currentlang/credits.txt") f = OpenMergedCredits(f, opened.c_str(), mode, err);
			if (rel == "currentlang/game.loc.xml") f = OpenMergedLoc(f, opened.c_str(), mode, err);
			if (PatchHas(rel)) f = OpenPatched(f, rel, opened.c_str(), mode, err);
		}
	}
	return f;
}

}

void InstallVfsHooks(bool knownExe)
{
	if (!knownExe) {
		return;
	}
	char exe[MAX_PATH];
	GetModuleFileNameA(nullptr, exe, MAX_PATH);
	g_gameDir = exe;
	g_gameDir.erase(g_gameDir.find_last_of('\\'));

	if (HookCreate(reinterpret_cast<void *>(addr::kVfsOpenCompressedAware), reinterpret_cast<void *>(&HookVfsOpen),
			reinterpret_cast<void **>(&g_origOpen)) == MH_OK &&
		HookEnable(reinterpret_cast<void *>(addr::kVfsOpenCompressedAware)) == MH_OK) {
		LogD("vfs", "hooked (replacement from mod\\ %s, dump: %s)", GetConfig().modOverrides ? "active" : "disabled",
			GetConfig().dumpFiles.empty() ? "-" : GetConfig().dumpFiles.c_str());
	}
	else {
		LogE("vfs", "could not hook VfsOpenCompressedAware");
	}
}

void VfsDumpAtStartup()
{
	const std::string &list = GetConfig().dumpAtStartup;
	size_t start = 0;
	unsigned done = 0;
	while (start < list.size()) {
		size_t end = list.find(',', start);
		std::string item = list.substr(start, end == std::string::npos ? std::string::npos : end - start);
		item.erase(0, item.find_first_not_of(' '));
		item.erase(item.find_last_not_of(' ') + 1);
		start = end == std::string::npos ? list.size() : end + 1;
		if (item.empty()) continue;
		std::vector<char> data;
		const std::string rel = Normalize(item.c_str());
		if (!ReadGameFile(item.c_str(), 1, data)) {
			LogW("vfs", "dump at startup: %s does not exist", rel.c_str());
			continue;
		}
		const std::string out = DiskPath("mod-dump", rel);
		MakeDirsFor(out);
		FILE *f = nullptr;
		if (fopen_s(&f, out.c_str(), "wb") == 0 && f) {
			fwrite(data.data(), 1, data.size(), f);
			fclose(f);
			done++;
		}
	}
	if (done) LogD("vfs", "dumps at startup %u files in mod-dump\\", done);
}

namespace {

const bool g_dumpRegistered = (TweaksOnConfigLoaded(&VfsDumpAtStartup), true);

}
