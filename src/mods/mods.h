// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

inline constexpr int kScriptApiVersion = 6;

struct ModDependency {
	std::string id;
	std::string version;
};

struct ModLanguage {
	std::string code;
	std::string name;
	std::string base;
};

struct ModOrderLabel {
	std::string file;
	std::vector<std::pair<std::string, std::string>> names;
};

enum ModKind { kModCosmetic = 0, kModContent = 1, kModRules = 2 };
const char *ModsKindId(int kind);
int ModsKindFromId(const std::string &id);
int ModsRulesSchema();

struct ModInfo {
	std::string folder;
	std::string id;
	std::string name;
	std::string version;
	std::string description;
	std::string license;
	std::vector<std::string> authors;
	bool redistributable = false;
	bool hasManifest = false;
	std::string manifestError;
	std::vector<ModDependency> requires;
	std::vector<std::string> incompatible;
	std::string sourceUrl;
	uint64_t gbId = 0;
	std::vector<std::string> loadAfter;
	std::vector<std::string> loadBefore;
	std::vector<std::string> tags;
	std::string category;
	std::vector<ModLanguage> languages;
	std::vector<ModOrderLabel> orderLabels;

	std::vector<std::string> paths;
	std::vector<std::string> forbidden;
	bool hasLoose = false;
	bool gameplay = false;
	bool content = false;
	int kind = kModCosmetic;
	int declaredKind = -1;
	uint64_t totalBytes = 0;
	unsigned fileCount = 0;
	std::string hash;
	std::string gameplayHash;
};

inline bool ModsSameMod(const ModInfo &a, const ModInfo &b)
{
	return _stricmp(a.id.c_str(), b.id.c_str()) == 0 || (a.gbId && a.gbId == b.gbId);
}

struct ModProfile {
	std::string name;
	std::vector<std::string> mods;
};

void ModsScan();
std::vector<ModInfo> ModsList();
bool ModsFind(const std::string &folder, ModInfo &out);

std::vector<std::string> ModsProfileNames();
ModProfile ModsLoadProfile(const std::string &name);
bool ModsSaveProfile(const ModProfile &p);
bool ModsDeleteProfile(const std::string &name);
std::string ModsActiveProfileName();
void ModsSetActiveProfileName(const std::string &name);
const ModProfile &ModsMountedProfile();
bool ModsScenarioProvider(const std::string &scenarioPath, std::string &id, std::string &version, std::string &name);
bool ModsMountedHasExact(const std::string &id, const std::string &version);
bool ModsRestartNeeded();
bool ModsRenameProfile(const std::string &from, const std::string &to);
std::vector<std::string> ModsProfilesUsing(const std::string &folder);
std::string ModsUninstall(const std::string &folder);
void ModsNoteReceivedMap(const std::string &fullPath);
bool ModsUninstallPending(const std::string &folder);
void ModsApplyPendingUninstalls();
int ModsCompareVersions(const std::string &a, const std::string &b);
void ModsQueueSwap(const std::string &newFolder, const std::string &oldFolder);
void ModsApplyPendingSwaps();

struct ModConflict {
	std::string path;
	std::vector<std::string> folders;
};
std::vector<ModConflict> ModsConflicts(const ModProfile &p);
std::vector<std::string> ModsProblems(const ModProfile &p);
std::string ModsProfileGameplayHash(const ModProfile &p);

void ModsRestartGame(int largeAddress = -1, const std::string &args = "");
void ModsPatchExeAtExit(int largeAddress);
bool ModsExeIsLargeAddressAware();
std::string ModsPreparePublish(const std::string &folder);
void ModsOpenFolder(const std::string &folder);

void ModsOnSaveWritten(const std::string &gamePath, bool online = false);
std::string ModsCheckSaveOnLoad(const std::string &gamePath);
void ModsOnMapWritten(const std::string &gamePath);

struct ModNeed {
	std::string folder, id, name, version;
	bool installed = false;
	std::string installedFolder;
	bool inProfile = false;
};
struct ModNeeds {
	std::vector<ModNeed> mods;
	std::string profile;
	std::vector<std::string> folders;
	bool Satisfied() const;
	bool AllInstalled() const;
	std::string Text(bool onlyMissing) const;
	std::vector<std::string> Names(bool onlyMissing) const;
};
ModNeeds ModsNeededFor(const std::string &gamePath, const std::string &profile = "");
std::string ModsActivateProfileFor(const ModNeeds &needs, const std::string &label);
ModProfile ModsLooseProfileWith(const std::vector<std::string> &folders);

std::vector<std::string> ModsMountedLocalDirs();
std::vector<std::string> ModsMountedDirs();
const std::string &ModsLanguage();
struct Json;
std::string ModsLocalizedText(const Json &j, const char *key, const std::string &def);

void InstallModsHooks(bool knownExe);

bool ModsAllowedFile(const std::string &relPath);
std::string ModsFolderPath();

std::vector<std::string> ModsAutoOrder(const std::vector<std::string> &folders);
size_t ModsInsertPosition(const std::vector<std::string> &folders, const std::string &folder);
struct ModIssues {
	std::vector<std::string> problems;
	int overwrites = 0;
	std::vector<std::string> overlapWith;
	std::vector<std::string> overlapFolders;
};
ModIssues ModsIssuesFor(const ModProfile &p, const std::string &folder);
