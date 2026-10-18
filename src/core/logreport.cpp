// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "core/logreport.h"

#include <windows.h>

#include <algorithm>
#include <cstdio>

#include "core/log.h"
#include "core/logredact.h"
#include "core/zipfile.h"

namespace {

const size_t kLogCap = 24u << 20;
const size_t kSideFileCap = 2u << 20;
const size_t kTraceCap = 64u << 20;
const size_t kDesyncCap = 16u << 20;
const size_t kCompilerOutputCap = 128u << 10;
const size_t kSettingsCap = 256u << 10;
const unsigned long long kDay = 86400ull * 10000000ull;
const unsigned long long kCrashMaxAge = 7 * kDay;
const unsigned long long kDesyncMaxAge = 7 * kDay;
const size_t kRejoinFiles = 4;
const char kReportPrefix[] = "imperivm-report-";
const char kPrivacyNote[] =
	"\r\nPrivacy\r\n"
	"This report was made on this PC and was not sent anywhere. Steam IDs, IP addresses, the Windows user name and the\r\n"
	"player names the game knows were replaced with labels such as <steamid1>, <ip1> and <you>. Names of players from\r\n"
	"earlier sessions can still appear: read the files before sharing them.\r\n";

struct Entry {
	std::string name;
	unsigned long long time = 0;
	bool directory = false;
	bool link = false;
};

unsigned long long FileTimeValue(const FILETIME &t)
{
	return (static_cast<unsigned long long>(t.dwHighDateTime) << 32) | t.dwLowDateTime;
}

unsigned long long NowFileTime()
{
	FILETIME t;
	GetSystemTimeAsFileTime(&t);
	return FileTimeValue(t);
}

std::vector<Entry> ListFolder(const std::string &dir)
{
	std::vector<Entry> out;
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((dir + "\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return out;
	do {
		Entry e;
		e.name = fd.cFileName;
		if (e.name == "." || e.name == "..") continue;
		e.time = FileTimeValue(fd.ftLastWriteTime);
		e.link = (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
		e.directory = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && !e.link;
		out.push_back(e);
	} while (FindNextFileA(h, &fd));
	FindClose(h);
	return out;
}

void RemoveTree(const std::string &path)
{
	for (const Entry &e : ListFolder(path)) {
		const std::string child = path + "\\" + e.name;
		if (e.directory) {
			RemoveTree(child);
		}
		else {
			SetFileAttributesA(child.c_str(), FILE_ATTRIBUTE_NORMAL);
			if (!DeleteFileA(child.c_str()) && e.link) RemoveDirectoryA(child.c_str());
		}
	}
	RemoveDirectoryA(path.c_str());
}

void EnsureFolder(const std::string &dir)
{
	for (size_t i = dir.find('\\', 3); i != std::string::npos; i = dir.find('\\', i + 1)) CreateDirectoryA(dir.substr(0, i).c_str(), nullptr);
	CreateDirectoryA(dir.c_str(), nullptr);
}

bool WriteAll(const std::string &path, const std::string &data)
{
	const size_t slash = path.find_last_of('\\');
	if (slash != std::string::npos) EnsureFolder(path.substr(0, slash));
	HANDLE f = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (f == INVALID_HANDLE_VALUE) return false;
	size_t done = 0;
	bool ok = true;
	while (ok && done < data.size()) {
		DWORD wrote = 0;
		const DWORD chunk = static_cast<DWORD>(std::min<size_t>(data.size() - done, 1u << 20));
		ok = WriteFile(f, data.data() + done, chunk, &wrote, nullptr) && wrote > 0;
		done += wrote;
	}
	CloseHandle(f);
	return ok;
}

bool ReadCapped(const std::string &path, size_t cap, bool keepTail, std::string &out)
{
	out.clear();
	HANDLE f = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (f == INVALID_HANDLE_VALUE) return false;
	LARGE_INTEGER size = {};
	GetFileSizeEx(f, &size);
	const unsigned long long total = static_cast<unsigned long long>(size.QuadPart);
	unsigned long long left = 0;
	if (keepTail && total > cap) {
		LARGE_INTEGER at;
		at.QuadPart = static_cast<LONGLONG>(total - cap);
		SetFilePointerEx(f, at, nullptr, FILE_BEGIN);
		left = total - cap;
	}
	char buffer[65536];
	DWORD got = 0;
	while (out.size() < cap && ReadFile(f, buffer, static_cast<DWORD>(std::min<size_t>(sizeof(buffer), cap - out.size())), &got, nullptr) && got) out.append(buffer, got);
	CloseHandle(f);
	if (left) out = "[the first " + std::to_string(left) + " bytes of this file were left out]\r\n" + out;
	else if (!keepTail && total > cap) out += "\r\n[the rest of this file was left out: " + std::to_string(total - cap) + " bytes]\r\n";
	return true;
}

std::string BaseName(const std::string &path)
{
	const size_t slash = path.find_last_of("\\/");
	return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string Stamp()
{
	SYSTEMTIME s;
	GetLocalTime(&s);
	char buffer[32];
	sprintf_s(buffer, "%04d%02d%02d-%02d%02d%02d", s.wYear, s.wMonth, s.wDay, s.wHour, s.wMinute, s.wSecond);
	return buffer;
}

std::string LocalTime(unsigned long long fileTime)
{
	FILETIME utc, local;
	utc.dwLowDateTime = static_cast<DWORD>(fileTime);
	utc.dwHighDateTime = static_cast<DWORD>(fileTime >> 32);
	SYSTEMTIME s;
	if (!FileTimeToLocalFileTime(&utc, &local) || !FileTimeToSystemTime(&local, &s)) return std::string();
	char buffer[32];
	sprintf_s(buffer, "%04d-%02d-%02d %02d:%02d", s.wYear, s.wMonth, s.wDay, s.wHour, s.wMinute);
	return buffer;
}

struct Found {
	std::string path;
	unsigned long long time = 0;
};

Found NewestUnderLogs(const std::string &logsDir, const char *file, unsigned long long maxAge)
{
	Found best;
	const unsigned long long now = NowFileTime();
	for (const Entry &e : ListFolder(logsDir)) {
		if (!e.directory || e.name == "imperivm") continue;
		const std::string path = logsDir + "\\" + e.name + "\\" + file;
		WIN32_FILE_ATTRIBUTE_DATA a;
		if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &a)) continue;
		const unsigned long long t = FileTimeValue(a.ftLastWriteTime);
		if (t + maxAge < now || t <= best.time) continue;
		best.path = path;
		best.time = t;
	}
	return best;
}

bool ZipFolder(const std::string &folder, const std::string &zip)
{
	std::vector<std::string> names;
	for (const Entry &e : ListFolder(folder)) names.push_back(e.name);
	return !names.empty() && ZipCreate(zip, folder, names);
}

void PruneReports(const std::string &reports, size_t keep)
{
	std::vector<Entry> mine;
	for (const Entry &e : ListFolder(reports)) {
		if (e.name.compare(0, sizeof(kReportPrefix) - 1, kReportPrefix) == 0) mine.push_back(e);
	}
	std::sort(mine.begin(), mine.end(), [](const Entry &a, const Entry &b) { return a.name > b.name; });
	for (size_t i = keep; i < mine.size(); i++) {
		const std::string path = reports + "\\" + mine[i].name;
		if (mine[i].directory) RemoveTree(path);
		else DeleteFileA(path.c_str());
	}
}

class Staging {
public:
	Staging(const std::string &folder, LogRedactor &redactor, ReportResult &result) : folder_(folder), redactor_(redactor), result_(result) {}

	void AddText(const std::string &name, const std::string &text, const std::string &note = std::string())
	{
		const size_t before = redactor_.Replacements();
		const std::string clean = redactor_.Apply(text);
		if (!WriteAll(folder_ + "\\" + name, clean)) return;
		ReportFile file;
		file.name = name;
		file.bytes = clean.size();
		file.replaced = redactor_.Replacements() - before;
		file.note = note;
		result_.files.push_back(file);
	}

	bool AddFile(const std::string &name, const std::string &source, size_t cap, bool keepTail, const std::string &note = std::string())
	{
		std::string text;
		if (!ReadCapped(source, cap, keepTail, text)) return false;
		AddText(name, text, note);
		return true;
	}

private:
	std::string folder_;
	LogRedactor &redactor_;
	ReportResult &result_;
};

std::string FileList(const ReportResult &result)
{
	std::string text;
	for (const ReportFile &f : result.files) {
		text += "  " + f.name + "  " + std::to_string(f.bytes) + " bytes, " + std::to_string(f.replaced) + " replaced";
		if (!f.note.empty()) text += ", " + f.note;
		text += "\r\n";
	}
	return text;
}

}

std::string ReportFolder(const LogPaths &paths)
{
	return paths.modDir + "\\reports";
}

ReportResult ReportCreate(const LogPaths &paths, const ReportInput &input)
{
	ReportResult result;
	const std::string reports = ReportFolder(paths);
	EnsureFolder(reports);
	const std::string stamp = Stamp();
	const std::string staging = reports + "\\" + kReportPrefix + stamp;
	RemoveTree(staging);
	EnsureFolder(staging);
	if (GetFileAttributesA(staging.c_str()) == INVALID_FILE_ATTRIBUTES) {
		result.error = "could not create the report folder";
		return result;
	}

	LogRedactor redactor;
	if (!input.ownName.empty()) redactor.AddName(input.ownName, "you");
	for (const std::string &name : input.otherNames) redactor.AddName(name);
	char user[256] = {};
	DWORD userSize = sizeof(user);
	if (GetUserNameA(user, &userSize)) redactor.AddUserName(user);
	int profile = 0;
	for (const Entry &e : ListFolder(paths.gameDir + "\\Profiles")) {
		if (e.directory) redactor.AddName(e.name, "profile" + std::to_string(++profile));
	}

	Staging out(staging, redactor, result);
	std::vector<std::string> logs;
	for (const std::string &log : {input.currentLog, paths.crashed, paths.Numbered(1), paths.Numbered(2)}) {
		if (!log.empty() && std::find(logs.begin(), logs.end(), log) == logs.end()) logs.push_back(log);
	}
	for (const std::string &log : logs) out.AddFile("logs\\" + BaseName(log), log, kLogCap, true);

	std::string settings;
	if (ReadCapped(paths.gameDir + "\\imperivm-mod.ini", kSettingsCap, false, settings)) out.AddText("settings\\imperivm-mod.ini", RedactIniSecrets(settings));

	std::vector<Entry> traces;
	for (const Entry &e : ListFolder(paths.modDir + "\\rejoin")) {
		if (!e.directory && e.name.size() > 4 && e.name.compare(e.name.size() - 4, 4, ".txt") == 0) traces.push_back(e);
	}
	std::sort(traces.begin(), traces.end(), [](const Entry &a, const Entry &b) { return a.time > b.time; });
	if (traces.size() > kRejoinFiles) traces.resize(kRejoinFiles);
	for (const Entry &e : traces) out.AddFile("rejoin\\" + e.name, paths.modDir + "\\rejoin\\" + e.name, kTraceCap, true, "modified " + LocalTime(e.time));

	const Found crash = NewestUnderLogs(paths.logsDir, "vx.log", kCrashMaxAge);
	if (!crash.path.empty()) out.AddFile("game\\vx.log", crash.path, kSideFileCap, false, "the game's own crash report, " + LocalTime(crash.time));
	const Found desync = NewestUnderLogs(paths.logsDir, "desync.txt", kDesyncMaxAge);
	if (!desync.path.empty()) out.AddFile("game\\desync.txt", desync.path, kDesyncCap, false, "the game's desync dump, " + LocalTime(desync.time));
	out.AddFile("game\\comp_out.txt", paths.logsDir + "\\comp_out.txt", kCompilerOutputCap, true);

	std::string info = "Imperivm Resurrected - problem report\r\n";
	info += "Created: " + LocalTime(NowFileTime()) + " (local time)\r\n";
	info += input.info;
	if (info.back() != '\n') info += "\r\n";
	info = redactor.Apply(info) + kPrivacyNote + "\r\nFiles\r\n" + FileList(result);
	WriteAll(staging + "\\report-info.txt", info);

	const std::string zip = staging + ".zip";
	if (ZipFolder(staging, zip)) {
		RemoveTree(staging);
		result.zipped = true;
		result.path = zip;
	}
	else {
		result.path = staging;
	}
	result.ok = true;
	PruneReports(reports, kReportsKept);
	LogI("report", "created %s with %u files%s", BaseName(result.path).c_str(), static_cast<unsigned>(result.files.size()), result.zipped ? "" : " (could not compress, left as a folder)");
	return result;
}
