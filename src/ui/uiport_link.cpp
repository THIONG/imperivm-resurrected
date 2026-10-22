// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "core/uiport.h"
#include "ui/controlsui.h"
#include "ui/modui.h"
#include "ui/mpui.h"

namespace {

const bool g_registered = [] {
	UiPort port;
	port.toast = &ModUiToast;
	port.notice = &ModUiNotice;
	port.askRestart = &ModUiAskRestart;
	port.modManagerOpen = &ModUiIsOpen;
	port.multiplayerOpen = &MpUiIsOpen;
	port.askDropPlayers = &MpUiAskDropPlayers;
	port.hostSave = &MpUiHostSave;
	port.rejoinStarting = &MpUiRejoinStarting;
	port.rejoinInGame = &MpUiRejoinInGame;
	port.refreshMainMenuButton = &MpUiRefreshMainMenuButton;
	port.joinFromDiscord = &MpUiJoinFromDiscord;
	port.openControls = &ControlsUiOpen;
	port.wakeGameThread = &MpUiWakeGameThread;
	UiPortSet(port);
	return true;
}();

}
