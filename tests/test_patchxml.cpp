// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>
#include <vector>

#include "mods/patchxml.h"

static const char *kCommands =
	"<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"
	"<commands>\n"
	"\t<!-- keep this comment -->\n"
	"\t<cmd name=\"trainRHastatus\" method=\"train\" costgold=\"100\" costpop=\"1\" execdelay=\"6000\">\n"
	"\t\t<src obj=\"RBarracks\"/>\n"
	"\t</cmd>\n"
	"\t<cmd name=\"trainRArcher\" key=\"A\" costgold=\"40\"/>\n"
	"\t<cmd name=\"other\" costgold=\"1\"/>\n"
	"</commands>\n";

static bool Contains(const std::string &data, const std::string &part)
{
	if (data.find(part) != std::string::npos) return true;
	std::printf("missing %s in:\n%s\n", test::Show(part).c_str(), data.c_str());
	return false;
}

static bool Lacks(const std::string &data, const std::string &part)
{
	if (data.find(part) == std::string::npos) return true;
	std::printf("unexpected %s in:\n%s\n", test::Show(part).c_str(), data.c_str());
	return false;
}

static bool Xml(std::string &data, const std::string &patch, std::vector<std::string> &errors)
{
	errors.clear();
	return PatchXmlText(data, patch, errors);
}

