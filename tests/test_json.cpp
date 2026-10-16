// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include <windows.h>

#include "check.h"

#include <cstdio>
#include <string>

#include "core/json.h"
#include "i18n/tr.h"

static const std::string &TestLanguage()
{
	static const std::string english = "english";
	return english;
}

int main()
{
	TrSetLanguageSource(&TestLanguage);
	Json old;
	std::string err;
	Check(Json::Parse("{\"id\":\"x\",\"nombre\":{\"spanish\":\"Hola\"},\"autores\":[\"a\"],\"tipo\":\"reglas\",\"huellaJugabilidad\":\"abc\"}", old, &err), "parse");
	Check(old.Get("name") != nullptr, "name via nombre");
	Check(old.Get("authors") && old.Get("authors")->arr.size() == 1, "authors via autores");
	Check(old.GetString("kind") == "reglas", "kind via tipo");
	Check(old.GetString("gameplayFingerprint") == "abc", "fingerprint alias");
	old.Set("name", Json::MakeString("New"));
	Check(old.Get("nombre") == nullptr || old.GetString("name") == "New", "set replaces legacy");
	int names = 0;
	for (const auto &kv : old.obj) names += (kv.first == "name" || kv.first == "nombre") ? 1 : 0;
	Check(names == 1, "only one name key after Set");
	Json fresh;
	Check(Json::Parse("{\"name\":\"N\",\"nombre\":\"old\"}", fresh, &err), "parse 2");
	Check(fresh.GetString("name") == "N", "new key wins");
	Json bad;
	Check(!Json::Parse("{\"a\": }", bad, &err) && err.find("position") != std::string::npos, "error has position: " + err);
	return test::Summary("json");
}
