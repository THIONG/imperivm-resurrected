// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>

#include <atomic>
#include <mutex>
#include <set>
#include <string>

#include "core/guard.h"
#include "core/log.h"
#include "mods/mods_internal.h"

using namespace mods_impl;

namespace {
std::mutex g_watchLock;
HANDLE g_watchStop = nullptr;
bool g_watching = false;
std::atomic<uint64_t> g_serial{0};
std::string g_scannedSignature;
std::set<std::string> g_startFolders;
bool g_haveStart = false;

void AddTree(const std::string &dir, uint64_t &bytes, unsigned &files)
{
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((dir + "\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return;
	do {
		const std::string name = fd.cFileName;
		if (name == "." || name == "..") continue;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			AddTree(dir + "\\" + name, bytes, files);
			continue;
		}
		if (_stricmp(name.c_str(), ".fingerprint") == 0) continue;
		bytes += static_cast<uint64_t>(fd.nFileSizeHigh) << 32 | fd.nFileSizeLow;
		files++;
	} while (FindNextFileA(h, &fd));
	FindClose(h);
}

std::string FolderSignature()
{
	std::string out;
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((ModsDir() + "\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return out;
	do {
		const std::string name = fd.cFileName;
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || name[0] == '.' || name[0] == '_') continue;
		uint64_t bytes = 0;
		unsigned files = 0;
		AddTree(ModsDir() + "\\" + name, bytes, files);
		out += name + "|" + std::to_string(bytes) + "|" + std::to_string(files) + "\n";
	} while (FindNextFileA(h, &fd));
	FindClose(h);
	return out;
}

void WatchLoop(HANDLE stop)
{
	HANDLE change = FindFirstChangeNotificationA(ModsDir().c_str(), TRUE,
		FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE);
	std::string previous;
	bool dirty = true;
	for (;;) {
		HANDLE waits[2] = {stop, change};
		const DWORD r = WaitForMultipleObjects(change != INVALID_HANDLE_VALUE ? 2 : 1, waits, FALSE, dirty ? 1000 : INFINITE);
		if (r == WAIT_OBJECT_0) break;
		if (r == WAIT_OBJECT_0 + 1) {
			FindNextChangeNotification(change);
			dirty = true;
			continue;
		}
		if (r != WAIT_TIMEOUT) break;
		const std::string now = FolderSignature();
		std::string scanned;
		{
			std::lock_guard<std::mutex> lock(g_watchLock);
			scanned = g_scannedSignature;
		}
		if (now == scanned) {
			dirty = false;
			previous.clear();
		}
		else if (now == previous) {
			LogI("content", "the mods folder changed: scanning it again");
			ModsScan();
			dirty = false;
			previous.clear();
		}
		else {
			previous = now;
		}
	}
	if (change != INVALID_HANDLE_VALUE) FindCloseChangeNotification(change);
}
}

namespace mods_impl {
void ModsNoteScan(std::vector<ModInfo> &mods)
{
	const std::string signature = FolderSignature();
	std::lock_guard<std::mutex> lock(g_watchLock);
	g_scannedSignature = signature;
	if (!g_haveStart) {
		for (const ModInfo &m : mods) g_startFolders.insert(m.folder);
		g_haveStart = true;
	}
	for (ModInfo &m : mods) m.newSinceStart = g_startFolders.count(m.folder) == 0;
}

void ModsBumpSerial()
{
	g_serial++;
}
}

uint64_t ModsListSerial()
{
	return g_serial;
}

void ModsWatchStart()
{
	std::lock_guard<std::mutex> lock(g_watchLock);
	if (g_watching) return;
	g_watchStop = CreateEventA(nullptr, TRUE, FALSE, nullptr);
	if (!g_watchStop) return;
	g_watching = true;
	HANDLE stop = g_watchStop;
	SafeThread([stop] {
		WatchLoop(stop);
		CloseHandle(stop);
	}).detach();
}

void ModsWatchStop()
{
	std::lock_guard<std::mutex> lock(g_watchLock);
	if (!g_watching) return;
	g_watching = false;
	SetEvent(g_watchStop);
	g_watchStop = nullptr;
}
