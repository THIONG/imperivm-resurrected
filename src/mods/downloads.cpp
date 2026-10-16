// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>
#include <wincodec.h>
#include <winhttp.h>

#include <algorithm>
#include <deque>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

#include "mods/downloads.h"
#include "core/json.h"
#include "core/guard.h"
#include "core/pathsafe.h"
#include "core/versioncmp.h"
#include "mods/mods.h"
#include "i18n/tr.h"
#include "core/uiport.h"

namespace {

const wchar_t kUserAgent[] = L"Imperivm-mod/0.9";
const uint64_t kMaxDownload = 4ull << 30;
const size_t kMaxJson = 8u << 20;

std::string GameDir()
{
	static std::string dir;
	if (dir.empty()) {
		HMODULE self = nullptr;
		GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCSTR>(&GameDir), &self);
		char path[MAX_PATH];
		GetModuleFileNameA(self, path, MAX_PATH);
		char *slash = strrchr(path, '\\');
		if (slash) *slash = 0;
		dir = path;
	}
	return dir;
}

std::string ModsDir() { return GameDir() + "\\mods"; }
std::string WorkDir() { return ModsDir() + "\\_downloads"; }
std::string IniPath() { return GameDir() + "\\imperivm-mod.ini"; }

std::string IniGet(const char *section, const char *key)
{
	char buf[2048] = {};
	GetPrivateProfileStringA(section, key, "", buf, sizeof(buf), IniPath().c_str());
	return buf;
}

void MakeDirs(const std::string &path)
{
	for (size_t i = 3; i <= path.size(); i++) {
		if (i == path.size() || path[i] == '\\') {
			CreateDirectoryA(path.substr(0, i).c_str(), nullptr);
		}
	}
}

bool IsDir(const std::string &p)
{
	DWORD a = GetFileAttributesA(p.c_str());
	return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

bool IsFile(const std::string &p)
{
	DWORD a = GetFileAttributesA(p.c_str());
	return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

std::string Lower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
	return s;
}

std::wstring Wide(const std::string &s)
{
	int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	std::wstring w(n > 0 ? n - 1 : 0, L'\0');
	if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
	return w;
}

std::string Narrow(const std::wstring &w, UINT cp = CP_UTF8)
{
	int n = WideCharToMultiByte(cp, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string s(n > 0 ? n - 1 : 0, '\0');
	if (n > 1) WideCharToMultiByte(cp, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
	return s;
}

void DeleteTree(const std::string &dir)
{
	if (dir.find("\\_downloads\\") == std::string::npos) {
		return;
	}
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((dir + "\\*").c_str(), &fd);
	if (h != INVALID_HANDLE_VALUE) {
		do {
			std::string n = fd.cFileName;
			if (n == "." || n == "..") continue;
			std::string p = dir + "\\" + n;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					RemoveDirectoryA(p.c_str());
				}
				else {
					SetFileAttributesA(p.c_str(), FILE_ATTRIBUTE_NORMAL);
					DeleteFileA(p.c_str());
				}
			}
			else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				DeleteTree(p);
			}
			else {
				SetFileAttributesA(p.c_str(), FILE_ATTRIBUTE_NORMAL);
				DeleteFileA(p.c_str());
			}
		} while (FindNextFileA(h, &fd));
		FindClose(h);
	}
	RemoveDirectoryA(dir.c_str());
}

std::string Timestamp()
{
	SYSTEMTIME t;
	GetLocalTime(&t);
	char buf[32];
	sprintf_s(buf, "%04d%02d%02d-%02d%02d%02d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
	return buf;
}

std::string HashFile(const std::string &path, LPCWSTR alg)
{
	BCRYPT_ALG_HANDLE a = nullptr;
	BCRYPT_HASH_HANDLE h = nullptr;
	std::string hex;
	if (BCryptOpenAlgorithmProvider(&a, alg, nullptr, 0) != 0) return hex;
	if (BCryptCreateHash(a, &h, nullptr, 0, nullptr, 0, 0) == 0) {
		HANDLE f = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
		if (f != INVALID_HANDLE_VALUE) {
			std::vector<unsigned char> buf(1 << 20);
			DWORD n = 0;
			while (ReadFile(f, buf.data(), static_cast<DWORD>(buf.size()), &n, nullptr) && n) {
				BCryptHashData(h, buf.data(), n, 0);
			}
			CloseHandle(f);
			DWORD len = 0, got = 0;
			BCryptGetProperty(a, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&len), sizeof(len), &got, 0);
			std::vector<unsigned char> out(len);
			if (BCryptFinishHash(h, out.data(), len, 0) == 0) {
				static const char *d = "0123456789abcdef";
				for (unsigned char c : out) {
					hex += d[c >> 4];
					hex += d[c & 15];
				}
			}
		}
		BCryptDestroyHash(h);
	}
	BCryptCloseAlgorithmProvider(a, 0);
	return hex;
}

std::mutex g_lock;
DownloadStatus g_status;
std::atomic<bool> g_cancel{false};
std::vector<CatalogEntry> g_catalog;
bool g_catalogBusy = false;

struct Job {
	enum Kind { Entry, Archive } kind;
	CatalogEntry entry;
	std::string text;
};
std::deque<Job> g_jobs;
std::condition_variable g_cv;
bool g_workerStarted = false;

void SetProgress(uint64_t done, uint64_t total)
{
	std::lock_guard<std::mutex> lock(g_lock);
	g_status.done = done;
	g_status.total = total;
}

void Finish(bool ok, const std::string &msg)
{
	{
		std::lock_guard<std::mutex> lock(g_lock);
		g_status.lastOk = ok;
		g_status.message = msg;
		g_status.finished++;
	}
	LogAt(ok ? kLogInfo : kLogWarn, "download", "%s", TrSource(msg).c_str());
	UiToast(msg, !ok);
	if (ok) UiRefreshMainMenuButton();
}

