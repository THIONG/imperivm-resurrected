// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once

#include <string>

#include "core/json.h"

void ModVarsClear();
bool ModVarsHas(const std::string &key);
int ModVarsGet(const std::string &key);
bool ModVarsSet(const std::string &key, int value);
int ModVarsCount();

Json ModVarsToJson();
void ModVarsFromJson(const Json &obj);

void ModVarsPendingFromSave(const std::string &gamePath);
bool ModRulesFromSaveTake(unsigned &rules, unsigned long &tick);
bool ModRulesFromSavePeek(unsigned &rules, unsigned long &tick);

void InstallModVars(bool knownExe);
