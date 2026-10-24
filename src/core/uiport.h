// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct UiPort {
	void (*toast)(const std::string &text, bool error) = nullptr;
	void (*notice)(const std::string &title, const std::string &text) = nullptr;
	void (*askRestart)(const std::string &reason, int largeAddress, const std::vector<std::string> &items, const std::string &itemsTitle) = nullptr;
	bool (*modManagerOpen)() = nullptr;
	bool (*multiplayerOpen)() = nullptr;
	bool (*askDropPlayers)(const std::vector<std::string> &names, int positions) = nullptr;
	bool (*hostSave)(const std::string &path) = nullptr;
	void (*rejoinStarting)() = nullptr;
	void (*rejoinInGame)() = nullptr;
	void (*refreshMainMenuButton)() = nullptr;
	void (*joinFromDiscord)(uint64_t lobbyId) = nullptr;
	void (*openControls)() = nullptr;
	void (*wakeGameThread)() = nullptr;
};

void UiPortSet(const UiPort &port);

void UiToast(const std::string &text, bool error = false);
void UiNotice(const std::string &title, const std::string &text);
void UiAskRestart(const std::string &reason = "", int largeAddress = -1, const std::vector<std::string> &items = {},
	const std::string &itemsTitle = "");
bool UiModManagerOpen();
bool UiMultiplayerOpen();
bool UiAskDropPlayers(const std::vector<std::string> &names, int positions);
bool UiHostSave(const std::string &path);
void UiRejoinStarting();
void UiRejoinInGame();
void UiRefreshMainMenuButton();
void UiJoinFromDiscord(uint64_t lobbyId);
void UiOpenControls();
void UiWakeGameThread();