std::string HttpGet(const std::string &url, const std::vector<std::wstring> &headers, std::string *toMemory,
	const std::string &toFile, bool progress)
{
	std::wstring wurl = Wide(url);
	URL_COMPONENTS uc = {sizeof(uc)};
	wchar_t host[256] = {}, path[4096] = {};
	uc.lpszHostName = host;
	uc.dwHostNameLength = 256;
	uc.lpszUrlPath = path;
	uc.dwUrlPathLength = 4096;
	uc.dwExtraInfoLength = static_cast<DWORD>(-1);
	if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) {
		return "invalid address";
	}
	if (uc.nScheme != INTERNET_SCHEME_HTTPS) {
		return "only https:// addresses are supported";
	}
	std::wstring object = std::wstring(uc.lpszUrlPath, uc.dwUrlPathLength) +
		(uc.lpszExtraInfo ? std::wstring(uc.lpszExtraInfo, uc.dwExtraInfoLength) : L"");
	HINTERNET s = WinHttpOpen(kUserAgent, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);
	if (!s) {
		s = WinHttpOpen(kUserAgent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	}
	HINTERNET c = s ? WinHttpConnect(s, std::wstring(uc.lpszHostName, uc.dwHostNameLength).c_str(), uc.nPort, 0) : nullptr;
	HINTERNET r = c ? WinHttpOpenRequest(c, L"GET", object.c_str(), nullptr, WINHTTP_NO_REFERER,
						  WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)
					: nullptr;
	std::string err;
	auto close = [&] {
		if (r) WinHttpCloseHandle(r);
		if (c) WinHttpCloseHandle(c);
		if (s) WinHttpCloseHandle(s);
	};
	if (!r) {
		close();
		return "could not connect";
	}
	for (const std::wstring &h : headers) {
		WinHttpAddRequestHeaders(r, h.c_str(), static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD);
	}
	if (!WinHttpSendRequest(r, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
		!WinHttpReceiveResponse(r, nullptr)) {
		char buf[64];
		sprintf_s(buf, "network error %lu", GetLastError());
		close();
		return buf;
	}
	DWORD code = 0, size = sizeof(code);
	WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &code, &size, nullptr);
	if (code != 200) {
		std::string detail;
		char body[2048];
		DWORD got = 0;
		if (WinHttpReadData(r, body, sizeof(body) - 1, &got) && got) {
			body[got] = 0;
			Json j;
			if (Json::Parse(body, j)) {
				if (const Json *e = j.Get("error")) detail = e->GetString("message");
				else detail = j.GetString("_sErrorMessage", j.GetString("_sErrorCode"));
			}
		}
		close();
		return Tr("the server responded ") + std::to_string(code) + (detail.empty() ? "" : ": " + detail);
	}
	wchar_t lenText[32] = {};
	DWORD lenSize = sizeof(lenText);
	uint64_t total = 0;
	if (WinHttpQueryHeaders(r, WINHTTP_QUERY_CONTENT_LENGTH, nullptr, lenText, &lenSize, nullptr)) {
		total = _wcstoui64(lenText, nullptr, 10);
	}
	if (total > kMaxDownload) {
		close();
		return "the file is too large";
	}
	HANDLE file = INVALID_HANDLE_VALUE;
	if (!toFile.empty()) {
		file = CreateFileA(toFile.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (file == INVALID_HANDLE_VALUE) {
			close();
			return Tr("could not create ") + toFile;
		}
	}
	std::vector<char> buf(256 * 1024);
	uint64_t done = 0;
	if (progress) SetProgress(0, total);
	for (;;) {
		if (g_cancel) {
			err = Tr("canceled");
			break;
		}
		DWORD n = 0;
		if (!WinHttpReadData(r, buf.data(), static_cast<DWORD>(buf.size()), &n)) {
			err = "connection interrupted";
			break;
		}
		if (n == 0) break;
		done += n;
		if (done > kMaxDownload || (toMemory && done > kMaxJson)) {
			err = "response too large";
			break;
		}
		if (toMemory) toMemory->append(buf.data(), n);
		DWORD written = 0;
		if (file != INVALID_HANDLE_VALUE && (!WriteFile(file, buf.data(), n, &written, nullptr) || written != n)) {
			err = "could not write to disk";
			break;
		}
		if (progress) SetProgress(done, total);
	}
	if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
	if (!err.empty() && !toFile.empty()) DeleteFileA(toFile.c_str());
	close();
	return err;
}

std::string HttpGetJson(const std::string &url, const std::vector<std::wstring> &headers, Json &out)
{
	std::string body;
	std::string err = HttpGet(url, headers, &body, "", false);
	if (!err.empty()) return err;
	std::string perr;
	if (!Json::Parse(body, out, &perr)) return TrFmt("invalid response (%s)", {perr});
	return "";
}

const char kGbApi[] = "https://gamebanana.com/apiv11";

std::string GbGame()
{
	const std::string g = IniGet("GameBanana", "GameId");
	return g.empty() ? "26069" : g;
}

std::string UrlEncode(const std::string &s)
{
	std::string out;
	char buf[4];
	for (unsigned char c : s) {
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += static_cast<char>(c);
		else {
			sprintf_s(buf, "%%%02X", c);
			out += buf;
		}
	}
	return out;
}

std::string StripHtml(const std::string &html)
{
	std::string out;
	for (size_t i = 0; i < html.size();) {
		if (html[i] == '<') {
			const size_t end = html.find('>', i);
			if (end == std::string::npos) break;
			const std::string tag = Lower(html.substr(i + 1, end - i - 1));
			if (tag.compare(0, 2, "br") == 0 || tag == "/p" || tag == "/div" || tag.compare(0, 2, "/h") == 0) out += '\n';
			else if (tag.compare(0, 2, "li") == 0) out += "\n\xE2\x80\xA2 ";
			i = end + 1;
			continue;
		}
		if (html[i] == '&') {
			static const struct {
				const char *ent, *text;
			} kEnt[] = {{"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&#039;", "'"}, {"&#39;", "'"}, {"&nbsp;", " "}};
			bool done = false;
			for (const auto &e : kEnt) {
				const size_t n = strlen(e.ent);
				if (html.compare(i, n, e.ent) == 0) {
					out += e.text;
					i += n;
					done = true;
					break;
				}
			}
			if (done) continue;
		}
		out += html[i++];
	}
	std::string clean;
	int blanks = 0;
	for (char c : out) {
		if (c == '\n') {
			if (++blanks > 2) continue;
		}
		else if (c != ' ' && c != '\r') {
			blanks = 0;
		}
		clean += c;
	}
	const size_t first = clean.find_first_not_of("\n ");
	clean.erase(0, first == std::string::npos ? clean.size() : first);
	while (!clean.empty() && (clean.back() == '\n' || clean.back() == ' ')) clean.pop_back();
	return clean;
}

std::string GbImageUrl(const Json &m)
{
	const Json *media = m.Get("_aPreviewMedia");
	const Json *images = media ? media->Get("_aImages") : nullptr;
	if (!images || images->arr.empty()) return "";
	const Json &img = images->arr[0];
	const std::string base = img.GetString("_sBaseUrl");
	const std::string file = img.GetString("_sFile530", img.GetString("_sFile"));
	return base.empty() || file.empty() ? "" : base + "/" + file;
}

std::string DateVersion(uint64_t ts)
{
	const time_t t = static_cast<time_t>(ts);
	struct tm tmv = {};
	if (!ts || gmtime_s(&tmv, &t) != 0) return "";
	char buf[16];
	strftime(buf, sizeof(buf), "%Y.%m.%d", &tmv);
	return buf;
}

void GbFillSummary(GbMod &r, const Json &m)
{
	r.id = static_cast<uint64_t>(m.GetNumber("_idRow"));
	r.name = m.GetString("_sName");
	r.page = m.GetString("_sProfileUrl");
	if (const Json *u = m.Get("_aSubmitter")) r.author = u->GetString("_sName");
	r.logoUrl = GbImageUrl(m);
	r.shots.clear();
	if (const Json *media = m.Get("_aPreviewMedia")) {
		if (const Json *images = media->Get("_aImages")) {
			for (const Json &img : images->arr) {
				const std::string base = img.GetString("_sBaseUrl"), file = img.GetString("_sFile");
				if (base.empty() || file.empty()) continue;
				GbShot s;
				s.big = base + "/" + file;
				const std::string thumbFile = img.GetString("_sFile220", img.GetString("_sFile530", file));
				s.thumb = base + "/" + thumbFile;
				r.shots.push_back(s);
				if (r.shots.size() >= 14) break;
			}
		}
	}
	r.likes = static_cast<int>(m.GetNumber("_nLikeCount"));
	r.views = static_cast<uint64_t>(m.GetNumber("_nViewCount"));
	r.added = static_cast<uint64_t>(m.GetNumber("_tsDateAdded"));
	r.updated = std::max<uint64_t>(r.added, static_cast<uint64_t>(std::max(m.GetNumber("_tsDateUpdated"), m.GetNumber("_tsDateModified"))));
	r.hasFile = m.GetBool("_bHasFiles", true);
	r.tags.clear();
	const Json *cat = m.Get("_aRootCategory");
	if (!cat) cat = m.Get("_aCategory");
	if (cat) r.category = cat->GetString("_sName");
	if (!r.category.empty()) r.tags.push_back(r.category);
	if (const Json *sub = m.Get("_aSubCategory")) {
		const std::string n = sub->GetString("_sName");
		if (!n.empty() && n != r.category) r.tags.push_back(n);
	}
	if (const Json *t = m.Get("_aTags")) {
		for (const Json &tag : t->arr) {
			const std::string n = tag.type == Json::String ? tag.str : tag.GetString("_sValue", tag.GetString("_sTitle"));
			if (!n.empty() && std::find(r.tags.begin(), r.tags.end(), n) == r.tags.end()) r.tags.push_back(n);
		}
	}
	r.facts = CatalogFactsOf(r.category, r.tags);
	CatalogEntry &e = r.entry;
	e.source = "GameBanana";
	e.id = "gamebanana-" + std::to_string(r.id);
	e.name = r.name;
	e.page = r.page;
	e.gbId = r.id;
	e.authors.clear();
	if (!r.author.empty()) e.authors.push_back(r.author);
	e.tags = r.tags;
	e.version = m.GetString("_sVersion");
	if (e.version.empty()) e.version = DateVersion(r.updated);
}

size_t GbUsableFiles(const Json *files)
{
	size_t n = 0;
	if (files) {
		for (const Json &f : files->arr) {
			if (!f.GetBool("_bIsArchived", false)) n++;
		}
	}
	return n;
}

std::string GbFileVersion(const Json &file, const std::string &modVersion, size_t usableFiles)
{
	const std::string v = file.GetString("_sVersion");
	if (!v.empty()) return v;
	if (usableFiles == 1 && !modVersion.empty()) return modVersion;
	return DateVersion(static_cast<uint64_t>(file.GetNumber("_tsDateAdded")));
}

void GbFillDetails(GbMod &r, const Json &p)
{
	GbFillSummary(r, p);
	r.summary = p.GetString("_sDescription");
	r.descriptionHtml = p.GetString("_sText");
	r.description = StripHtml(r.descriptionHtml);
	r.downloads = static_cast<uint64_t>(p.GetNumber("_nDownloadCount"));
	CatalogEntry &e = r.entry;
	e.description = r.summary.empty() ? r.description.substr(0, 300) : r.summary;
	e.url.clear();
	const Json *best = nullptr;
	const Json *files = p.Get("_aFiles");
	if (files) {
		for (const Json &f : files->arr) {
			if (f.GetBool("_bIsArchived", false) || f.GetString("_sDownloadUrl").empty()) continue;
			if (!best || FileIsNewer(f.GetString("_sVersion"), static_cast<unsigned long long>(f.GetNumber("_tsDateAdded")), best->GetString("_sVersion"),
					static_cast<unsigned long long>(best->GetNumber("_tsDateAdded")))) {
				best = &f;
			}
		}
	}
	if (best) {
		e.url = best->GetString("_sDownloadUrl");
		e.bytes = static_cast<uint64_t>(best->GetNumber("_nFilesize"));
		e.md5 = Lower(best->GetString("_sMd5Checksum"));
		e.version = GbFileVersion(*best, p.GetString("_sVersion"), GbUsableFiles(files));
	}
	r.hasFile = !e.url.empty();
	r.deps.clear();
	if (const Json *req = p.Get("_aRequirements")) {
		for (const Json &q : req->arr) {
			if (q.type != Json::Array || q.arr.size() < 2 || q.arr[1].type != Json::String) continue;
			const std::string url = Lower(q.arr[1].str);
			const size_t at = url.find("gamebanana.com/mods/");
			if (at == std::string::npos) continue;
			const uint64_t dep = _strtoui64(url.c_str() + at + 20, nullptr, 10);
			if (dep && dep != r.id && std::find(r.deps.begin(), r.deps.end(), dep) == r.deps.end()) r.deps.push_back(dep);
		}
	}
	r.dependencies = !r.deps.empty();
	r.details = true;
}

std::string GbFetchMod(uint64_t id, GbMod &out)
{
	Json j;
	const std::string err = HttpGetJson(std::string(kGbApi) + "/Mod/" + std::to_string(id) + "/ProfilePage", {}, j);
	if (!err.empty()) return err;
	if (!j.Get("_idRow")) return j.GetString("_sErrorCode", Tr("empty response"));
	if (j.GetBool("_bIsTrashed", false) || j.GetBool("_bIsWithheld", false) || j.GetBool("_bIsPrivate", false)) {
		return "the content is not available";
	}
	GbFillDetails(out, j);
	return "";
}

std::string GbListAll(std::vector<GbMod> &out)
{
	out.clear();
	for (int page = 1; page <= 40; page++) {
		Json j;
		const std::string err = HttpGetJson(std::string(kGbApi) + "/Mod/Index?_nPerpage=50&_nPage=" + std::to_string(page) +
			"&_aFilters%5BGeneric_Game%5D=" + GbGame(), {}, j);
		if (!err.empty()) return err;
		const Json *recs = j.Get("_aRecords");
		if (!recs) return j.GetString("_sErrorCode", Tr("empty response"));
		for (const Json &m : recs->arr) {
			GbMod r;
			GbFillSummary(r, m);
			if (r.id) out.push_back(r);
		}
		const Json *meta = j.Get("_aMetadata");
		if (recs->arr.empty() || (meta && meta->GetBool("_bIsComplete", false))) break;
	}
	return "";
}

void ReadGb(std::vector<CatalogEntry> &out, std::string &errors)
{
	for (const ModInfo &m : ModsList()) {
		if (!m.gbId) continue;
		if (std::find_if(out.begin(), out.end(), [&](const CatalogEntry &e) { return e.gbId == m.gbId; }) != out.end()) continue;
		GbMod r;
		const std::string err = GbFetchMod(m.gbId, r);
		if (!err.empty()) {
			errors += (errors.empty() ? "" : "; ") + std::string("GameBanana ") + std::to_string(m.gbId) + ": " + err;
			continue;
		}
		if (!r.hasFile) continue;
		r.entry.id = m.id;
		out.push_back(r.entry);
	}
}

void RefreshCatalogsNow()
{
	std::vector<CatalogEntry> entries;
	std::string errors;
	ReadGb(entries, errors);
	std::vector<CatalogEntry> unique;
	for (const CatalogEntry &e : entries) {
		auto it = std::find_if(unique.begin(), unique.end(), [&](const CatalogEntry &u) { return _stricmp(u.id.c_str(), e.id.c_str()) == 0; });
		if (it == unique.end()) unique.push_back(e);
		else if (CompareVersions(e.version, it->version) > 0) *it = e;
	}
	std::sort(unique.begin(), unique.end(), [](const CatalogEntry &a, const CatalogEntry &b) { return Lower(a.name) < Lower(b.name); });
	const size_t found = unique.size();
	std::lock_guard<std::mutex> lock(g_lock);
	g_catalog = std::move(unique);
	g_catalogBusy = false;
	LogAt(errors.empty() ? kLogInfo : kLogWarn, "download", "catalog: %u installed content packs checked on GameBanana%s%s", static_cast<unsigned>(found),
		errors.empty() ? "" : ". Errors: ", errors.c_str());
	UiRefreshMainMenuButton();
}

bool IsMarker(const std::string &name)
{
	static const char *kMarkers[] = {"mod.json", "packs", "local", "patches", "data", "scenarios", "adventures",
		"conquests", "conquestmaps", "ui", "units", "sounds", "currentlang", "music", "movies"};
	const std::string n = Lower(name);
	for (const char *m : kMarkers) {
		if (n == m) return true;
	}
	return false;
}

std::string FindModRoot(std::string dir)
{
	for (int depth = 0; depth < 4; depth++) {
		WIN32_FIND_DATAA fd;
		HANDLE h = FindFirstFileA((dir + "\\*").c_str(), &fd);
		if (h == INVALID_HANDLE_VALUE) return dir;
		int dirs = 0, files = 0;
		bool marker = false;
		std::string only;
		do {
			std::string n = fd.cFileName;
			if (n == "." || n == "..") continue;
			marker = marker || IsMarker(n);
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				dirs++;
				only = n;
			}
			else {
				files++;
			}
		} while (FindNextFileA(h, &fd));
		FindClose(h);
		if (marker || dirs != 1 || files > 0) return dir;
		dir += "\\" + only;
	}
	return dir;
}

