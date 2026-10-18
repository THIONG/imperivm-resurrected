// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

struct PatchedValue {
	std::string file;
	std::string key;
	std::string label;
	std::string value;
	bool removed = false;
};

class XmlPatchRun {
public:
	explicit XmlPatchRun(const std::string &base);
	~XmlPatchRun();
	XmlPatchRun(const XmlPatchRun &) = delete;
	XmlPatchRun &operator=(const XmlPatchRun &) = delete;
	bool Ok() const;
	bool Apply(const std::string &patch, std::vector<std::string> &errors, std::vector<PatchedValue> *values = nullptr);
	std::string Text() const;

private:
	struct State;
	std::unique_ptr<State> state;
};

bool PatchXmlText(std::string &data, const std::string &patch, std::vector<std::string> &errors);

bool PatchIniText(std::string &data, const std::string &patch, std::vector<std::string> &errors, std::set<std::string> *removed = nullptr);

bool PatchScriptText(std::string &data, const std::string &patch, std::vector<std::string> &errors);

std::vector<PatchedValue> PatchValues(const std::string &file, const std::string &patch);

std::vector<std::vector<PatchedValue>> PatchXmlResolve(const std::string &file, const std::string &base, const std::vector<std::string> &before,
	const std::vector<std::string> &patches);
