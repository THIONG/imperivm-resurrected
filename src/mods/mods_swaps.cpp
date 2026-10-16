// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>
#include <fstream>
#include <algorithm>
#include "MinHook.h"
#include "i18n/tr.h"
#include "core/log.h"

#include "mods/mods_internal.h"

using namespace mods_impl;

namespace mods_impl {
namespace {
std::vector<std::string> PendingUninstalls()
{
	char buf[4096] = {};
	GetPrivateProfileStringA("Mods", "UninstallOnStart", "", buf, sizeof(buf), IniPath().c_str());
	std::vector<std::string> out;
	std::string s = buf;
	for (size_t p = 0; p < s.size();) {
		const size_t q = s.find('|', p);
		const std::string f = s.substr(p, q == std::string::npos ? std::string::npos : q - p);
		if (!f.empty()) out.push_back(f);
		p = q == std::string::npos ? s.size() : q + 1;
	}
	return out;
}

void SetPendingUninstalls(const std::vector<std::string> &list)
{
	std::string s;
	for (const std::string &f : list) s += (s.empty() ? "" : "|") + f;
	WritePrivateProfileStringA("Mods", "UninstallOnStart", s.empty() ? nullptr : s.c_str(), IniPath().c_str());
}

bool RemoveFolder(const std::string &path)
{
	std::string from = path;
	from.push_back('\0');
	SHFILEOPSTRUCTA op = {};
	op.wFunc = FO_DELETE;
	op.pFrom = from.c_str();
	op.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
	return SHFileOperationA(&op) == 0 && !op.fAnyOperationsAborted;
}
}
}

int ModsCompareVersions(const std::string &a, const std::string &b)
{
	return CompareVersions(a, b);
}

namespace mods_impl {
namespace {
std::string SwapFile() { return ModsDir() + "\\_downloads\\swaps.txt"; }
}
}

void ModsQueueSwap(const std::string &newFolder, const std::string &oldFolder)
{
	CreateDirectoryA((ModsDir() + "\\_downloads").c_str(), nullptr);
	FILE *f = nullptr;
	if (fopen_s(&f, SwapFile().c_str(), "ab") == 0 && f) {
		fprintf(f, "%s\t%s\n", newFolder.c_str(), oldFolder.c_str());
		fclose(f);
	}
}