void CheckFiles(const std::string &base, const std::string &rel, std::string &bad, unsigned &count)
{
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((base + "\\" + rel + "*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return;
	do {
		std::string n = fd.cFileName;
		if (n == "." || n == "..") continue;
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) || !pathsafe::SafeComponent(n)) {
			if (bad.empty()) bad = rel + n;
			continue;
		}
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			CheckFiles(base, rel + n + "\\", bad, count);
		}
		else {
			count++;
			if (bad.empty() && !ModsAllowedFile(rel + n)) bad = rel + n;
		}
	} while (FindNextFileA(h, &fd));
	FindClose(h);
}

std::string SafeFolder(const std::string &name)
{
	std::string out;
	for (unsigned char c : name) {
		out += (isalnum(c) || c >= 0x80 || (c != 0 && strchr(" -_.()+", c))) ? static_cast<char>(c) : '_';
	}
	while (!out.empty() && (out[0] == '_' || out[0] == '.' || out[0] == ' ')) out.erase(0, 1);
	while (!out.empty() && (out.back() == '.' || out.back() == ' ')) out.pop_back();
	if (out.empty()) return "mod";
	out = out.substr(0, 80);
	if (pathsafe::ReservedDeviceName(out)) out += "-mod";
	return out;
}

std::string TarPath()
{
	char sys[MAX_PATH];
	GetSystemDirectoryA(sys, MAX_PATH);
	return std::string(sys) + "\\tar.exe";
}

