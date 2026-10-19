// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>

#include "core/hpfs.h"
#include "hpfs_builder.h"

namespace {
std::string Text(const std::vector<unsigned char> &v) { return std::string(v.begin(), v.end()); }

void ListsAndReads()
{
	HpfsBuilder b;
	const std::vector<unsigned char> bytes = b.Build({HpfsBuilder::File("game.xml", "<game/>"),
		HpfsBuilder::Dir("Maps", {HpfsBuilder::Dir("1", {HpfsBuilder::File("map.xml", "<map size=\"16384\"/>")})}),
		HpfsBuilder::File("player0.xml", "<playerdata/>")});
	HpfsFile f;
	CHECK(f.Open(bytes.data(), bytes.size()));
	CHECK_EQ(f.Entries().size(), static_cast<size_t>(5));
	std::vector<unsigned char> out;
	const HpfsEntry *e = f.Find("maps/1/MAP.XML");
	CHECK(e != nullptr);
	if (e) {
		CHECK(f.Read(*e, out));
		CHECK_EQ(Text(out), std::string("<map size=\"16384\"/>"));
	}
	CHECK(f.Find("Maps\\1\\map.xml") != nullptr);
	CHECK(f.Find("Maps") == nullptr);
	CHECK(f.Find("missing.xml") == nullptr);
	const HpfsEntry *dir = nullptr;
	for (const HpfsEntry &x : f.Entries()) {
		if (x.path == "Maps") dir = &x;
	}
	CHECK(dir != nullptr && dir->directory);
	if (dir) CHECK(!f.Read(*dir, out));
}

void BigStreams()
{
	for (size_t size : {size_t(40), size_t(2000), size_t(20000)}) {
		std::string payload(size, 'x');
		for (size_t i = 0; i < size; i++) payload[i] = static_cast<char>('a' + i % 23);
		HpfsBuilder b;
		const std::vector<unsigned char> bytes = b.Build({HpfsBuilder::File("big.bin", payload)});
		HpfsFile f;
		CHECK(f.Open(bytes.data(), bytes.size()));
		const HpfsEntry *e = f.Find("big.bin");
		CHECK(e != nullptr);
		std::vector<unsigned char> out;
		if (e) {
			CHECK(f.Read(*e, out));
			CHECK_EQ(Text(out), payload);
			CHECK(!f.Read(*e, out, static_cast<uint32_t>(size - 1)));
		}
	}
}

void RealPageSize()
{
	HpfsBuilder b;
	b.pageSize = 512;
	std::string payload(70000, 'q');
	const std::vector<unsigned char> bytes = b.Build({HpfsBuilder::File("grid", payload)});
	HpfsFile f;
	CHECK(f.Open(bytes.data(), bytes.size()));
	std::vector<unsigned char> out;
	const HpfsEntry *e = f.Find("grid");
	CHECK(e != nullptr);
	if (e) {
		CHECK(f.Read(*e, out));
		CHECK_EQ(out.size(), payload.size());
	}
}

void RejectsDamage()
{
	HpfsBuilder b;
	const std::vector<unsigned char> good = b.Build({HpfsBuilder::File("a.xml", "<a/>")});
	HpfsFile f;
	std::vector<unsigned char> bad = good;
	bad[0] = 'X';
	CHECK(!f.Open(bad.data(), bad.size()));
	CHECK(!f.Open(good.data(), 10));
	CHECK(!f.Open(nullptr, 0));
	bad = good;
	HpfsBuilder::Put32(&bad[4], 100);
	CHECK(!f.Open(bad.data(), bad.size()));
	bad = good;
	HpfsBuilder::Put32(&bad[0x1c], 999999);
	CHECK(!f.Open(bad.data(), bad.size()));
	bad.assign(good.begin(), good.begin() + good.size() / 2);
	CHECK(!f.Open(bad.data(), bad.size()));
}

void RejectsLoops()
{
	HpfsBuilder c;
	c.bytes.assign(c.pageSize, 0);
	memcpy(c.bytes.data(), "HPFS", 4);
	HpfsBuilder::Put32(&c.bytes[4], c.pageSize);
	const uint32_t handle = static_cast<uint32_t>(c.bytes.size() / c.pageSize);
	std::vector<unsigned char> dir(8, 0);
	HpfsBuilder::Put32(dir.data(), handle);
	dir[4] = 1;
	dir[6] = 4;
	dir.insert(dir.end(), {'s', 'e', 'l', 'f'});
	CHECK_EQ(c.AddStream(dir), handle);
	HpfsBuilder::Put32(&c.bytes[0x1c], handle);
	HpfsFile f;
	CHECK(!f.Open(c.bytes.data(), c.bytes.size()));
}
}

int main()
{
	ListsAndReads();
	BigStreams();
	RealPageSize();
	RejectsDamage();
	RejectsLoops();
	return test::Summary("hpfs");
}
