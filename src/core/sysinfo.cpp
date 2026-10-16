// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>

#include "core/sysinfo.h"

#include <cstdio>
#include <ctime>

namespace {

std::string RegistryText(HKEY root, const char *key, const char *name)
{
	HKEY h = nullptr;
	if (RegOpenKeyExA(root, key, 0, KEY_READ | KEY_WOW64_64KEY, &h) != ERROR_SUCCESS) return "";
	char text[256] = {};
	DWORD size = sizeof(text) - 1, type = 0;
	if (RegQueryValueExA(h, name, nullptr, &type, reinterpret_cast<BYTE *>(text), &size) != ERROR_SUCCESS || type != REG_SZ) text[0] = 0;
	RegCloseKey(h);
	return text;
}

std::string Squeeze(const std::string &s)
{
	std::string out;
	bool space = false;
	for (char c : s) {
		if (c == ' ' || c == '\t') {
			space = !out.empty();
			continue;
		}
		if (space) out += ' ';
		space = false;
		out += c;
	}
	return out;
}

struct MonitorList {
	std::vector<std::string> lines;
};

BOOL CALLBACK CollectMonitor(HMONITOR monitor, HDC, LPRECT, LPARAM param)
{
	MonitorList &list = *reinterpret_cast<MonitorList *>(param);
	MONITORINFOEXA info = {};
	info.cbSize = sizeof(info);
	if (!GetMonitorInfoA(monitor, &info)) return TRUE;
	DEVMODEA mode = {};
	mode.dmSize = sizeof(mode);
	char text[160];
	if (EnumDisplaySettingsA(info.szDevice, ENUM_CURRENT_SETTINGS, &mode)) {
		sprintf_s(text, "monitor %u%s: %lux%lu at %lu Hz", static_cast<unsigned>(list.lines.size() + 1), (info.dwFlags & MONITORINFOF_PRIMARY) ? " (primary)" : "",
			mode.dmPelsWidth, mode.dmPelsHeight, mode.dmDisplayFrequency);
	}
	else {
		sprintf_s(text, "monitor %u%s: %ldx%ld", static_cast<unsigned>(list.lines.size() + 1), (info.dwFlags & MONITORINFOF_PRIMARY) ? " (primary)" : "",
			info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);
	}
	list.lines.push_back(text);
	return TRUE;
}

}

std::string DescribeWindows()
{
	typedef LONG(WINAPI * RtlGetVersion_t)(OSVERSIONINFOW *);
	HMODULE ntdll = GetModuleHandleA("ntdll.dll");
	OSVERSIONINFOW v = {};
	v.dwOSVersionInfoSize = sizeof(v);
	if (ntdll) {
		const RtlGetVersion_t fn = reinterpret_cast<RtlGetVersion_t>(GetProcAddress(ntdll, "RtlGetVersion"));
		if (fn) fn(&v);
	}
	SYSTEM_INFO native;
	GetNativeSystemInfo(&native);
	const char *arch = native.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x64" : native.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64 ? "ARM64"
		: native.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ? "x86" : "unknown architecture";
	const char *name = v.dwMajorVersion == 10 && v.dwBuildNumber >= 22000 ? "Windows 11" : v.dwMajorVersion == 10 ? "Windows 10" : "Windows";
	const std::string display = RegistryText(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "DisplayVersion");
	char text[200];
	sprintf_s(text, "%s%s%s (%lu.%lu.%lu), %s", name, display.empty() ? "" : " ", display.c_str(), v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber, arch);
	std::string out = text;
	if (ntdll) {
		typedef const char *(CDECL * WineVersion_t)();
		const WineVersion_t wine = reinterpret_cast<WineVersion_t>(GetProcAddress(ntdll, "wine_get_version"));
		if (wine) out += std::string("; running under Wine ") + wine();
	}
	return out;
}

std::string DescribeCpu()
{
	std::string name = Squeeze(RegistryText(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString"));
	if (name.empty()) name = "unknown CPU";
	SYSTEM_INFO si;
	GetNativeSystemInfo(&si);
	char text[32];
	sprintf_s(text, " (%lu threads)", si.dwNumberOfProcessors);
	return name + text;
}

std::string DescribeMemory()
{
	MEMORYSTATUSEX m = {};
	m.dwLength = sizeof(m);
	if (!GlobalMemoryStatusEx(&m)) return "unknown memory";
	char text[96];
	sprintf_s(text, "%.1f GB of memory (%.1f GB free)", m.ullTotalPhys / 1073741824.0, m.ullAvailPhys / 1073741824.0);
	return text;
}

std::string DescribeLanguage()
{
	wchar_t wide[LOCALE_NAME_MAX_LENGTH] = {};
	if (!GetUserDefaultLocaleName(wide, LOCALE_NAME_MAX_LENGTH)) return "unknown";
	char text[LOCALE_NAME_MAX_LENGTH] = {};
	WideCharToMultiByte(CP_ACP, 0, wide, -1, text, sizeof(text), nullptr, nullptr);
	return text;
}

std::vector<std::string> DescribeMonitors()
{
	MonitorList list;
	EnumDisplayMonitors(nullptr, nullptr, CollectMonitor, reinterpret_cast<LPARAM>(&list));
	return list.lines;
}

unsigned BuildStamp()
{
	HMODULE self = nullptr;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(&BuildStamp), &self) || !self) return 0;
	const IMAGE_DOS_HEADER *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(self);
	if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
	const IMAGE_NT_HEADERS *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(reinterpret_cast<const char *>(self) + dos->e_lfanew);
	return nt->Signature == IMAGE_NT_SIGNATURE ? nt->FileHeader.TimeDateStamp : 0;
}

std::string BuildStampText(unsigned stamp)
{
	if (!stamp) return "unknown";
	const __time64_t t = stamp;
	tm utc = {};
	if (_gmtime64_s(&utc, &t) != 0) return "unknown";
	char text[48];
	sprintf_s(text, "%08x, built %04d-%02d-%02d %02d:%02d UTC", stamp, utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday, utc.tm_hour, utc.tm_min);
	return text;
}