bool RunTar(const std::string &arguments, HANDLE out, DWORD timeoutMs, DWORD &code)
{
	const std::string cmd = "\"" + TarPath() + "\" " + arguments;
	STARTUPINFOA si = {sizeof(si)};
	if (out != INVALID_HANDLE_VALUE) {
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		si.hStdOutput = out;
		si.hStdError = out;
	}
	PROCESS_INFORMATION pi = {};
	std::vector<char> line(cmd.begin(), cmd.end());
	line.push_back(0);
	if (!CreateProcessA(nullptr, line.data(), nullptr, nullptr, out != INVALID_HANDLE_VALUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
		return false;
	}
	code = 1;
	if (WaitForSingleObject(pi.hProcess, timeoutMs) == WAIT_TIMEOUT) {
		TerminateProcess(pi.hProcess, 1);
		WaitForSingleObject(pi.hProcess, 5000);
		LogW("download", "tar.exe took more than %lu s and was closed", timeoutMs / 1000);
	}
	else {
		GetExitCodeProcess(pi.hProcess, &code);
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return true;
}

std::string ReadCapped(const std::string &path, size_t cap)
{
	std::string out;
	HANDLE f = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (f == INVALID_HANDLE_VALUE) return out;
	char buf[65536];
	DWORD got = 0;
	while (out.size() < cap && ReadFile(f, buf, sizeof(buf), &got, nullptr) && got) out.append(buf, got);
	CloseHandle(f);
	return out;
}

const size_t kMaxArchiveEntries = 60000;
const unsigned long long kMaxArchiveBytes = 6ull << 30;

std::string InspectArchive(const std::string &archive)
{
	const std::string listing = WorkDir() + "\\list-" + Timestamp() + ".txt";
	SECURITY_ATTRIBUTES sa = {sizeof(sa), nullptr, TRUE};
	HANDLE out = CreateFileA(listing.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
	if (out == INVALID_HANDLE_VALUE) return "the extraction could not be prepared";
	DWORD code = 1;
	const bool ran = RunTar("-t -v -f \"" + archive + "\"", out, 5 * 60 * 1000, code);
	CloseHandle(out);
	const std::string text = ReadCapped(listing, 48u << 20);
	DeleteFileA(listing.c_str());
	if (!ran) return "could not run tar.exe (Windows 10 or later required)";
	if (code != 0) return "the archive could not be read (unsupported format or damaged file)";
	size_t entries = 0;
	unsigned long long bytes = 0;
	for (size_t pos = 0; pos < text.size();) {
		size_t end = text.find('\n', pos);
		if (end == std::string::npos) end = text.size();
		const std::string row = text.substr(pos, end - pos);
		pos = end + 1;
		if (row.size() < 12 || !strchr("-dlhcbps", row[0]) || !strchr("r-", row[1]) || !strchr("w-", row[2])) continue;
		if (row[0] != '-' && row[0] != 'd') return "the archive contains links or special files: it will not be installed";
		entries++;
		size_t at = 0;
		for (int field = 0; field < 4; field++) {
			at = row.find_first_not_of(' ', at);
			if (at == std::string::npos) break;
			at = row.find(' ', at);
			if (at == std::string::npos) break;
		}
		if (at != std::string::npos) bytes += _strtoui64(row.c_str() + at, nullptr, 10);
	}
	if (entries > kMaxArchiveEntries || bytes > kMaxArchiveBytes) return "the archive is too large to install";
	return "";
}

std::string Extract(const std::string &archive, const std::string &dest)
{
	const std::string bad = InspectArchive(archive);
	if (!bad.empty()) return bad;
	DWORD code = 1;
	if (!RunTar("-x -f \"" + archive + "\" -C \"" + dest + "\"", INVALID_HANDLE_VALUE, 30 * 60 * 1000, code)) {
		return "could not run tar.exe (Windows 10 or later required)";
	}
	return code == 0 ? "" : "could not extract (unsupported format or damaged file)";
}

bool InstallArchiveNow(const std::string &archive, const CatalogEntry &info, const Json &sources, std::string &msg)
{
	const std::string tmp = WorkDir() + "\\tmp-" + Timestamp();
	MakeDirs(tmp);
	msg = Extract(archive, tmp);
	if (!msg.empty()) {
		DeleteTree(tmp);
		return false;
	}
	const std::string root = FindModRoot(tmp);
	std::string bad;
	unsigned count = 0;
	CheckFiles(root, "", bad, count);
	if (count == 0) {
		DeleteTree(tmp);
		msg = "the file is empty";
		return false;
	}
	if (!bad.empty()) {
		DeleteTree(tmp);
		msg = Tr("not installed: it contains a disallowed file (") + bad + ")";
		return false;
	}
	Json manifest;
	const std::string manifestPath = root + "\\mod.json";
	const bool hasManifest = ReadJsonFile(manifestPath, manifest) && manifest.type == Json::Object;
	if (!hasManifest) {
		manifest = Json::MakeObject();
		std::string base = archive.substr(archive.find_last_of('\\') + 1);
		base = base.substr(0, base.find_last_of('.'));
		manifest.Set("id", Json::MakeString(info.id.empty() ? Lower(SafeFolder(base)) : info.id));
		manifest.Set("name", Json::MakeString(info.name.empty() ? base : info.name));
		manifest.Set("version", Json::MakeString(info.version));
		Json authors = Json::MakeArray();
		for (const std::string &a : info.authors) authors.arr.push_back(Json::MakeString(a));
		manifest.Set("authors", authors);
		manifest.Set("description", Json::MakeString(info.description));
		manifest.Set("autoGenerated", Json::MakeBool(true));
	}
	if (sources.type == Json::Object && !manifest.Get("sources")) {
		manifest.Set("sources", sources);
	}
	else if (sources.type == Json::Object && sources.Get("gamebanana")) {
		Json f = *manifest.Get("sources");
		if (f.type != Json::Object) f = Json::MakeObject();
		f.Set("gamebanana", *sources.Get("gamebanana"));
		if (sources.Get("tags")) f.Set("tags", *sources.Get("tags"));
		if (!f.Get("page") && sources.Get("page")) f.Set("page", *sources.Get("page"));
		manifest.Set("sources", f);
	}
	WriteJsonFile(manifestPath, manifest);
	const std::string id = manifest.GetString("id");
	const std::string name = ModsLocalizedText(manifest, "name", id);
	const std::string version = manifest.GetString("version");
	std::string folder, best;
	for (const ModInfo &m : ModsList()) {
		const bool sameMod = (!id.empty() && _stricmp(m.id.c_str(), id.c_str()) == 0) || (info.gbId && m.gbId == info.gbId);
		if (!sameMod) continue;
		if (info.aside) {
			if (m.version == version) {
				DeleteTree(tmp);
				msg = name + " " + version + Tr(" is already installed");
				return true;
			}
		}
		else if (info.replaceFolder.empty() && (folder.empty() || ModsCompareVersions(m.version, best) > 0)) {
			folder = m.folder;
			best = m.version;
		}
	}
	if (!info.replaceFolder.empty() && IsDir(ModsDir() + "\\" + info.replaceFolder)) folder = info.replaceFolder;
	if (folder.empty()) {
		const std::string base = SafeFolder(info.aside && !version.empty() ? name + " " + version : name);
		folder = base;
		for (int n = 2; IsDir(ModsDir() + "\\" + folder); n++) {
			folder = base + " (" + std::to_string(n) + ")";
		}
	}
	const std::string target = ModsDir() + "\\" + folder;
	bool pending = false;
	if (IsDir(target)) {
		const std::string backup = WorkDir() + "\\backup\\" + folder + "-" + Timestamp();
		MakeDirs(WorkDir() + "\\backup");
		if (!MoveFileExA(target.c_str(), backup.c_str(), 0)) {
			const std::string pend = WorkDir() + "\\pending\\" + folder;
			MakeDirs(WorkDir() + "\\pending");
			DeleteTree(pend);
			if (!MoveFileExA(root.c_str(), pend.c_str(), 0)) {
				DeleteTree(tmp);
				msg = Tr("could not prepare the update for ") + name;
				return false;
			}
			pending = true;
		}
		else {
			LogI("download", "previous version of %s saved in %s", folder.c_str(), backup.c_str());
		}
	}
	if (!pending && !MoveFileExA(root.c_str(), target.c_str(), MOVEFILE_COPY_ALLOWED)) {
		DeleteTree(tmp);
		msg = Tr("could not copy to the content folder (error ") + std::to_string(GetLastError()) + ")";
		return false;
	}
	DeleteTree(tmp);
	ModsCancelUninstall(folder);
	ModsScan();
	const std::string label = name + (version.empty() || name.find(version) != std::string::npos ? "" : " " + version);
	msg = pending ? label + Tr(" downloaded (applied on restart)") : label + Tr(" installed");
	return true;
}

std::string DownloadToTemp(const std::string &url, const std::string &nameHint, std::string &file)
{
	MakeDirs(WorkDir());
	std::string ext = Lower(url.substr(0, url.find('?')));
	ext = ext.size() > 4 && ext.find_last_of('.') != std::string::npos ? ext.substr(ext.find_last_of('.')) : ".zip";
	if (ext.size() > 8 || ext.find('/') != std::string::npos) ext = ".zip";
	file = WorkDir() + "\\" + SafeFolder(nameHint) + "-" + Timestamp() + ext;
	return HttpGet(url, {}, nullptr, file, true);
}

void RunInstall(const CatalogEntry &e, const Json &sources)
{
	std::string file;
	std::string err = DownloadToTemp(e.url, e.name.empty() ? "mod" : e.name, file);
	if (err.empty() && !e.sha256.empty() && HashFile(file, BCRYPT_SHA256_ALGORITHM) != e.sha256) {
		err = "the SHA-256 hash does not match the catalog's (file tampered with or damaged)";
	}
	if (err.empty() && !e.md5.empty() && HashFile(file, BCRYPT_MD5_ALGORITHM) != e.md5) {
		err = "the MD5 hash does not match (damaged file)";
	}
	if (!err.empty()) {
		if (!file.empty()) DeleteFileA(file.c_str());
		Finish(false, Tr("Could not download ") + (e.name.empty() ? e.url : e.name) + ": " + err);
		return;
	}
	std::string msg;
	const bool ok = InstallArchiveNow(file, e, sources, msg);
	DeleteFileA(file.c_str());
	Finish(ok, ok ? msg : Tr("Could not install ") + (e.name.empty() ? e.url : e.name) + ": " + msg);
}

void Worker()
{
	for (;;) {
		Job job;
		{
			std::unique_lock<std::mutex> lock(g_lock);
			g_cv.wait(lock, [] { return !g_jobs.empty(); });
			job = g_jobs.front();
			g_jobs.pop_front();
			g_status.active = true;
			g_status.done = g_status.total = 0;
			g_status.queued = static_cast<int>(g_jobs.size());
			g_status.what = job.kind == Job::Entry
				? job.entry.name + (job.entry.version.empty() || job.entry.name.find(job.entry.version) != std::string::npos ? "" : " " + job.entry.version)
				: job.text;
		}
		g_cancel = false;
		LogI("download", "starting %s", g_status.what.c_str());
		if (job.kind == Job::Entry) {
			Json sources = Json::MakeObject();
			if (!job.entry.page.empty()) sources.Set("page", Json::MakeString(job.entry.page));
			if (job.entry.gbId) sources.Set("gamebanana", Json::MakeNumber(static_cast<double>(job.entry.gbId)));
			if (!job.entry.tags.empty()) {
				Json tags = Json::MakeArray();
				for (const std::string &t : job.entry.tags) tags.arr.push_back(Json::MakeString(t));
				sources.Set("tags", tags);
			}
			RunInstall(job.entry, sources);
		}
		else if (job.kind == Job::Archive) {
			std::string msg;
			const bool ok = InstallArchiveNow(job.text, CatalogEntry(), Json(), msg);
			Finish(ok, ok ? msg : Tr("Could not install ") + job.text + ": " + msg);
		}
		std::lock_guard<std::mutex> lock(g_lock);
		g_status.active = false;
		g_status.queued = static_cast<int>(g_jobs.size());
	}
}

void Enqueue(const Job &job)
{
	std::lock_guard<std::mutex> lock(g_lock);
	if (!g_workerStarted) {
		g_workerStarted = true;
		SafeThread(Worker).detach();
	}
	g_jobs.push_back(job);
	g_status.queued = static_cast<int>(g_jobs.size());
	g_cv.notify_one();
}

}

void DlRefreshCatalogs()
{
	{
		std::lock_guard<std::mutex> lock(g_lock);
		if (g_catalogBusy) return;
		g_catalogBusy = true;
	}
	SafeThread(RefreshCatalogsNow).detach();
}

void DlStartUpdateCheck()
{
	if (IniGet("Mods", "UpdateNotices") == "0") return;
	SafeThread([] {
		Sleep(8000);
		DlRefreshCatalogs();
		for (int i = 0; i < 120; i++) {
			Sleep(500);
			std::lock_guard<std::mutex> lock(g_lock);
			if (!g_catalogBusy) break;
		}
		const std::vector<ModInfo> mods = ModsList();
		std::vector<std::string> news;
		for (const ModInfo &m : mods) {
			CatalogEntry e;
			if (DlUpdateFor(m, mods, e)) news.push_back(m.name + " " + e.version);
		}
		if (news.empty()) {
			LogI("download", "mods up to date");
			return;
		}
		std::string text;
		if (news.size() == 1) {
			text = TrFmt("Update available: %s", {news[0]});
		}
		else {
			text = Tr("Updates available:");
			for (size_t i = 0; i < news.size() && i < 4; i++) text += "\n" + news[i];
			if (news.size() > 4) text += "\n" + TrFmt("and %s more", {std::to_string(news.size() - 4)});
		}
		UiToast(text);
		UiRefreshMainMenuButton();
	}).detach();
}

bool DlFindUpdate(const std::string &modId, const std::string &installedVersion, CatalogEntry &out, uint64_t gbId)
{
	std::lock_guard<std::mutex> lock(g_lock);
	for (const CatalogEntry &e : g_catalog) {
		const bool same = gbId && e.gbId ? e.gbId == gbId : _stricmp(e.id.c_str(), modId.c_str()) == 0;
		if (same && UpdateIsNewer(e.version, installedVersion)) {
			out = e;
			return true;
		}
	}
	return false;
}

void DlInstallEntry(const CatalogEntry &e)
{
	Job j{Job::Entry, e, ""};
	Enqueue(j);
}

void DlInstallArchive(const std::string &path)
{
	Job j{Job::Archive, CatalogEntry(), path};
	Enqueue(j);
}

std::string DlPickArchive()
{
	wchar_t file[MAX_PATH] = {};
	OPENFILENAMEW ofn = {sizeof(ofn)};
	ofn.hwndOwner = GetActiveWindow();
	std::wstring filter = Wide(Tr("Compressed content") + " (*.zip;*.7z;*.rar;*.tar;*.gz)");
	filter.push_back(L'\0');
	filter += L"*.zip;*.7z;*.rar;*.tar;*.gz;*.tgz";
	filter.push_back(L'\0');
	filter += Wide(Tr("All files"));
	filter.push_back(L'\0');
	filter += L"*.*";
	filter.push_back(L'\0');
	filter.push_back(L'\0');
	const std::wstring title = Wide(Tr("Install content from a file"));
	ofn.lpstrFilter = filter.c_str();
	ofn.lpstrFile = file;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = title.c_str();
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	if (!GetOpenFileNameW(&ofn)) return "";
	return Narrow(file, CP_ACP);
}

DownloadStatus DlStatus()
{
	std::lock_guard<std::mutex> lock(g_lock);
	return g_status;
}

void DlCancel()
{
	g_cancel = true;
}

void DlApplyPendingInstalls()
{
	const std::string pendDir = WorkDir() + "\\pending";
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((pendDir + "\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return;
	do {
		std::string n = fd.cFileName;
		if (n == "." || n == ".." || !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
		const std::string target = ModsDir() + "\\" + n;
		if (IsDir(target)) {
			MakeDirs(WorkDir() + "\\backup");
			MoveFileExA(target.c_str(), (WorkDir() + "\\backup\\" + n + "-" + Timestamp()).c_str(), 0);
		}
		if (MoveFileExA((pendDir + "\\" + n).c_str(), target.c_str(), 0)) {
			LogI("download", "applied the pending update of %s", n.c_str());
		}
		else {
			LogW("download", "could not apply the pending update of %s (error %lu)", n.c_str(), GetLastError());
		}
	} while (FindNextFileA(h, &fd));
	FindClose(h);
}

namespace {

std::mutex g_ioLock;
GbResults g_ioResults;
uint64_t g_ioWanted = 0;

std::vector<GbMod> g_gbAll;
DWORD g_gbAllStamp = 0;
bool g_gbAllValid = false;
const DWORD kGbListMs = 60 * 1000;

std::map<std::string, std::shared_ptr<const GbImage>> g_ioImages;
std::deque<std::string> g_ioImageQueue;
std::deque<std::string> g_bigOrder;
bool g_ioImageThread = false;
std::condition_variable g_ioImageCv;

std::string Fold(const std::string &s)
{
	std::string out = Lower(s);
	static const struct {
		const char *from, *to;
	} kMap[] = {{"\xC3\xA1", "a"}, {"\xC3\xA9", "e"}, {"\xC3\xAD", "i"}, {"\xC3\xB3", "o"}, {"\xC3\xBA", "u"}, {"\xC3\xBC", "u"},
		{"\xC3\xB1", "n"}, {"\xC3\x81", "a"}, {"\xC3\x89", "e"}, {"\xC3\x8D", "i"}, {"\xC3\x93", "o"}, {"\xC3\x9A", "u"}, {"\xC3\x91", "n"}};
	for (const auto &m : kMap) {
		for (size_t at = out.find(m.from); at != std::string::npos; at = out.find(m.from, at)) out.replace(at, strlen(m.from), m.to);
	}
	return out;
}

void RunSearch(GbQuery q, uint64_t serial)
{
	GbResults r;
	r.query = q;
	r.serial = serial;
	std::vector<GbMod> all;
	std::string err;
	{
		std::lock_guard<std::mutex> lock(g_ioLock);
		if (g_gbAllValid && GetTickCount() - g_gbAllStamp < kGbListMs) all = g_gbAll;
	}
	if (all.empty()) {
		err = GbListAll(all);
		if (err.empty()) {
			std::lock_guard<std::mutex> lock(g_ioLock);
			g_gbAll = all;
			g_gbAllStamp = GetTickCount();
			g_gbAllValid = true;
		}
	}
	if (!err.empty()) {
		r.error = err;
		LogW("download", "gamebanana: could not read the mod list (%s)", err.c_str());
	}
	else {
		const std::string search = Fold(q.search);
		std::vector<GbMod> found;
		for (const GbMod &m : all) {
			if (!search.empty()) {
				std::string hay = Fold(m.name + " " + m.author);
				for (const std::string &t : m.tags) hay += " " + Fold(t);
				if (m.facts.category >= 0) hay += " " + Fold(Tr(CatalogCategoryLabel(m.facts.category)));
				if (hay.find(search) == std::string::npos) continue;
			}
			CatalogCount(m.facts, r.counts);
			if (CatalogMatches(m.facts, q.filter)) found.push_back(m);
		}
		std::stable_sort(found.begin(), found.end(), [&](const GbMod &a, const GbMod &b) {
			switch (q.sort) {
			case 1: return a.likes != b.likes ? a.likes > b.likes : a.views > b.views;
			case 2: return a.added > b.added;
			case 3: return a.updated > b.updated;
			default: return a.views != b.views ? a.views > b.views : a.likes > b.likes;
			}
		});
		r.total = static_cast<int>(found.size());
		const size_t from = static_cast<size_t>(std::max(0, q.page * q.perPage));
		for (size_t i = from; i < found.size() && i < from + static_cast<size_t>(q.perPage); i++) r.mods.push_back(found[i]);
		std::vector<std::thread> threads;
		for (GbMod &m : r.mods) {
			threads.push_back(SafeThread([&m] {
				GbMod full = m;
				if (GbFetchMod(m.id, full).empty()) m = full;
			}));
		}
		for (std::thread &t : threads) t.join();
	}
	std::lock_guard<std::mutex> lock(g_ioLock);
	if (serial == g_ioWanted) g_ioResults = std::move(r);
}

std::shared_ptr<GbImage> DecodeImage(const std::string &bytes, UINT maxWidth = 320)
{
	std::shared_ptr<GbImage> out;
	IWICImagingFactory *factory = nullptr;
	IWICStream *stream = nullptr;
	IWICBitmapDecoder *decoder = nullptr;
	IWICBitmapFrameDecode *frame = nullptr;
	IWICBitmapScaler *scaler = nullptr;
	IWICFormatConverter *conv = nullptr;
	UINT w = 0, h = 0;
	if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))) &&
		SUCCEEDED(factory->CreateStream(&stream)) &&
		SUCCEEDED(stream->InitializeFromMemory(reinterpret_cast<BYTE *>(const_cast<char *>(bytes.data())), static_cast<DWORD>(bytes.size()))) &&
		SUCCEEDED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder)) &&
		SUCCEEDED(decoder->GetFrame(0, &frame)) && SUCCEEDED(frame->GetSize(&w, &h)) && w && h) {
		UINT tw = w, th = h;
		if (tw > maxWidth) {
			th = std::max<UINT>(1, th * maxWidth / tw);
			tw = maxWidth;
		}
		IWICBitmapSource *src = frame;
		if ((tw != w || th != h) && SUCCEEDED(factory->CreateBitmapScaler(&scaler)) &&
			SUCCEEDED(scaler->Initialize(frame, tw, th, WICBitmapInterpolationModeFant))) {
			src = scaler;
		}
		else {
			tw = w;
			th = h;
		}
		if (SUCCEEDED(factory->CreateFormatConverter(&conv)) &&
			SUCCEEDED(conv->Initialize(src, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom))) {
			auto img = std::make_shared<GbImage>();
			img->width = static_cast<int>(tw);
			img->height = static_cast<int>(th);
			img->bgra.resize(static_cast<size_t>(tw) * th);
			if (SUCCEEDED(conv->CopyPixels(nullptr, tw * 4, static_cast<UINT>(img->bgra.size() * 4), reinterpret_cast<BYTE *>(img->bgra.data())))) {
				out = img;
			}
		}
	}
	if (conv) conv->Release();
	if (scaler) scaler->Release();
	if (frame) frame->Release();
	if (decoder) decoder->Release();
	if (stream) stream->Release();
	if (factory) factory->Release();
	return out;
}

