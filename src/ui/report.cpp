// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>

#include <atomic>
#include <string>

#include "core/guard.h"
#include "core/log.h"
#include "core/logfiles.h"
#include "core/logreport.h"
#include "core/sysinfo.h"
#include "core/version.h"
#include "i18n/tr.h"
#include "mods/mods.h"
#include "net/steamnet.h"
#include "ui/modui.h"
#include "ui/report.h"

namespace {

std::atomic<bool> g_busy{false};

HWND OwnForegroundWindow()
{
	HWND h = GetForegroundWindow();
	DWORD pid = 0;
	GetWindowThreadProcessId(h, &pid);
	return pid == GetCurrentProcessId() ? h : nullptr;
}

void MinimizeFullscreenGame()
{
	HWND game = OwnForegroundWindow();
	if (game && !(GetWindowLongA(game, GWL_STYLE) & WS_CAPTION)) PostMessageA(game, WM_SYSCOMMAND, SC_MINIMIZE, 0);
}

void ShowInExplorer(const std::string &path, bool select)
{
	MinimizeFullscreenGame();
	const HRESULT co = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (select) ShellExecuteA(nullptr, "open", "explorer.exe", ("/select,\"" + path + "\"").c_str(), nullptr, SW_SHOWNORMAL);
	else ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	if (SUCCEEDED(co)) CoUninitialize();
}

std::string FileName(const std::string &path)
{
	const size_t slash = path.find_last_of('\\');
	return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string BuildInfo()
{
	std::string text;
	text += "Mod version: " + std::string(ModVersionText()) + "\r\n";
	text += "Build: " + BuildStampText(BuildStamp()) + "\r\n";
	text += "Windows: " + DescribeWindows() + "\r\n";
	text += "CPU: " + DescribeCpu() + "\r\n";
	text += "Memory: " + DescribeMemory() + "\r\n";
	text += "Language: " + DescribeLanguage() + "\r\n";
	for (const std::string &monitor : DescribeMonitors()) text += "Display: " + monitor + "\r\n";
	const ModProfile profile = ModsMountedProfile();
	text += "Content profile: " + profile.name + "\r\n";
	text += "Rules fingerprint: " + ModsProfileGameplayHash(profile) + "\r\n";
	text += "Active content: " + std::to_string(profile.mods.size()) + "\r\n";
	for (const std::string &folder : profile.mods) {
		ModInfo m;
		if (!ModsFind(folder, m)) {
			text += "  " + folder + " (not found)\r\n";
			continue;
		}
		text += "  " + m.name + " [" + m.id + "] " + m.version + ", " + ModsKindId(m.kind) + ", " + std::to_string(m.fileCount) + " files\r\n";
	}
	return text;
}

void Run()
{
	LogDumpContext("report");
	ReportInput input;
	input.currentLog = LogFilePath();
	input.info = BuildInfo();
	input.ownName = SteamOnlinePlayerName();
	input.otherNames = SteamSeenNames();
	const ReportResult result = ReportCreate(MakeLogPaths(LogGameDir()), input);
	if (!result.ok) {
		LogE("report", "could not create the report: %s", result.error.c_str());
		ModUiToast(Tr("The report could not be created."), true);
		return;
	}
	if (result.zipped) ModUiToast(TrFmt("Report created: %s. Attach it when you tell the author about the problem.", {FileName(result.path)}));
	else ModUiToast(Tr("The report could not be compressed; its folder was opened instead."));
	ShowInExplorer(result.path, result.zipped);
}

}

void ReportOpenLogsFolder()
{
	const std::string folder = MakeLogPaths(LogGameDir()).modDir;
	SafeThread([folder] { ShowInExplorer(folder, false); }).detach();
}

void ReportStart()
{
	if (g_busy.exchange(true)) return;
	ModUiToast(Tr("Creating the report..."));
	SafeThread([] {
		Guard("report", Run);
		g_busy = false;
	}).detach();
}

bool ReportBusy()
{
	return g_busy;
}