void ModsApplyPendingSwaps()
{
	FILE *f = nullptr;
	if (fopen_s(&f, SwapFile().c_str(), "rb") != 0 || !f) return;
	std::string text;
	char buf[512];
	size_t n;
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0) text.append(buf, n);
	fclose(f);
	DeleteFileA(SwapFile().c_str());
	size_t pos = 0;
	while (pos < text.size()) {
		size_t end = text.find('\n', pos);
		if (end == std::string::npos) end = text.size();
		const std::string line = text.substr(pos, end - pos);
		pos = end + 1;
		const size_t tab = line.find('\t');
		if (tab == std::string::npos) continue;
		const std::string nw = line.substr(0, tab), old = line.substr(tab + 1);
		if (nw.empty() || old.empty() || nw == old || nw[0] == '_' || old[0] == '_' || nw[0] == '.' || old[0] == '.' ||
			nw.find_first_of("\\/") != std::string::npos || old.find_first_of("\\/") != std::string::npos) {
			continue;
		}
		const std::string nwPath = ModsDir() + "\\" + nw, oldPath = ModsDir() + "\\" + old;
		if (GetFileAttributesA(nwPath.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
		if (GetFileAttributesA(oldPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
			SYSTEMTIME t;
			GetLocalTime(&t);
			char stamp[32];
			sprintf_s(stamp, "%04d%02d%02d-%02d%02d%02d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
			CreateDirectoryA((ModsDir() + "\\_downloads").c_str(), nullptr);
			CreateDirectoryA((ModsDir() + "\\_downloads\\backup").c_str(), nullptr);
			const std::string backup = ModsDir() + "\\_downloads\\backup\\" + old + "-" + stamp;
			if (!MoveFileExA(oldPath.c_str(), backup.c_str(), MOVEFILE_COPY_ALLOWED)) {
				LogW("content", "could not save the previous version of %s (error %lu); the room's copy is kept", old.c_str(), GetLastError());
				continue;
			}
		}
		if (!MoveFileExA(nwPath.c_str(), oldPath.c_str(), MOVEFILE_COPY_ALLOWED)) {
			LogW("content", "could not put %s in place of %s (error %lu)", nw.c_str(), old.c_str(), GetLastError());
			continue;
		}
		for (const std::string &pn : ModsProfileNames()) {
			ModProfile pr = ModsLoadProfile(pn);
			bool changed = false;
			std::vector<std::string> out;
			for (const std::string &m : pr.mods) {
				const bool isNew = _stricmp(m.c_str(), nw.c_str()) == 0;
				if (isNew) changed = true;
				const std::string &name = isNew ? old : m;
				if (std::find_if(out.begin(), out.end(), [&](const std::string &o) { return _stricmp(o.c_str(), name.c_str()) == 0; }) == out.end()) {
					out.push_back(name);
				}
			}
			if (changed) {
				pr.mods = out;
				ModsSaveProfile(pr);
			}
		}
		LogI("content", "%s takes the place of %s (the previous version is in mods\\_downloads\\backup)", nw.c_str(), old.c_str());
	}
}

void ModsApplyPendingUninstalls()
{
	std::vector<std::string> left;
	for (const std::string &folder : PendingUninstalls()) {
		const std::string path = ModsDir() + "\\" + folder;
		if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
		if (RemoveFolder(path)) LogI("content", "%s uninstalled (deleted from disk)", folder.c_str());
		else left.push_back(folder);
	}
	SetPendingUninstalls(left);
}

namespace mods_impl {
namespace {
std::string ReceivedListPath() { return ModsDir() + "\\_received.txt"; }

std::vector<std::string> ReadReceived()
{
	std::vector<std::string> out;
	std::ifstream in(ReceivedListPath());
	std::string line;
	while (std::getline(in, line)) {
		while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
		if (!line.empty()) out.push_back(line);
	}
	return out;
}

void WriteReceived(const std::vector<std::string> &list)
{
	std::ofstream out(ReceivedListPath(), std::ios::trunc);
	for (const std::string &l : list) out << l << "\n";
}

std::string FileNameOf(const std::string &path)
{
	const size_t slash = path.find_last_of("\\/");
	return slash == std::string::npos ? path : path.substr(slash + 1);
}

void CollectMapNames(const std::string &dir, std::vector<std::string> &names)
{
	WIN32_FIND_DATAA fd;
	HANDLE f = FindFirstFileA((dir + "\\*").c_str(), &fd);
	if (f == INVALID_HANDLE_VALUE) return;
	do {
		const std::string n = fd.cFileName;
		if (n == "." || n == "..") continue;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) CollectMapNames(dir + "\\" + n, names);
		else if (n.size() > 5 && _stricmp(n.c_str() + n.size() - 5, ".bfhp") == 0) names.push_back(n);
	} while (FindNextFileA(f, &fd));
	FindClose(f);
}

void RemoveReceivedMaps(const std::vector<std::string> &names)
{
	if (names.empty()) return;
	std::vector<std::string> keep;
	for (const std::string &path : ReadReceived()) {
		const std::string file = FileNameOf(path);
		bool match = false;
		for (const std::string &n : names) match = match || _stricmp(n.c_str(), file.c_str()) == 0;
		if (!match) {
			keep.push_back(path);
			continue;
		}
		if (DeleteFileA(path.c_str()) || GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
			DeleteFileA((path + ".mods.json").c_str());
			LogI("content", "also deleted the copy of the map a host passed (%s)", path.c_str());
		}
		else {
			keep.push_back(path);
		}
	}
	WriteReceived(keep);
}
}
}

void ModsNoteReceivedMap(const std::string &fullPath)
{
	std::vector<std::string> list = ReadReceived();
	for (const std::string &l : list) {
		if (_stricmp(l.c_str(), fullPath.c_str()) == 0) return;
	}
	list.push_back(fullPath);
	WriteReceived(list);
}

std::string ModsUninstall(const std::string &folder)
{
	if (folder.empty() || folder.find_first_of("\\/") != std::string::npos || folder[0] == '_' || folder[0] == '.') {
		return "That cannot be uninstalled";
	}
	ModInfo m;
	const std::string name = ModsFind(folder, m) ? m.name : folder;
	for (const std::string &n : ModsProfilesUsing(folder)) {
		ModProfile p = ModsLoadProfile(n);
		p.mods.erase(std::remove_if(p.mods.begin(), p.mods.end(),
						 [&](const std::string &f) { return _stricmp(f.c_str(), folder.c_str()) == 0; }),
			p.mods.end());
		ModsSaveProfile(p);
	}
	bool mounted = false;
	for (const std::string &f : ModsMountedProfile().mods) {
		if (_stricmp(f.c_str(), folder.c_str()) == 0) mounted = true;
	}
	const std::string path = ModsDir() + "\\" + folder;
	{
		std::vector<std::string> maps;
		CollectMapNames(path, maps);
		RemoveReceivedMaps(maps);
	}
	if (!mounted && RemoveFolder(path)) {
		LogI("content", "%s uninstalled (deleted from disk)", folder.c_str());
		ModsScan();
		return name + Tr(" uninstalled");
	}
	std::vector<std::string> pending = PendingUninstalls();
	if (std::find(pending.begin(), pending.end(), folder) == pending.end()) pending.push_back(folder);
	SetPendingUninstalls(pending);
	LogI("content", "%s will be uninstalled on restart%s", folder.c_str(), mounted ? " (it is loaded)" : " (could not delete now)");
	return name + Tr(" will be uninstalled when the game restarts");
}

bool ModsUninstallPending(const std::string &folder)
{
	const std::vector<std::string> pending = PendingUninstalls();
	return std::find(pending.begin(), pending.end(), folder) != pending.end();
}