void ImageWorker()
{
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	for (;;) {
		std::string url;
		{
			std::unique_lock<std::mutex> lock(g_ioLock);
			g_ioImageCv.wait(lock, [] { return !g_ioImageQueue.empty(); });
			url = g_ioImageQueue.front();
			g_ioImageQueue.pop_front();
		}
		std::string body;
		std::shared_ptr<GbImage> img;
		const bool big = url.compare(0, 2, "B|") == 0;
		if (HttpGet(big ? url.substr(2) : url, {}, &body, "", false).empty()) img = DecodeImage(body, big ? 1600 : 320);
		if (!img) {
			static int logged = 0;
			if (logged++ < 3) LogW("download", "gamebanana: could not show the image %s", url.c_str());
			img = std::make_shared<GbImage>();
		}
		std::lock_guard<std::mutex> lock(g_ioLock);
		g_ioImages[url] = img;
		if (big) {
			g_bigOrder.push_back(url);
			while (g_bigOrder.size() > 4) {
				g_ioImages.erase(g_bigOrder.front());
				g_bigOrder.pop_front();
			}
		}
	}
}

bool InstalledGb(uint64_t id)
{
	for (const ModInfo &m : ModsList()) {
		if (m.gbId == id) return true;
	}
	return false;
}

void InstallGbDependencies(const GbMod &m, int depth = 0)
{
	if (depth > 4) return;
	for (uint64_t depId : m.deps) {
		if (InstalledGb(depId)) continue;
		GbMod dep;
		const std::string err = GbFetchMod(depId, dep);
		if (!err.empty() || !dep.hasFile) {
			LogW("download", "gamebanana: %s needs the mod %llu, which could not be requested (%s)", m.name.c_str(), static_cast<unsigned long long>(depId),
				err.empty() ? "no file" : err.c_str());
			continue;
		}
		InstallGbDependencies(dep, depth + 1);
		LogI("download", "gamebanana: %s needs %s; it is installed too", m.name.c_str(), dep.name.c_str());
		DlInstallEntry(dep.entry);
	}
}

