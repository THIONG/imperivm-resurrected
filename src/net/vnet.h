// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>
#include <utility>
#include <vector>

typedef void (*VnetSendFn)(uint64_t peer, const void *data, int len);

void VnetInstallHooks(bool knownExe);
void VnetSetSender(VnetSendFn fn);
void VnetSetPeers(int myK, const std::vector<std::pair<uint64_t, int>> &peers);
bool VnetActive();
int VnetMyNumber();
void VnetPump(int timeoutMs);
void VnetDeliver(uint64_t from, const void *data, int len);
void VnetLogStats();
int VnetSilenceMs(uint64_t peer);
bool VnetHasPeer(uint64_t peer);