static void XmlSet()
{
	std::string data = kCommands;
	std::vector<std::string> errors;
	CHECK(Xml(data, "<patch><set select=\"/commands/cmd[@name='trainRHastatus']\" costgold=\"60\" execdelay=\"3000\" unset=\"costpop\"/></patch>", errors));
	CHECK(errors.empty());
	CHECK(Contains(data, "costgold=\"60\""));
	CHECK(Contains(data, "execdelay=\"3000\""));
	CHECK(Lacks(data, "costpop"));
	CHECK(Contains(data, "<!-- keep this comment -->"));
	CHECK(Contains(data, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>"));
	CHECK(Contains(data, "<cmd name=\"trainRArcher\" key=\"A\" costgold=\"40\"/>"));
	CHECK(Contains(data, "<src obj=\"RBarracks\"/>"));

	data = kCommands;
	CHECK(Xml(data, "<patch><set select=\"/commands/cmd[@name='trainRArcher']\" rollover=\"Archer\" key=\"Z\"/></patch>", errors));
	CHECK(Contains(data, "rollover=\"Archer\""));
	CHECK(Contains(data, "key=\"Z\""));
	CHECK(Lacks(data, "key=\"A\""));

	data = kCommands;
	CHECK(Xml(data, "<patch><set select=\"/commands/cmd\" extra=\"1\"/></patch>", errors));
	size_t count = 0;
	for (size_t at = data.find("extra=\"1\""); at != std::string::npos; at = data.find("extra=\"1\"", at + 1)) count++;
	CHECK_EQ(count, size_t(3));
}

static void XmlAdd()
{
	std::vector<std::string> errors;
	std::string data = kCommands;
	CHECK(Xml(data, "<patch><add select=\"/commands\"><cmd name=\"added\" costgold=\"5\"/></add></patch>", errors));
	CHECK(errors.empty());
	CHECK(Contains(data, "<cmd name=\"added\" costgold=\"5\"/>"));
	CHECK(data.find("added") > data.find("name=\"other\""));
	CHECK(Contains(data, "</commands>"));

	data = kCommands;
	CHECK(Xml(data, "<patch><add select=\"/commands\" position=\"start\"><cmd name=\"first\"/></add></patch>", errors));
	CHECK(data.find("name=\"first\"") < data.find("name=\"trainRHastatus\""));

	data = kCommands;
	CHECK(Xml(data, "<patch><add select=\"/commands/cmd[@name='trainRArcher']\" position=\"before\"><cmd name=\"b4\"/></add></patch>", errors));
	CHECK(data.find("name=\"b4\"") < data.find("name=\"trainRArcher\""));
	CHECK(data.find("name=\"b4\"") > data.find("name=\"trainRHastatus\""));

	data = kCommands;
	CHECK(Xml(data, "<patch><add select=\"/commands/cmd[@name='trainRArcher']\" position=\"after\"><cmd name=\"af\"/></add></patch>", errors));
	CHECK(data.find("name=\"af\"") > data.find("name=\"trainRArcher\""));
	CHECK(data.find("name=\"af\"") < data.find("name=\"other\""));

	data = kCommands;
	CHECK(Xml(data, "<patch><add select=\"//cmd[@name='trainRHastatus']\"><src obj=\"Second\"/></add></patch>", errors));
	CHECK(data.find("Second") > data.find("RBarracks"));
	CHECK(data.find("Second") < data.find("name=\"trainRArcher\""));

	data = kCommands;
	CHECK(Xml(data, "<patch><add select=\"/commands\"><cmd name=\"one\"/><cmd name=\"two\"/></add></patch>", errors));
	CHECK(data.find("name=\"one\"") < data.find("name=\"two\""));
}

static void XmlRemoveReplace()
{
	std::vector<std::string> errors;
	std::string data = kCommands;
	CHECK(Xml(data, "<patch><remove select=\"//cmd[@name='other']\"/></patch>", errors));
	CHECK(errors.empty());
	CHECK(Lacks(data, "name=\"other\""));
	CHECK(Contains(data, "name=\"trainRArcher\""));
	CHECK(Contains(data, "</commands>"));

	data = kCommands;
	CHECK(Xml(data, "<patch><remove select=\"/commands/cmd[@key]\"/></patch>", errors));
	CHECK(Lacks(data, "trainRArcher"));
	CHECK(Contains(data, "trainRHastatus"));

	data = kCommands;
	CHECK(Xml(data, "<patch><replace select=\"/commands/cmd[@name='trainRHastatus']/src\"><src obj=\"RBarracks2\"/></replace></patch>", errors));
	CHECK(Lacks(data, "obj=\"RBarracks\""));
	CHECK(Contains(data, "obj=\"RBarracks2\""));

	data = kCommands;
	CHECK(Xml(data, "<patch><replace select=\"/commands/cmd[@name='other']\"><cmd name=\"x\"/><cmd name=\"y\"/></replace></patch>", errors));
	CHECK(Lacks(data, "name=\"other\""));
	CHECK(data.find("name=\"x\"") < data.find("name=\"y\""));
}

static void XmlSelectors()
{
	std::vector<std::string> errors;
	std::string data = kCommands;
	CHECK(Xml(data, "<patch><set select=\"/commands/cmd[2]\" mark=\"two\"/></patch>", errors));
	CHECK(data.find("mark=\"two\"") > data.find("trainRArcher") - 1);
	CHECK(data.find("mark=\"two\"") < data.find("name=\"other\""));

	data = kCommands;
	CHECK(Xml(data, "<patch><set select=\"//CMD[@NAME='OTHER']\" mark=\"ci\"/></patch>", errors));
	CHECK(Contains(data, "mark=\"ci\""));

	data = kCommands;
	CHECK(Xml(data, "<patch><set select='//cmd[@name=\"other\"]' mark=\"dq\"/></patch>", errors));
	CHECK(Contains(data, "mark=\"dq\""));

	data = kCommands;
	CHECK(Xml(data, "<patch><set select=\"//*[@name='trainRArcher']\" mark=\"any\"/></patch>", errors));
	CHECK(Contains(data, "mark=\"any\""));

	data = kCommands;
	CHECK(Xml(data, "<patch><set select=\"/commands/cmd[@costgold][3]\" mark=\"chain\"/></patch>", errors));
	CHECK(data.find("mark=\"chain\"") > data.find("name=\"other\""));

	data = kCommands;
	CHECK(!Xml(data, "<patch><set select=\"/commands/cmd[@name='x'\" mark=\"1\"/></patch>", errors));
	CHECK_EQ(errors.size(), size_t(1));

	data = kCommands;
	CHECK(!Xml(data, "<patch><set mark=\"1\"/></patch>", errors));
	CHECK_EQ(errors.size(), size_t(1));
}

static void XmlErrors()
{
	std::vector<std::string> errors;
	std::string data = kCommands;
	CHECK(!Xml(data, "<patch><set select=\"/commands/cmd[@name='missing']\" a=\"1\"/></patch>", errors));
	CHECK_EQ(errors.size(), size_t(1));
	CHECK_EQ(data, kCommands);

	CHECK(!Xml(data, "<patch><set select=\"/commands/cmd[@name='missing']\" a=\"1\" optional=\"yes\"/></patch>", errors));
	CHECK(errors.empty());
	CHECK(!Xml(data, "<patch><set select=\"/commands/cmd[@name='missing']\" a=\"1\" optional=\"si\"/></patch>", errors));
	CHECK(errors.empty());

	CHECK(Xml(data, "<patch><set select=\"/commands/cmd[@name='missing']\" a=\"1\"/><set select=\"/commands/cmd[@name='other']\" ok=\"1\"/></patch>", errors));
	CHECK_EQ(errors.size(), size_t(1));
	CHECK(Contains(data, "ok=\"1\""));

	data = kCommands;
	CHECK(!Xml(data, "<patch><set select=\"/commands\" a=\"1\"></patch>", errors));
	CHECK_EQ(errors.size(), size_t(1));
	CHECK_EQ(data, kCommands);

	CHECK(!Xml(data, "just text", errors));
	CHECK_EQ(errors.size(), size_t(1));

	CHECK(!Xml(data, "<patch><explode select=\"/commands\"/></patch>", errors));
	CHECK_EQ(errors.size(), size_t(1));

	CHECK(!Xml(data, "<patch><add select=\"/commands\"/></patch>", errors));
	CHECK_EQ(errors.size(), size_t(1));

	CHECK(!Xml(data, "<patch><add select=\"/commands\" position=\"middle\"><cmd name=\"q\"/></add></patch>", errors));
	CHECK_EQ(errors.size(), size_t(1));

	std::string broken = "<commands><cmd name=\"a\"></commands>";
	CHECK(!Xml(broken, "<patch><set select=\"/commands\" a=\"1\"/></patch>", errors));
	CHECK_EQ(errors.size(), size_t(1));
	CHECK_EQ(broken, "<commands><cmd name=\"a\"></commands>");

	CHECK(!Xml(data, "<patch/>", errors));
}

static void XmlFidelity()
{
	std::vector<std::string> errors;
	std::string data = std::string("\xEF\xBB\xBF") + kCommands;
	CHECK(Xml(data, "<patch><set select=\"/commands/cmd[@name='other']\" costgold=\"2\"/></patch>", errors));
	CHECK(data.compare(0, 3, "\xEF\xBB\xBF") == 0);
	CHECK(Contains(data, "costgold=\"2\""));

	data = kCommands;
	CHECK(Xml(data, "<patch><set select=\"/commands/cmd[@name='other']\" rollover=\"Fish &amp; Chips &lt;1&gt;\"/></patch>", errors));
	CHECK(Contains(data, "rollover=\"Fish &amp; Chips &lt;1&gt;\""));

	data = "<root>\r\n  <a x=\"1\"/>\r\n</root>\r\n";
	CHECK(Xml(data, "<patch><set select=\"/root/a\" x=\"2\"/></patch>", errors));
	CHECK_EQ(data, "<root>\r\n  <a x=\"2\"/>\r\n</root>\r\n");

	data = "<root><a x=\"1\"/><!-- c --><![CDATA[ raw <b> ]]></root>";
	CHECK(Xml(data, "<patch><set select=\"/root/a\" x=\"2\"/></patch>", errors));
	CHECK_EQ(data, "<root><a x=\"2\"/><!-- c --><![CDATA[ raw <b> ]]></root>");

	data = kCommands;
	std::string twice = kCommands;
	const std::string patch = "<patch><set select=\"/commands/cmd[@name='other']\" costgold=\"9\"/></patch>";
	CHECK(Xml(data, patch, errors));
	CHECK(Xml(twice, patch, errors));
	CHECK(Xml(twice, patch, errors));
	CHECK_EQ(data, twice);
}

static void XmlStacking()
{
	std::vector<std::string> errors;
	std::string data = kCommands;
	CHECK(Xml(data, "<patch><set select=\"/commands/cmd[@name='other']\" costgold=\"2\"/></patch>", errors));
	CHECK(Xml(data, "<patch><set select=\"/commands/cmd[@name='other']\" costfood=\"3\"/></patch>", errors));
	CHECK(Xml(data, "<patch><add select=\"/commands\"><cmd name=\"third\"/></add></patch>", errors));
	CHECK(Contains(data, "costgold=\"2\""));
	CHECK(Contains(data, "costfood=\"3\""));
	CHECK(Contains(data, "name=\"third\""));
	CHECK(Contains(data, "name=\"trainRHastatus\""));
}

static const char *kIni =
	"; game constants\r\n"
	"[GamePlay]\r\n"
	"DamageAmplify = 200 ;old comment\r\n"
	"TeachingLevel2 = 5\r\n"
	"\r\n"
	"[GameOptions Objects]\r\n"
	"RSText\r\n"
	"RButton\r\n";

static bool Ini(std::string &data, const std::string &patch, std::vector<std::string> &errors)
{
	errors.clear();
	return PatchIniText(data, patch, errors);
}

static void IniBasics()
{
	std::vector<std::string> errors;
	std::string data = kIni;
	CHECK(Ini(data, "[GamePlay]\nDamageAmplify = 250\n", errors));
	CHECK(errors.empty());
	CHECK(Contains(data, "DamageAmplify = 250\r\n"));
	CHECK(Lacks(data, "old comment"));
	CHECK(Contains(data, "TeachingLevel2 = 5\r\n"));
	CHECK(Contains(data, "; game constants\r\n"));

	data = kIni;
	CHECK(Ini(data, "[gameplay]\ndamageamplify=300\n", errors));
	CHECK(Contains(data, "damageamplify = 300\r\n"));
	CHECK(Lacks(data, "DamageAmplify"));

	data = kIni;
	CHECK(Ini(data, "[GamePlay]\nNewKey = 1\n", errors));
	CHECK(data.find("NewKey = 1") > data.find("TeachingLevel2"));
	CHECK(data.find("NewKey = 1") < data.find("[GameOptions Objects]"));

	data = kIni;
	CHECK(Ini(data, "[Brand New]\nA = 1\nB = 2\n", errors));
	CHECK(Contains(data, "[Brand New]\r\nA = 1\r\n"));
	CHECK(data.find("B = 2") > data.find("A = 1"));
}

static void IniRemoveAdd()
{
	std::vector<std::string> errors;
	std::string data = kIni;
	CHECK(Ini(data, "[GamePlay]\n-TeachingLevel2\n", errors));
	CHECK(errors.empty());
	CHECK(Lacks(data, "TeachingLevel2"));
	CHECK(Contains(data, "DamageAmplify"));

	data = kIni;
	CHECK(Ini(data, "[GameOptions Objects]\n-RSText\n+MyButton\n+RButton\n", errors));
	CHECK(errors.empty());
	CHECK(Lacks(data, "RSText"));
	CHECK(Contains(data, "MyButton\r\n"));
	size_t count = 0;
	for (size_t at = data.find("RButton"); at != std::string::npos; at = data.find("RButton", at + 1)) count++;
	CHECK_EQ(count, size_t(1));

	data = kIni;
	CHECK(!Ini(data, "[GamePlay]\n-Missing\n", errors));
	CHECK_EQ(errors.size(), size_t(1));
	CHECK_EQ(data, kIni);

	data = kIni;
	CHECK(Ini(data, "[Fresh]\n+Line\n", errors));
	CHECK(Contains(data, "[Fresh]\r\nLine\r\n"));
}

static void IniErrorsAndFormat()
{
	std::vector<std::string> errors;
	std::string data = kIni;
	CHECK(!Ini(data, "DamageAmplify = 1\n", errors));
	CHECK_EQ(errors.size(), size_t(1));
	CHECK(!Ini(data, "[GamePlay]\nnot a pair\n", errors));
	CHECK_EQ(errors.size(), size_t(1));
	CHECK(!Ini(data, "; only a comment\n\n", errors));
	CHECK(errors.empty());
	CHECK_EQ(data, kIni);

	data = "[A]\nk = 1\n";
	CHECK(Ini(data, "[A]\nk = 2\n", errors));
	CHECK_EQ(data, "[A]\nk = 2\n");

	data = std::string("\xEF\xBB\xBF") + "[A]\r\nk = 1\r\n";
	CHECK(Ini(data, "[A]\r\nk = 2\r\n", errors));
	CHECK_EQ(data, std::string("\xEF\xBB\xBF") + "[A]\r\nk = 2\r\n");

	data = "[A]\r\nk = 1";
	CHECK(Ini(data, "[A]\nj = 3\n", errors));
	CHECK(Contains(data, "j = 3"));

	data = kIni;
	CHECK(Ini(data, "[GamePlay]\nDamageAmplify = 250 ;trailing\n", errors));
	CHECK(Contains(data, "DamageAmplify = 250 ;trailing\r\n"));
}

static const char *kScript =
	"// GetTacticScript\r\n"
	"int nRace;\r\n"
	"int player;\r\n"
	"nRace = set.GetCentralBuilding.race;\r\n"
	"player = set.player;\r\n"
	"return TS_Default;\r\n";

static bool Script(std::string &data, const std::string &patch, std::vector<std::string> &errors)
{
	errors.clear();
	return PatchScriptText(data, patch, errors);
}

static void ScriptDirectives()
{
	std::vector<std::string> errors;
	std::string data = kScript;
	CHECK(Script(data, "@before nRace = set.GetCentralBuilding.race;\n// MOD\nif (x) return 1;\n\n@after player = set.player;\nLog(1);\n", errors));
	CHECK(errors.empty());
	CHECK_EQ(data,
		"// GetTacticScript\r\n"
		"int nRace;\r\n"
		"int player;\r\n"
		"// MOD\r\n"
		"if (x) return 1;\r\n"
		"nRace = set.GetCentralBuilding.race;\r\n"
		"player = set.player;\r\n"
		"Log(1);\r\n"
		"return TS_Default;\r\n");

	data = kScript;
	CHECK(Script(data, "@replace return TS_Default\nreturn TS_Other;\n", errors));
	CHECK(Contains(data, "return TS_Other;\r\n"));
	CHECK(Lacks(data, "TS_Default"));

	data = kScript;
	CHECK(Script(data, "@top\nint extra;\n@bottom\n// tail\n", errors));
	CHECK(data.find("int extra;") == std::string("// GetTacticScript\r\n").size());
	CHECK(Contains(data, "return TS_Default;\r\n// tail\r\n"));

	data = kScript;
	CHECK(Script(data, "@after int player;\nint a;\n@end\nthis is ignored\n@before return\nint b;\n", errors));
	CHECK(Lacks(data, "this is ignored"));
	CHECK(Contains(data, "int a;"));
	CHECK(Contains(data, "int b;\r\nreturn TS_Default;"));

	data = "int a;\nreturn 1;\n";
	CHECK(Script(data, "@before return\nint z;\n", errors));
	CHECK_EQ(data, "int a;\nint z;\nreturn 1;\n");
}

static void ScriptLegacyAndErrors()
{
	std::vector<std::string> errors;
	std::string data = kScript;
	CHECK(Script(data, "@antes return TS_Default\nint viejo;\n@despues int player;\nint tras;\n", errors));
	CHECK(errors.empty());
	CHECK(Contains(data, "int viejo;\r\nreturn TS_Default;"));
	CHECK(Contains(data, "int player;\r\nint tras;"));

	data = kScript;
	CHECK(!Script(data, "@before no such line\nint z;\n", errors));
	CHECK_EQ(errors.size(), size_t(1));
	CHECK_EQ(data, kScript);

	CHECK(!Script(data, "@explode\nint z;\n", errors));
	CHECK_EQ(errors.size(), size_t(1));

	CHECK(!Script(data, "@before\nint z;\n", errors));
	CHECK_EQ(errors.size(), size_t(1));

	CHECK(!Script(data, "@bottom\n\n\n", errors));
	CHECK_EQ(errors.size(), size_t(1));

	CHECK(Script(data, "@before no such line\nint z;\n@bottom\n// ok\n", errors));
	CHECK_EQ(errors.size(), size_t(1));
	CHECK(Contains(data, "// ok"));
}

int main()
{
	XmlSet();
	XmlAdd();
	XmlRemoveReplace();
	XmlSelectors();
	XmlErrors();
	XmlFidelity();
	XmlStacking();
	IniBasics();
	IniRemoveAdd();
	IniErrorsAndFormat();
	ScriptDirectives();
	ScriptLegacyAndErrors();
	return test::Summary("patchxml");
}