void RunGbInstall(GbMod m)
{
	if (!m.details) {
		const std::string err = GbFetchMod(m.id, m);
		if (!err.empty() || !m.hasFile) {
			Finish(false, TrFmt("Could not request %s from GameBanana", {m.name}) + (err.empty() ? "" : ": " + err));
			return;
		}
	}
	InstallGbDependencies(m);
	DlInstallEntry(m.entry);
}

struct FilesEntry {
	std::shared_ptr<GbFiles> files;
	DWORD stamp = 0;
};
std::map<uint64_t, FilesEntry> g_ioFiles;

void FetchGbFiles(uint64_t id)
{
	auto r = std::make_shared<GbFiles>();
	r->loading = false;
	Json j;
	std::string err = HttpGetJson(std::string(kGbApi) + "/Mod/" + std::to_string(id) + "/ProfilePage", {}, j);
	if (err.empty() && !j.Get("_idRow")) err = j.GetString("_sErrorCode", Tr("empty response"));
	if (!err.empty()) {
		r->error = err;
		LogW("download", "gamebanana: could not read the versions of %llu (%s)", static_cast<unsigned long long>(id), err.c_str());
	}
	else {
		const std::string modVersion = j.GetString("_sVersion");
		if (const Json *files = j.Get("_aFiles")) {
			for (const Json &f : files->arr) {
				if (f.GetBool("_bIsArchived", false)) continue;
				GbFile file;
				file.id = static_cast<uint64_t>(f.GetNumber("_idRow"));
				file.version = f.GetString("_sVersion");
				file.bytes = static_cast<uint64_t>(f.GetNumber("_nFilesize"));
				file.date = static_cast<uint64_t>(f.GetNumber("_tsDateAdded"));
				file.md5 = Lower(f.GetString("_sMd5Checksum"));
				file.unversioned = file.version.empty();
				if (file.unversioned) file.version = GbFileVersion(f, modVersion, GbUsableFiles(files));
				if (file.id) r->files.push_back(file);
			}
		}
		std::sort(r->files.begin(), r->files.end(), [](const GbFile &a, const GbFile &b) { return a.date > b.date; });
		Json u;
		if (HttpGetJson(std::string(kGbApi) + "/Mod/" + std::to_string(id) + "/Updates?_nPage=1&_nPerpage=50", {}, u).empty()) {
			if (const Json *recs = u.Get("_aRecords")) {
				for (GbFile &f : r->files) {
					for (const Json &up : recs->arr) {
						const std::string name = up.GetString("_sName");
						if (!TitleHasVersion(name, f.version)) continue;
						std::string text = StripHtml(up.GetString("_sText"));
						if (const Json *cl = up.Get("_aChangeLog")) {
							for (const Json &c : cl->arr) {
								const std::string line = c.GetString("text", c.GetString("_sText"));
								if (!line.empty()) text += std::string(text.empty() ? "" : "\n") + "\xE2\x80\xA2 " + line;
							}
						}
						f.changelog = text;
						break;
					}
				}
			}
		}
	}
	std::lock_guard<std::mutex> lock(g_ioLock);
	g_ioFiles[id].files = r;
	g_ioFiles[id].stamp = GetTickCount();
}

