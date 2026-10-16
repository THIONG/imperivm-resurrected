// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct CatalogEntry {
	std::string source;
	std::string id;
	std::string name;
	std::string version;
	std::string description;
	std::string url;
	std::string sha256;
	std::string md5;
	std::string page;
	std::vector<std::string> authors;
	uint64_t bytes = 0;
	bool gameplay = false;
	uint64_t gbId = 0;
	std::vector<std::string> tags;
	std::string replaceFolder;
	bool aside = false;
};

struct DownloadStatus {
	bool active = false;
	std::string what;
	uint64_t done = 0;
	uint64_t total = 0;
	std::string message;
	bool lastOk = true;
	int queued = 0;
	uint64_t finished = 0;
};

void DlRefreshCatalogs();
std::vector<CatalogEntry> DlCatalog();
std::string DlCatalogStatus();
bool DlFindUpdate(const std::string &modId, const std::string &installedVersion, CatalogEntry &out, uint64_t gbId = 0);

void DlStartUpdateCheck();

void DlInstallEntry(const CatalogEntry &e);
void DlInstallUrl(const std::string &url);
void DlInstallArchive(const std::string &path);
std::string DlPickArchive();
DownloadStatus DlStatus();
void DlCancel();

void DlApplyPendingInstalls();

struct GbShot {
	std::string thumb;
	std::string big;
};
struct GbMod {
	std::vector<GbShot> shots;
	uint64_t id = 0;
	std::string name, summary, description, descriptionHtml, author, logoUrl, page, category;
	std::vector<std::string> tags;
	uint64_t downloads = 0, views = 0;
	int likes = 0;
	uint64_t added = 0, updated = 0;
	bool hasFile = false;
	bool details = false;
	bool dependencies = false;
	std::vector<uint64_t> deps;
	CatalogEntry entry;
};
struct GbQuery {
	std::string search;
	std::string tag;
	int sort = 0;
	int page = 0;
	int perPage = 8;
};
struct GbResults {
	bool loading = false;
	std::string error;
	GbQuery query;
	std::vector<GbMod> mods;
	int total = 0;
	uint64_t serial = 0;
};
struct GbImage {
	int width = 0, height = 0;
	std::vector<uint32_t> bgra;
};
bool GbAvailable();
void GbSearch(const GbQuery &q);
GbResults GbGetResults();
std::shared_ptr<const GbImage> GbLogo(const std::string &url);
std::shared_ptr<const GbImage> GbBigImage(const std::string &url);
void GbInstall(const GbMod &m);
struct GbFile {
	uint64_t id = 0;
	std::string version, changelog, md5;
	uint64_t bytes = 0;
	uint64_t date = 0;
};
struct GbFiles {
	bool loading = true;
	std::string error;
	std::vector<GbFile> files;
};
GbFiles GbGetFiles(uint64_t modId);
bool GbChangelog(uint64_t modId, const std::string &version, std::string &text, std::string &foundVersion);
void GbInstallVersion(const GbMod &m, const GbFile &f);

std::shared_ptr<const GbMod> GbInfo(uint64_t id);
std::string GbAddModPage();

struct RoomModRequest {
	std::string id;
	uint64_t gbId = 0;
	std::string version;
};
struct RoomModStatus {
	bool active = false, done = false;
	int index = 0, count = 0;
	std::string what;
	uint64_t got = 0, total = 0;
	std::map<std::string, std::string> installed;
	std::vector<std::string> failed;
	std::string note;
};
void RoomModsFromGb(const std::vector<RoomModRequest> &mods);
RoomModStatus RoomModsStatus();
void RoomModsCancel();

struct Json;
std::string DlHttpGet(const std::string &url, std::string *toMemory, const std::string &toFile);
std::string DlHttpGetJson(const std::string &url, Json &out);
std::string DlExtract(const std::string &archive, const std::string &dest);
std::string DlFileSha256(const std::string &path);
