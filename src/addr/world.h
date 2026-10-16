// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>

namespace addr {

inline constexpr uintptr_t kTileLineVisit = 0x0040b1b0;
inline constexpr uintptr_t kTraceTileLine = 0x0040b2b0;
inline constexpr uintptr_t kComputeNeeds = 0x0041c0e0;
inline constexpr uintptr_t kCreatePlayerAI = 0x00434ca0;
inline constexpr uintptr_t kBestTemple = 0x00435750;
inline constexpr uintptr_t kGetClassCount = 0x004441e0;
inline constexpr uintptr_t kAiUpdate = 0x00448dd0;
inline constexpr uintptr_t kAiAddOrder = 0x004494c0;
inline constexpr uintptr_t kCatapultByRace = 0x004e3b30;
inline constexpr uintptr_t kDiplomacyExecute = 0x004e5f80;
inline constexpr uintptr_t kQueryGetResults = 0x004f9490;
inline constexpr uintptr_t kCalcAttackOutcome = 0x0050ff30;
inline constexpr uintptr_t kProcessDeath = 0x005141b0;
inline constexpr uintptr_t kIsExplored = 0x00515950;
inline constexpr uintptr_t kSetPaused = 0x0051d2e0;
inline constexpr uintptr_t kSetCurrentPlayer = 0x0051d5b0;
inline constexpr uintptr_t kGameUpdate = 0x0051e940;
inline constexpr uintptr_t kShowPausedMenu = 0x0051ee80;
inline constexpr uintptr_t kRandomDrawSite = 0x005223b5;
inline constexpr uintptr_t kDeclareDefeated = 0x00522f90;
inline constexpr uintptr_t kVisionRun = 0x0053e910;
inline constexpr uintptr_t kSetPosition = 0x0053f000;
inline constexpr uintptr_t kStartVictoryScripts = 0x00541020;
inline constexpr uintptr_t kMapSettlementSite = 0x0054ed96;
inline constexpr uintptr_t kMapSettlementSkip = 0x0054ee6d;
inline constexpr uintptr_t kRequestPause = 0x005503b0;
inline constexpr uintptr_t kRequestUnpause = 0x00550400;
inline constexpr uintptr_t kRequestPauseOnline = 0x00550450;
constexpr uintptr_t kCrashFixSites[] = {0x0055f500, 0x0057177c, 0x005717f6};
inline constexpr uintptr_t kGenerateRandomMap = 0x00587e60;
inline constexpr uintptr_t kPlaceTownTemplate = 0x0058e3c0;
inline constexpr uintptr_t kFindTownTemplate = 0x00590bd0;
inline constexpr uintptr_t kFindVillageTemplate = 0x00590ec0;
inline constexpr uintptr_t kSingleSettingsCtor = 0x00597370;
inline constexpr uintptr_t kPersistMpCheck = 0x00597da9;
inline constexpr uintptr_t kSaveGameToFile = 0x005983c0;
inline constexpr uintptr_t kLoadGameFromFile = 0x005984b0;
inline constexpr uintptr_t kGetRaceName = 0x005a7b90;
inline constexpr uintptr_t kGetRaceLetter = 0x005a7c00;
inline constexpr uintptr_t kBuildStatsText = 0x005b3380;
inline constexpr uintptr_t kOnKilled = 0x005b5ca0;
inline constexpr uintptr_t kLoadScrollSettings = 0x005b7fe0;
inline constexpr uintptr_t kMapScrollHook = 0x005b8100;
inline constexpr uintptr_t kIsDemoPlayback = 0x005bde80;
inline constexpr uintptr_t kCreateSettings = 0x005bfb90;
inline constexpr uintptr_t kMakeRandomName = 0x005bff50;
inline constexpr uintptr_t kSettlementSetPlayer = 0x005c4eb0;
inline constexpr uintptr_t kScoreVisibleUpdate = 0x005cae60;
inline constexpr uintptr_t kScoreVisibleDraw = 0x005cb100;
inline constexpr uintptr_t kInfobarSwitch = 0x005e18f0;
inline constexpr uintptr_t kSetInterfaceHidden = 0x005e31e0;
inline constexpr uintptr_t kClearSelection = 0x005e7e50;
inline constexpr uintptr_t kCreateEntityImage = 0x0060cb20;
inline constexpr uintptr_t kRebuild = 0x00616c40;
inline constexpr uintptr_t kScreenToMap = 0x006243b0;
inline constexpr uintptr_t kCenterOn = 0x00627300;
inline constexpr uintptr_t kMapViewOnKey = 0x00627ef0;
inline constexpr uintptr_t kGetRaceFolderName = 0x006c2fa0;
inline constexpr uintptr_t kDiplomacyApply = 0x006cb680;
inline constexpr uintptr_t kMultiHumans = 0x006cc1c0;
inline constexpr uintptr_t kBuildQuickSavePath = 0x006cc700;
inline constexpr uintptr_t kRequestQuitToMenu = 0x006cebf0;
inline constexpr uintptr_t kRequestLoadGame = 0x00749990;
inline constexpr uintptr_t kProcessPendingGameRequests = 0x00749fd0;
inline constexpr uintptr_t kAiVtable = 0x007ac76c;
inline constexpr uintptr_t kSquadVtable = 0x007b13c8;
inline constexpr uintptr_t kSettlementVtable = 0x007cea80;
inline constexpr uintptr_t kShipVtable = 0x007cf598;
inline constexpr uintptr_t kScrollSpeedMin = 0x00824688;
inline constexpr uintptr_t kScrollSpeedMax = 0x0082468c;
inline constexpr uintptr_t kScrollStepMax = 0x00824690;
inline constexpr uintptr_t kScrollRefresh = 0x00824694;
inline constexpr uintptr_t kScrollEdge = 0x00824698;
inline constexpr uintptr_t kScrollInitialWait = 0x0082469c;
inline constexpr uintptr_t kScrollStepAlign = 0x008246a0;
inline constexpr uintptr_t kGameSpecialNames = 0x00824bf0;
inline constexpr uintptr_t kScrollToken = 0x009bdb78;
inline constexpr uintptr_t kScrollBits = 0x009bdb84;
inline constexpr uintptr_t kZoomMapGlobal = 0x009d2cc0;

}