void RunGbInstallVersion(GbMod m, GbFile f)
{
	if (!m.details) GbFetchMod(m.id, m);
	CatalogEntry e = m.entry;
	e.version = f.version;
	e.bytes = f.bytes;
	e.url = "https://gamebanana.com/dl/" + std::to_string(f.id);
	e.md5 = f.md5;
	e.aside = true;
	InstallGbDependencies(m);
	DlInstallEntry(e);
}

}

GbFiles GbGetFiles(uint64_t modId)
{
	std::lock_guard<std::mutex> lock(g_ioLock);
	FilesEntry &fe = g_ioFiles[modId];
	if (fe.files && (fe.files->error.empty() || GetTickCount() - fe.stamp < 5000)) return *fe.files;
	if (!fe.files || !fe.files->loading) {
		auto pending = std::make_shared<GbFiles>();
		fe.files = pending;
		fe.stamp = GetTickCount();
		SafeThread(FetchGbFiles, modId).detach();
	}
	return *fe.files;
}

bool GbChangelog(uint64_t modId, const std::string &version, std::string &text, std::string &foundVersion)
{
	GbGetFiles(modId);
	std::lock_guard<std::mutex> lock(g_ioLock);
	const auto it = g_ioFiles.find(modId);
	if (it == g_ioFiles.end() || !it->second.files || it->second.files->loading) return false;
	const GbFile *pick = nullptr;
	if (version.empty()) {
		pick = GbLatestFile(it->second.files->files);
	}
	else {
		for (const GbFile &f : it->second.files->files) {
			if (f.version == version) {
				pick = &f;
				break;
			}
		}
	}
	if (!pick || pick->changelog.empty()) return false;
	text = pick->changelog;
	foundVersion = pick->version;
	return true;
}

const GbFile *GbLatestFile(const std::vector<GbFile> &files)
{
	const GbFile *best = nullptr;
	for (const GbFile &f : files) {
		if (!best || FileIsNewer(f.unversioned ? "" : f.version, f.date, best->unversioned ? "" : best->version, best->date)) best = &f;
	}
	return best;
}

void GbInstallVersion(const GbMod &m, const GbFile &f)
{
	SafeThread(RunGbInstallVersion, m, f).detach();
}

void GbSearch(const GbQuery &q)
{
	uint64_t serial;
	{
		std::lock_guard<std::mutex> lock(g_ioLock);
		serial = ++g_ioWanted;
		g_ioResults.loading = true;
		g_ioResults.error.clear();
		g_ioResults.query = q;
	}
	SafeThread(RunSearch, q, serial).detach();
}

GbResults GbGetResults()
{
	std::lock_guard<std::mutex> lock(g_ioLock);
	GbResults r = g_ioResults;
	r.loading = r.serial != g_ioWanted;
	return r;
}

std::shared_ptr<const GbImage> GbBigImage(const std::string &url)
{
	if (url.compare(0, 8, "https://") != 0) return nullptr;
	const std::string key = "B|" + url;
	std::lock_guard<std::mutex> lock(g_ioLock);
	const auto it = g_ioImages.find(key);
	if (it != g_ioImages.end()) return it->second && it->second->width ? it->second : nullptr;
	g_ioImages[key] = nullptr;
	g_ioImageQueue.push_back(key);
	if (!g_ioImageThread) {
		g_ioImageThread = true;
		SafeThread(ImageWorker).detach();
	}
	g_ioImageCv.notify_one();
	return nullptr;
}

