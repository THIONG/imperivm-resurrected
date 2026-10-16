// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>
#pragma warning(push)
#pragma warning(disable : 4996 4828)
#pragma warning(pop)
#include <algorithm>
#include "core/log.h"
#include "mods/mods.h"
#include "net/mpsave.h"
#include "net/rejoin.h"
#include "i18n/tr.h"

#include "net/steamnet_internal.h"

using namespace steamnet_impl;

namespace steamnet_impl {
namespace {
std::string GameFolder()
{
	char path[MAX_PATH];
	GetModuleFileNameA(nullptr, path, MAX_PATH);
	char *slash = strrchr(path, '\\');
	if (slash) *slash = 0;
	return path;
}

std::string g_savePublishedFile;

const char kSaveTransferPath[] = "#save";

std::string ScenarioFile(const std::string &path)
{
	if (path == kSaveTransferPath) return GameFolder() + "\\SaveOnline\\room.snr";
	std::string p = path;
	for (char &c : p) {
		if (c == '/') c = '\\';
	}
	if (_strnicmp(p.c_str(), "Scenarios\\", 10) != 0 || p.find("..") != std::string::npos || p.find(':') != std::string::npos) return "";
	return GameFolder() + "\\" + p + ".bfhp";
}

bool FileFingerprint(const std::string &file, uint64_t &size, std::string &hash)
{
	HANDLE h = CreateFileA(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (h == INVALID_HANDLE_VALUE) return false;
	uint64_t fnv = 1469598103934665603ULL;
	size = 0;
	std::vector<uint8_t> buf(64 * 1024);
	DWORD got = 0;
	while (ReadFile(h, buf.data(), static_cast<DWORD>(buf.size()), &got, nullptr) && got) {
		for (DWORD i = 0; i < got; i++) fnv = (fnv ^ buf[i]) * 1099511628211ULL;
		size += got;
	}
	CloseHandle(h);
	char hex[17];
	sprintf_s(hex, "%016llx", static_cast<unsigned long long>(fnv));
	hash = hex;
	return true;
}

std::string g_scenPublished;
}
}

namespace steamnet_impl {
void PublishScenario(const std::string &path)
{
	if (!g_lobby || path == g_scenPublished) return;
	g_scenPublished = path;
	std::string value, modId, modVersion, modName;
	if (!path.empty()) {
		uint64_t size = 0;
		std::string hash;
		const std::string file = ScenarioFile(path);
		const bool loose = !file.empty() && FileFingerprint(file, size, hash);
		value = loose ? path + "\t" + std::to_string(size) + "\t" + hash : path + "\t0\t";
		if (!loose && ModsScenarioProvider(path, modId, modVersion, modName)) {
			for (std::string *field : {&modId, &modVersion, &modName}) std::replace(field->begin(), field->end(), '\t', ' ');
			value += "\t" + modId + "\t" + modVersion + "\t" + modName;
		}
	}
	PublishMods(modId);
	S.SteamAPI_ISteamMatchmaking_SetLobbyData(g_steam.mm, g_lobby, "scen", value.c_str());
	{
		std::lock_guard<std::mutex> lock(g_lock);
		g_share.scenarioValue = value;
	}
	LogI("room", "scenario of the room: %s", path.empty() ? "(random map)" : value.c_str());
}
}

namespace steamnet_impl {
namespace {
void HostOnMapRequest(uint64 peer, const std::string &path)
{
	const bool save = path == kSaveTransferPath;
	const std::string file = save ? g_savePublishedFile : ScenarioFile(path);
	uint64_t size = 0;
	std::string hash;
	if (file.empty() || (!save && path != g_scenPublished) || !FileFingerprint(file, size, hash) || size > 64 * 1024 * 1024) {
		LogW("room", "scenarios: \"%s\" is not sent to %s", path.c_str(), Name(peer).c_str());
		return;
	}
	for (size_t i = 0; i < g_share.out.size();) {
		OutTransfer &o = g_share.out[i];
		const bool sameMap = o.peer == peer && !o.mods && !o.steps.empty() && o.steps.back().type == kShMapEnd && o.steps.back().text == path;
		if (!sameMap) {
			i++;
			continue;
		}
		if (o.file != INVALID_HANDLE_VALUE) CloseHandle(o.file);
		LogW("room", "scenarios: previous sending of \"%s\" to %s discarded (they ask again)", path.c_str(), Name(peer).c_str());
		g_share.out.erase(g_share.out.begin() + i);
	}
	OutTransfer t;
	t.peer = peer;
	t.steps.push_back({kShMapFile, path + "\t" + std::to_string(size), file});
	t.steps.push_back({kShMapEnd, path, ""});
	g_share.out.push_back(std::move(t));
	LogI("room", "scenarios: sending \"%s\" (%llu bytes) to %s", path.c_str(), size, Name(peer).c_str());
}

struct MapIn {
	uint64 host = 0;
	std::string handled;
	std::string handledSave;
	bool isSave = false;
	std::string path, part;
	HANDLE file = INVALID_HANDLE_VALUE;
};
MapIn g_mapIn;
}
}

namespace steamnet_impl {
}

namespace steamnet_impl {
namespace {
OnlineMapStatus &InStatus()
{
	return g_mapIn.isSave ? g_share.saveStatus : g_share.mapStatus;
}

void MapFailed(const std::string &why)
{
	if (g_mapIn.file != INVALID_HANDLE_VALUE) CloseHandle(g_mapIn.file);
	g_mapIn.file = INVALID_HANDLE_VALUE;
	if (!g_mapIn.part.empty()) DeleteFileA(g_mapIn.part.c_str());
	std::lock_guard<std::mutex> lock(g_lock);
	InStatus().state = 2;
	InStatus().error = why;
	LogW("room", "scenarios: could not receive (%s)", why.c_str());
}
}
}

namespace steamnet_impl {
void CheckRoomScenario()
{
	if (!g_lobby || S.SteamAPI_ISteamMatchmaking_GetLobbyOwner(g_steam.mm, g_lobby) == g_steam.me) return;
	const std::string value = Data(g_lobby, "scen");
	if (value == g_mapIn.handled) return;
	g_mapIn.handled = value;
	const std::vector<std::string> f = SplitTabs(value);
	OnlineMapStatus st;
	if (f.size() >= 3 && !f[0].empty()) {
		st.name = f[0].substr(f[0].find_last_of("/\\") + 1);
		const uint64_t size = _strtoui64(f[1].c_str(), nullptr, 10);
		const std::string file = ScenarioFile(f[0]);
		const bool fromMod = f.size() >= 6 && !f[3].empty();
		uint64_t mySize = 0;
		std::string myHash;
		if (fromMod && size == 0) {
			const uint64 host = S.SteamAPI_ISteamMatchmaking_GetLobbyOwner(g_steam.mm, g_lobby);
			if (ModsMountedHasExact(f[3], f[4])) {
				ShareSend(host, kShMapHave, value.data(), value.size());
			}
			else {
				LogW("room", "scenarios: the map \"%s\" comes from \"%s\" %s, which is not loaded", f[0].c_str(), f[5].c_str(), f[4].c_str());
				if (g_lobby != g_pendingJoin.syncLobby) Notice(TrFmt("The room map comes from \"%s\", which you do not have loaded: leave the room and join again to download it", {f[5]}));
			}
		}
		else if (size && !file.empty() && !(FileFingerprint(file, mySize, myHash) && mySize == size && myHash == f[2])) {
			g_mapIn.host = S.SteamAPI_ISteamMatchmaking_GetLobbyOwner(g_steam.mm, g_lobby);
			st.state = 1;
			st.total = size;
			ShareSend(g_mapIn.host, kShMapReq, f[0].data(), f[0].size());
			LogI("room", "scenarios: requesting \"%s\" from the host", f[0].c_str());
		}
		else if (size) {
			const uint64 host = S.SteamAPI_ISteamMatchmaking_GetLobbyOwner(g_steam.mm, g_lobby);
			ShareSend(host, kShMapHave, value.data(), value.size());
		}
	}
	std::lock_guard<std::mutex> lock(g_lock);
	g_share.mapStatus = st;
}

void CheckRoomSave()
{
	if (!g_lobby || S.SteamAPI_ISteamMatchmaking_GetLobbyOwner(g_steam.mm, g_lobby) == g_steam.me) return;
	const std::string value = Data(g_lobby, "save");
	if (value == g_mapIn.handledSave) return;
	g_mapIn.handledSave = value;
	const std::vector<std::string> f = SplitTabs(value);
	OnlineMapStatus st;
	if (f.size() >= 3 && !f[0].empty()) {
		st.name = f[0];
		const uint64_t size = _strtoui64(f[1].c_str(), nullptr, 10);
		uint64_t mySize = 0;
		std::string myHash;
		if (!(FileFingerprint(ScenarioFile(kSaveTransferPath), mySize, myHash) && mySize == size && myHash == f[2])) {
			g_mapIn.host = S.SteamAPI_ISteamMatchmaking_GetLobbyOwner(g_steam.mm, g_lobby);
			st.state = 1;
			st.total = size;
			ShareSend(g_mapIn.host, kShMapReq, kSaveTransferPath, strlen(kSaveTransferPath));
			LogD("match", "online game: requesting the saved game \"%s\" from the host", f[0].c_str());
		}
		else {
			st.ready = true;
		}
	}
	std::lock_guard<std::mutex> lock(g_lock);
	g_share.saveStatus = st;
}

void PublishSave(const std::string &file, const std::string &name)
{
	if (!g_lobby) return;
	g_savePublishedFile = file;
	uint64_t size = 0;
	std::string hash, value;
	if (!file.empty() && FileFingerprint(file, size, hash)) value = name + "\t" + std::to_string(size) + "\t" + hash;
	S.SteamAPI_ISteamMatchmaking_SetLobbyData(g_steam.mm, g_lobby, "save", value.c_str());
	std::string cams;
	FILE *f = nullptr;
	if (!value.empty() && fopen_s(&f, (file + ".cameras").c_str(), "rb") == 0 && f) {
		char buf[1024] = {};
		const size_t n = fread(buf, 1, sizeof(buf) - 1, f);
		fclose(f);
		cams.assign(buf, n);
	}
	S.SteamAPI_ISteamMatchmaking_SetLobbyData(g_steam.mm, g_lobby, "cams", cams.c_str());
	LogI("match", "online game: the room loads %s", value.empty() ? "(nothing)" : ("\"" + name + "\"").c_str());
}
}

namespace steamnet_impl {
namespace {
bool ClientOnMapMessage(uint64 peer, const uint8_t *data, size_t len)
{
	if (!len || peer != g_mapIn.host) return false;
	const uint8_t type = data[0];
	const std::string text(reinterpret_cast<const char *>(data + 1), len - 1);
	if (type == kShMapFile) {
		const std::vector<std::string> f = SplitTabs(text);
		const std::string file = f.empty() ? "" : ScenarioFile(f[0]);
		if (file.empty()) {
			MapFailed("the host sent a disallowed path");
			return true;
		}
		if (g_mapIn.file != INVALID_HANDLE_VALUE) {
			CloseHandle(g_mapIn.file);
			g_mapIn.file = INVALID_HANDLE_VALUE;
			LogI("room", "scenarios: the previous download was left half done; starting over");
		}
		g_mapIn.isSave = f[0] == kSaveTransferPath;
		g_mapIn.path = file;
		g_mapIn.part = file + ".partial";
		MakeDirs(file.substr(0, file.rfind('\\')));
		g_mapIn.file = CreateFileA(g_mapIn.part.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (g_mapIn.file == INVALID_HANDLE_VALUE) MapFailed("could not create the file");
		return true;
	}
	if (type == kShData && g_mapIn.file != INVALID_HANDLE_VALUE) {
		DWORD wrote = 0;
		if (!WriteFile(g_mapIn.file, data + 1, static_cast<DWORD>(len - 1), &wrote, nullptr)) {
			MapFailed("could not write");
			return true;
		}
		std::lock_guard<std::mutex> lock(g_lock);
		InStatus().got += len - 1;
		return true;
	}
	if (type == kShMapEnd && g_mapIn.file != INVALID_HANDLE_VALUE) {
		CloseHandle(g_mapIn.file);
		g_mapIn.file = INVALID_HANDLE_VALUE;
		if (GetFileAttributesA(g_mapIn.path.c_str()) != INVALID_FILE_ATTRIBUTES) {
			MoveFileExA(g_mapIn.path.c_str(), (g_mapIn.path + ".previous").c_str(), MOVEFILE_REPLACE_EXISTING);
		}
		if (!MoveFileExA(g_mapIn.part.c_str(), g_mapIn.path.c_str(), MOVEFILE_REPLACE_EXISTING)) {
			MapFailed("could not save the scenario");
			return true;
		}
		LogI("room", "scenarios: received %s", g_mapIn.path.c_str());
		if (!g_mapIn.isSave && !g_mapIn.handled.empty()) {
			ShareSend(g_mapIn.host, kShMapHave, g_mapIn.handled.data(), g_mapIn.handled.size());
		}
		ModsNoteReceivedMap(g_mapIn.path);
		std::lock_guard<std::mutex> lock(g_lock);
		InStatus().state = 0;
		InStatus().ready = true;
		return true;
	}
	return false;
}
}
}

namespace steamnet_impl {
void ResetRoomScenario()
{
	{
		std::lock_guard<std::mutex> lock(g_lock);
		g_share.scenarioValue.clear();
		g_share.mapHave.clear();
	}
	if (g_scenPublished.empty() && g_mapIn.handled.empty() && g_mapIn.handledSave.empty() && g_savePublishedFile.empty() &&
		g_mapIn.file == INVALID_HANDLE_VALUE) {
		return;
	}
	if (g_mapIn.file != INVALID_HANDLE_VALUE) {
		CloseHandle(g_mapIn.file);
		DeleteFileA(g_mapIn.part.c_str());
	}
	g_mapIn = MapIn();
	g_scenPublished.clear();
	g_savePublishedFile.clear();
	std::lock_guard<std::mutex> lock(g_lock);
	g_share.mapStatus = OnlineMapStatus();
	g_share.saveStatus = OnlineMapStatus();
}

void ReceiveShare()
{
	SteamNetworkingMessage_t *in[32];
	const int n = S.SteamAPI_ISteamNetworkingMessages_ReceiveMessagesOnChannel(g_steam.msgs, kShareChannel, in, 32);
	for (int i = 0; i < n; i++) {
		const uint64 peer = in[i]->m_identityPeer.GetSteamID64();
		const uint8_t *data = static_cast<const uint8_t *>(in[i]->m_pData);
		const size_t len = static_cast<size_t>(in[i]->m_cbSize);
		if (len && data[0] == kShReq) {
			if (g_lobby && S.SteamAPI_ISteamMatchmaking_GetLobbyOwner(g_steam.mm, g_lobby) == g_steam.me) {
				HostOnRequest(peer, std::string(reinterpret_cast<const char *>(data + 1), len - 1));
			}
		}
		else if (len && data[0] == kShSeat) {
			if (g_lobby && S.SteamAPI_ISteamMatchmaking_GetLobbyOwner(g_steam.mm, g_lobby) == g_steam.me) {
				const int slot = atoi(std::string(reinterpret_cast<const char *>(data + 1), len - 1).c_str());
				std::lock_guard<std::mutex> lock(g_seats.mx);
				if (g_seats.requests.size() < 32) g_seats.requests.push_back({peer, slot});
			}
		}
		else if (len && data[0] == kShSwap) {
			bool member = false;
			const int n = g_lobby ? S.SteamAPI_ISteamMatchmaking_GetNumLobbyMembers(g_steam.mm, g_lobby) : 0;
			for (int k = 0; k < n; k++) {
				if (S.SteamAPI_ISteamMatchmaking_GetLobbyMemberByIndex(g_steam.mm, g_lobby, k) == peer) member = true;
			}
			if (member && len < 128) {
				std::lock_guard<std::mutex> lock(g_seats.mx);
				if (g_seats.swaps.size() < 64) g_seats.swaps.push_back({peer, std::string(reinterpret_cast<const char *>(data + 1), len - 1)});
			}
		}
		else if (len && data[0] == kShMapHave) {
			if (g_lobby && S.SteamAPI_ISteamMatchmaking_GetLobbyOwner(g_steam.mm, g_lobby) == g_steam.me && len < 1024) {
				std::lock_guard<std::mutex> lock(g_lock);
				g_share.mapHave[peer] = std::string(reinterpret_cast<const char *>(data + 1), len - 1);
			}
		}
		else if (len && data[0] == kShProgress) {
			if (g_lobby && S.SteamAPI_ISteamMatchmaking_GetLobbyOwner(g_steam.mm, g_lobby) == g_steam.me && len < 64) {
				unsigned long long got = 0, total = 0;
				const std::string t(reinterpret_cast<const char *>(data + 1), len - 1);
				if (sscanf_s(t.c_str(), "%llu %llu", &got, &total) == 2 && total) {
					std::lock_guard<std::mutex> lock(g_lock);
					g_share.extProgress[peer] = {std::min<uint64_t>(got, total), total, GetTickCount()};
				}
			}
		}
		else if (len && data[0] == kShCam) {
			int player = 0, x = 0, y = 0;
			const std::string t(reinterpret_cast<const char *>(data + 1), len - 1);
			if (sscanf_s(t.c_str(), "%d %d %d", &player, &x, &y) == 3) MpSaveSetPlayerCamera(player, x, y);
		}
		else if (len && data[0] >= 40 && data[0] < 60) {
			const bool fromViewer = g_spectate.lobby && InLobby(g_spectate.lobby, peer);
			const bool fromHost = g_lobby && Data(g_lobby, "spec") == "1" && S.SteamAPI_ISteamMatchmaking_GetLobbyOwner(g_steam.mm, g_lobby) == peer;
			if (fromViewer || fromHost) SpectatorOnSteamMessage(peer, data[0], std::string(reinterpret_cast<const char *>(data + 1), len - 1));
		}
		else if (len && data[0] >= 20 && data[0] < 40) {
			RejoinOnSteamMessage(peer, data[0], std::string(reinterpret_cast<const char *>(data + 1), len - 1));
		}
		else if (len && data[0] == kShMapReq) {
			if (g_lobby && S.SteamAPI_ISteamMatchmaking_GetLobbyOwner(g_steam.mm, g_lobby) == g_steam.me) {
				HostOnMapRequest(peer, std::string(reinterpret_cast<const char *>(data + 1), len - 1));
			}
		}
		else if (!ClientOnMapMessage(peer, data, len)) {
			ClientOnMessage(peer, data, len);
		}
		in[i]->Release();
	}
	PumpOut();
}
}
