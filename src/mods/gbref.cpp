// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "mods/gbref.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#include "core/json.h"

namespace {
const char kModsField[] = "gamebanana";
const char kSoundsField[] = "gamebananaSound";

uint64_t RowFromNumber(double n)
{
	if (!(n >= 1) || n >= static_cast<double>(kGbSoundFlag)) return 0;
	return static_cast<uint64_t>(n);
}

bool IsWebp(const std::string &name)
{
	return name.size() >= 5 && _stricmp(name.c_str() + name.size() - 5, ".webp") == 0;
}

void AddPicture(const Json &img, bool webp, std::vector<GbPicture> &out)
{
	const std::string base = img.GetString("_sBaseUrl"), file = img.GetString("_sFile");
	if (base.empty() || file.empty()) return;
	auto pick = [&](const char *field, const std::string &fallback) {
		const std::string name = img.GetString(field);
		return name.empty() || (IsWebp(name) && !webp) ? fallback : name;
	};
	GbPicture p;
	p.big = base + "/" + file;
	const std::string card = pick("_sFile530", file);
	p.card = base + "/" + card;
	p.thumb = base + "/" + pick("_sFile220", card);
	out.push_back(p);
}

uint64_t RowAfter(const std::string &url, const std::string &prefix)
{
	const size_t at = url.find(prefix);
	if (at == std::string::npos) return 0;
	uint64_t row = 0;
	for (size_t i = at + prefix.size(); i < url.size() && isdigit(static_cast<unsigned char>(url[i])); i++) {
		row = row * 10 + static_cast<uint64_t>(url[i] - '0');
		if (row >= kGbSoundFlag) return 0;
	}
	return row;
}
}

const char *GbApiBase() { return "https://gamebanana.com/apiv13"; }

const char *GbModel(uint64_t key) { return GbIsSound(key) ? "Sound" : "Mod"; }

std::string GbApiPath(uint64_t key) { return std::string("/") + GbModel(key) + "/" + std::to_string(GbRow(key)); }

std::string GbKeyText(uint64_t key) { return (GbIsSound(key) ? "sound " : "") + std::to_string(GbRow(key)); }

std::string GbCatalogId(uint64_t key) { return (GbIsSound(key) ? "gamebanana-sound-" : "gamebanana-") + std::to_string(GbRow(key)); }

uint64_t GbKeyFromUrl(const std::string &url)
{
	std::string lower = url;
	for (char &c : lower) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
	if (const uint64_t row = RowAfter(lower, "gamebanana.com/mods/")) return row;
	if (const uint64_t row = RowAfter(lower, "gamebanana.com/sounds/")) return GbSoundKey(row);
	return 0;
}

uint64_t GbKeyFromJson(const Json &o)
{
	if (const uint64_t row = RowFromNumber(o.GetNumber(kModsField))) return row;
	if (const uint64_t row = RowFromNumber(o.GetNumber(kSoundsField))) return GbSoundKey(row);
	return 0;
}

void GbKeyToJson(Json &o, uint64_t key)
{
	if (o.type != Json::Object || !GbRow(key)) return;
	o.obj.erase(std::remove_if(o.obj.begin(), o.obj.end(), [](const std::pair<std::string, Json> &kv) { return kv.first == kModsField || kv.first == kSoundsField; }),
		o.obj.end());
	o.Set(GbIsSound(key) ? kSoundsField : kModsField, Json::MakeNumber(static_cast<double>(GbRow(key))));
}

std::vector<GbPicture> GbPictures(const Json &submission, bool webp)
{
	std::vector<GbPicture> out;
	if (const Json *content = submission.Get("_aPreviewContent")) {
		if (const Json *list = content->Get("screenshots")) {
			for (const Json &img : list->arr) AddPicture(img, webp, out);
		}
		else if (const Json *one = content->Get("screenshot")) {
			AddPicture(*one, webp, out);
		}
	}
	if (!out.empty()) return out;
	if (const Json *media = submission.Get("_aPreviewMedia")) {
		if (const Json *images = media->Get("_aImages")) {
			for (const Json &img : images->arr) AddPicture(img, webp, out);
		}
	}
	return out;
}

bool GbSameVersionNewFile(unsigned catalogFiles, uint64_t catalogFile, uint64_t catalogDate, uint64_t installedFile, uint64_t installedDate)
{
	if (catalogFiles != 1 || !catalogFile || !installedFile) return false;
	return catalogFile != installedFile && catalogDate > installedDate;
}
