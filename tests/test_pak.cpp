// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <cstring>

#include "mods/pak.h"

struct Item {
	std::string name;
	uint32_t offset;
	uint32_t size;
};

static std::string BuildPak(const std::vector<Item> &items, const char *magic = "HMMSYS PackFile")
{
	std::string data(0x28, '\0');
	memcpy(&data[0], magic, strlen(magic));
	const uint32_t count = static_cast<uint32_t>(items.size());
	memcpy(&data[0x20], &count, 4);
	std::string prev;
	for (const Item &it : items) {
		size_t shared = 0;
		while (shared < prev.size() && shared < it.name.size() && prev[shared] == it.name[shared]) shared++;
		data += static_cast<char>(it.name.size());
		data += static_cast<char>(shared);
		data += it.name.substr(shared);
		data.append(reinterpret_cast<const char *>(&it.offset), 4);
		data.append(reinterpret_cast<const char *>(&it.size), 4);
		prev = it.name;
	}
	return data;
}

static void Valid(const std::string &dir)
{
	const std::vector<Item> items = {
		{"data/commands/barracks.xml", 100, 2000},
		{"data/commands/blacksmith.xml", 2100, 3000},
		{"data/const.ini", 5100, 77},
		{"units/rhastatus.xml", 5177, 1},
		{"units/rhastatus.pass", 5178, 0},
	};
	const std::string path = dir + "\\valid.pak";
	test::WriteFile(path, BuildPak(items));
	std::vector<PakEntry> out;
	CHECK(ReadPakIndex(path, out));
	CHECK_EQ(out.size(), items.size());
	for (size_t i = 0; i < out.size() && i < items.size(); i++) {
		CHECK_EQ(out[i].name, items[i].name);
		CHECK_EQ(out[i].offset, items[i].offset);
		CHECK_EQ(out[i].size, items[i].size);
	}
}

static void Empty(const std::string &dir)
{
	const std::string path = dir + "\\empty.pak";
	test::WriteFile(path, BuildPak({}));
	std::vector<PakEntry> out = {PakEntry()};
	CHECK(ReadPakIndex(path, out));
	CHECK(out.empty());
}

static void Invalid(const std::string &dir)
{
	std::vector<PakEntry> out;
	CHECK(!ReadPakIndex(dir + "\\missing.pak", out));
	CHECK(out.empty());

	test::WriteFile(dir + "\\magic.pak", BuildPak({{"a.txt", 1, 2}}, "NOT A PACKFILE"));
	CHECK(!ReadPakIndex(dir + "\\magic.pak", out));

	test::WriteFile(dir + "\\short.pak", "HMMSYS PackFile");
	CHECK(!ReadPakIndex(dir + "\\short.pak", out));

	std::string cut = BuildPak({{"data/a.xml", 1, 2}, {"data/b.xml", 3, 4}});
	cut.resize(cut.size() - 5);
	test::WriteFile(dir + "\\cut.pak", cut);
	CHECK(!ReadPakIndex(dir + "\\cut.pak", out));

	std::string bad = BuildPak({{"data/a.xml", 1, 2}, {"data/b.xml", 3, 4}});
	bad[0x28 + 1] = 5;
	test::WriteFile(dir + "\\bad.pak", bad);
	CHECK(!ReadPakIndex(dir + "\\bad.pak", out));

	std::string tooMany = BuildPak({{"a", 1, 1}});
	const uint32_t count = 1000;
	memcpy(&tooMany[0x20], &count, 4);
	test::WriteFile(dir + "\\many.pak", tooMany);
	CHECK(!ReadPakIndex(dir + "\\many.pak", out));
}

int main()
{
	const std::string dir = test::TempDir("pak");
	Valid(dir);
	Empty(dir);
	Invalid(dir);
	test::RemoveTree(dir);
	return test::Summary("pak");
}