std::shared_ptr<const GbImage> GbLogo(const std::string &url)
{
	if (url.compare(0, 8, "https://") != 0) return nullptr;
	std::lock_guard<std::mutex> lock(g_ioLock);
	const auto it = g_ioImages.find(url);
	if (it != g_ioImages.end()) return it->second && it->second->width ? it->second : nullptr;
	g_ioImages[url] = nullptr;
	g_ioImageQueue.push_back(url);
	if (!g_ioImageThread) {
		g_ioImageThread = true;
		SafeThread(ImageWorker).detach();
	}
	g_ioImageCv.notify_one();
	return nullptr;
}

bool DlUpdateFor(const ModInfo &m, const std::vector<ModInfo> &installed, CatalogEntry &out)
{
	if (!DlFindUpdate(m.id, m.version, out, m.gbId)) return false;
	for (const ModInfo &o : installed) {
		if (o.folder != m.folder && ModsSameMod(o, m) && !UpdateIsNewer(out.version, o.version)) return false;
	}
	return true;
}

void DlUpdateMod(const std::string &folder, const CatalogEntry &e)
{
	SafeThread([folder, e] {
		CatalogEntry entry = e;
		entry.replaceFolder = folder;
		if (entry.gbId) {
			GbMod g;
			if (GbFetchMod(entry.gbId, g).empty()) InstallGbDependencies(g);
		}
		DlInstallEntry(entry);
	}).detach();
}

void GbInstall(const GbMod &m)
{
	if (!m.hasFile) return;
	SafeThread(RunGbInstall, m).detach();
}

namespace {
std::map<uint64_t, std::shared_ptr<const GbMod>> g_ioInfo;
std::map<uint64_t, DWORD> g_ioInfoAsked;
const DWORD kGbInfoRetryMs = 30000;

void FetchGbInfo(uint64_t id)
{
	auto m = std::make_shared<GbMod>();
	if (!GbFetchMod(id, *m).empty()) m = std::make_shared<GbMod>();
	std::lock_guard<std::mutex> lock(g_ioLock);
	g_ioInfo[id] = m;
}
}

std::shared_ptr<const GbMod> GbInfo(uint64_t id)
{
	if (!id) return nullptr;
	std::lock_guard<std::mutex> lock(g_ioLock);
	const auto it = g_ioInfo.find(id);
	if (it != g_ioInfo.end() && it->second && it->second->id) return it->second;
	if (it != g_ioInfo.end() && GetTickCount() - g_ioInfoAsked[id] < kGbInfoRetryMs) return nullptr;
	g_ioInfo[id] = nullptr;
	g_ioInfoAsked[id] = GetTickCount();
	SafeThread(FetchGbInfo, id).detach();
	return nullptr;
}

namespace {
std::mutex g_roomLock;
RoomModStatus g_room;
std::atomic<bool> g_roomCancel{false};

bool FindInstalledExact(uint64_t gbId, const std::string &id, const std::string &version, std::string &folder)
{
	for (const ModInfo &m : ModsList()) {
		const bool same = (gbId && m.gbId == gbId) || _stricmp(m.id.c_str(), id.c_str()) == 0;
		if (same && m.version == version) {
			folder = m.folder;
			return true;
		}
	}
	return false;
}

void RoomFail(const std::string &id, const std::string &why)
{
	LogW("content-share", "room mods: %s could not be downloaded from GameBanana (%s); it will be requested from the host", id.c_str(), why.c_str());
	std::lock_guard<std::mutex> lock(g_roomLock);
	g_room.failed.push_back(id);
	g_room.note = why;
}

void RoomModsThread(std::vector<RoomModRequest> reqs)
{
	uint64_t doneBytes = 0;
	for (size_t i = 0; i < reqs.size() && !g_roomCancel; i++) {
		const RoomModRequest &r = reqs[i];
		{
			std::lock_guard<std::mutex> lock(g_roomLock);
			g_room.index = static_cast<int>(i) + 1;
			g_room.what = r.id;
		}
		std::string folder;
		if (FindInstalledExact(r.gbId, r.id, r.version, folder)) {
			std::lock_guard<std::mutex> lock(g_roomLock);
			g_room.installed[r.id] = folder;
			continue;
		}
		if (!r.gbId) {
			RoomFail(r.id, Tr("not installed from GameBanana"));
			continue;
		}
		const GbFiles files = [&] {
			FetchGbFiles(r.gbId);
			std::lock_guard<std::mutex> lock(g_ioLock);
			return g_ioFiles[r.gbId].files ? *g_ioFiles[r.gbId].files : GbFiles();
		}();
		const GbFile *file = nullptr;
		for (const GbFile &f : files.files) {
			if (f.version == r.version) {
				file = &f;
				break;
			}
		}
		if (!file) {
			RoomFail(r.id, !files.error.empty() ? files.error : Tr("GameBanana does not have version ") + r.version);
			continue;
		}
		GbMod mod;
		GbFetchMod(r.gbId, mod);
		CatalogEntry e = mod.entry;
		e.source = "GameBanana";
		e.gbId = r.gbId;
		e.id = r.id;
		if (e.name.empty()) e.name = r.id;
		e.version = r.version;
		e.bytes = file->bytes;
		e.url = "https://gamebanana.com/dl/" + std::to_string(file->id);
		e.md5 = file->md5;
		e.aside = true;
		const uint64_t bytes = file->bytes;
		uint64_t before;
		{
			std::lock_guard<std::mutex> lock(g_lock);
			before = g_status.finished;
		}
		LogI("content-share", "room mods: %s %s is downloaded from GameBanana (%.1f MB)", e.name.c_str(), r.version.c_str(), bytes / 1048576.0);
		DlInstallEntry(e);
		const DWORD t0 = GetTickCount();
		for (;;) {
			Sleep(120);
			bool finished;
			{
				std::lock_guard<std::mutex> lock(g_lock);
				finished = g_status.finished > before;
				std::lock_guard<std::mutex> lock2(g_roomLock);
				g_room.got = doneBytes + g_status.done;
				g_room.total = std::max(doneBytes + (g_status.total ? g_status.total : bytes), g_room.got);
			}
			if (finished || g_roomCancel || GetTickCount() - t0 > 20 * 60 * 1000) break;
		}
		doneBytes += bytes;
		if (g_roomCancel) break;
		if (FindInstalledExact(r.gbId, r.id, r.version, folder)) {
			std::lock_guard<std::mutex> lock(g_roomLock);
			g_room.installed[r.id] = folder;
		}
		else {
			RoomFail(r.id, Tr("installation failed for version ") + r.version);
		}
	}
	std::lock_guard<std::mutex> lock(g_roomLock);
	g_room.active = false;
	g_room.done = !g_roomCancel;
}
}

void RoomModsFromGb(const std::vector<RoomModRequest> &mods)
{
	g_roomCancel = false;
	{
		std::lock_guard<std::mutex> lock(g_roomLock);
		g_room = RoomModStatus();
		g_room.active = true;
		g_room.count = static_cast<int>(mods.size());
	}
	SafeThread(RoomModsThread, mods).detach();
}

RoomModStatus RoomModsStatus()
{
	std::lock_guard<std::mutex> lock(g_roomLock);
	return g_room;
}

void RoomModsCancel()
{
	g_roomCancel = true;
	DlCancel();
	std::lock_guard<std::mutex> lock(g_roomLock);
	g_room.active = false;
}

std::string DlHttpGet(const std::string &url, std::string *toMemory, const std::string &toFile)
{
	return HttpGet(url, {}, toMemory, toFile, false);
}

std::string DlHttpGetJson(const std::string &url, Json &out)
{
	return HttpGetJson(url, {}, out);
}

std::string DlExtract(const std::string &archive, const std::string &dest)
{
	return Extract(archive, dest);
}

std::string DlFileSha256(const std::string &path)
{
	return HashFile(path, BCRYPT_SHA256_ALGORITHM);
}
