// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>
#include <vector>

#include "hooks/builtin_data.h"
#include "mods/patchxml.h"

namespace {

void AppliesAnywhere()
{
	int count = 0;
	const BuiltinText *texts = BuiltinPatchTexts(count);
	CHECK(count >= 3);
	int anywhere = 0;
	for (int i = 0; i < count; i++) {
		if (texts[i].gameFileSha256) continue;
		anywhere++;
		std::string data;
		std::vector<std::string> errors;
		CHECK(PatchIniText(data, texts[i].text, errors));
		CHECK(errors.empty());
		std::string again = data;
		errors.clear();
		PatchIniText(again, texts[i].text, errors);
		CHECK(errors.empty());
		CHECK_EQ(again, data);
	}
	CHECK(anywhere >= 2);
}

const BuiltinText *Find(const char *path)
{
	int count = 0;
	const BuiltinText *texts = BuiltinPatchTexts(count);
	for (int i = 0; i < count; i++) {
		if (std::string(texts[i].path) == path) return &texts[i];
	}
	return nullptr;
}

void FixesOnlyTheGameGaulTactic()
{
	const BuiltinText *fix = Find("data/ai/ts_gaultactic.vs");
	CHECK(fix != nullptr);
	if (!fix) return;
	CHECK(fix->gameFileSha256 != nullptr);
	if (!fix->gameFileSha256) return;
	const std::string sha = fix->gameFileSha256;
	CHECK_EQ(sha.size(), static_cast<size_t>(64));
	CHECK(sha.find_first_not_of("0123456789abcdef") == std::string::npos);
	std::string data = "\tbool bHorseshoe = false;\r\n\tint nIncPop = 0;\r\n\twhile(!bHorseshoe || olDruids.count<nDruids || nIncPop<2)\r\n\t{\r\n";
	std::vector<std::string> errors;
	CHECK(PatchScriptText(data, fix->text, errors));
	CHECK(errors.empty());
	CHECK_EQ(data, std::string("\tbool bHorseshoe = false;\r\n\tint nIncPop = 0;\r\n\twhile(!bHorseshoe || nIncPop<2)\r\n\t{\r\n"));
}

void KeepsTheMenuOfAPack()
{
	const BuiltinText *menu = Find("menuini/mainmenu.ini");
	CHECK(menu != nullptr);
	if (!menu) return;
	std::string data = "[MainMenu Objects]\nCaption\nUpdateBtn\nVersion\n\n[UpdateBtn]\nTemplate = %TmplIni%, ImgButton200\nText = News\nId = 0x1041\n\n"
					   "[Version]\nText = Some Mod 1.0\nId = 0x1022\n";
	std::vector<std::string> errors;
	CHECK(PatchIniText(data, menu->text, errors));
	CHECK(errors.empty());
	CHECK(data.find("Text = Additional content") != std::string::npos);
	CHECK(data.find("Text = News") == std::string::npos);
	CHECK(data.find("Text = Some Mod 1.0") != std::string::npos);
	CHECK(data.find("Caption\nUpdateBtn\nVersion") != std::string::npos);
}

}

int main()
{
	AppliesAnywhere();
	FixesOnlyTheGameGaulTactic();
	KeepsTheMenuOfAPack();
	return test::Summary("builtin");
}
