// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <windows.h>

#include "mods/gbcache.h"

static void Keys()
{
	CHECK_EQ(GbCacheKey("https://images.gamebanana.com/a.jpg").size(), 16u);
	CHECK(GbCacheKey("a") != GbCacheKey("b"));
	CHECK_EQ(GbCacheKey("a"), GbCacheKey("a"));
	const std::string path = GbCachePath("C:\\cache", "x");
	CHECK(path.compare(0, 9, "C:\\cache\\") == 0);
	CHECK(path.size() > 4 && path.compare(path.size() - 4, 4, ".bin") == 0);
}

static void Freshness()
{
	CHECK(GbCacheIsFresh(1000, 1100, 600));
	CHECK(!GbCacheIsFresh(1000, 1600, 600));
	CHECK(!GbCacheIsFresh(1000, 1601, 600));
	CHECK(GbCacheIsFresh(1000, 99999999, 0));
	CHECK(!GbCacheIsFresh(2000, 1000, 600));
}

static void RoundTrip()
{
	const std::string dir = test::TempDir("gbcache");
	std::string body;
	bool stale = true;
	CHECK(!GbCacheRead(dir, "u1", 600, body, stale, 1000));
	CHECK(GbCacheWrite(dir, "u1", std::string("hello\0world", 11), 1000));
	CHECK(GbCacheRead(dir, "u1", 600, body, stale, 1100));
	CHECK_EQ(body, std::string("hello\0world", 11));
	CHECK(!stale);
	CHECK(GbCacheRead(dir, "u1", 600, body, stale, 2000));
	CHECK(stale);
	CHECK_EQ(body.size(), 11u);
	CHECK(GbCacheRead(dir, "u1", 0, body, stale, 999999));
	CHECK(!stale);
	CHECK(GbCacheWrite(dir, "u1", "second", 3000));
	CHECK(GbCacheRead(dir, "u1", 600, body, stale, 3100));
	CHECK_EQ(body, std::string("second"));
	CHECK(GbCacheWrite(dir, "empty", "", 3000));
	CHECK(GbCacheRead(dir, "empty", 600, body, stale, 3000));
	CHECK(body.empty());
	GbCacheRemove(dir, "u1");
	CHECK(!GbCacheRead(dir, "u1", 600, body, stale, 3100));
	GbCacheClear(dir);
	CHECK(!GbCacheRead(dir, "empty", 600, body, stale, 3100));
}

static void Corrupt()
{
	const std::string dir = test::TempDir("gbcache_bad");
	CHECK(GbCacheWrite(dir, "x", "data", 1000));
	const std::string path = GbCachePath(dir, "x");
	HANDLE h = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	CHECK(h != INVALID_HANDLE_VALUE);
	DWORD n = 0;
	WriteFile(h, "ZZZZ", 4, &n, nullptr);
	CloseHandle(h);
	std::string body;
	bool stale = false;
	CHECK(!GbCacheRead(dir, "x", 600, body, stale, 1100));
}

static std::string Slurp(const std::string &path)
{
	std::string out;
	FILE *f = nullptr;
	if (fopen_s(&f, path.c_str(), "rb") == 0 && f) {
		char buf[4096];
		size_t n;
		while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out.append(buf, n);
		fclose(f);
	}
	return out;
}

static void Damaged()
{
	const std::string dir = test::TempDir("gbcache_damaged");
	CHECK(GbCacheWrite(dir, "https://x/a.json", "0123456789", 1000));
	const std::string path = GbCachePath(dir, "https://x/a.json");
	const std::string whole = Slurp(path);
	CHECK_EQ(whole.size(), static_cast<size_t>(32 + 16 + 10));
	std::string body;
	bool stale = false;
	for (size_t keep : {static_cast<size_t>(0), static_cast<size_t>(10), static_cast<size_t>(31), static_cast<size_t>(32 + 5), whole.size() - 1}) {
		test::WriteFile(path, whole.substr(0, keep));
		CHECK(!GbCacheRead(dir, "https://x/a.json", 600, body, stale, 1100));
	}
	test::WriteFile(path, whole + "extra");
	CHECK(!GbCacheRead(dir, "https://x/a.json", 600, body, stale, 1100));
	test::WriteFile(path, whole);
	CHECK(GbCacheRead(dir, "https://x/a.json", 600, body, stale, 1100));
	CHECK_EQ(body, std::string("0123456789"));
	const std::string other = GbCachePath(dir, "https://x/b.json");
	test::WriteFile(other, whole);
	CHECK(!GbCacheRead(dir, "https://x/b.json", 600, body, stale, 1100));
	std::string wrongVersion = whole;
	wrongVersion[4] = 9;
	test::WriteFile(path, wrongVersion);
	CHECK(!GbCacheRead(dir, "https://x/a.json", 600, body, stale, 1100));
}

static std::vector<GbCacheEntry> Files(std::initializer_list<std::pair<int64_t, uint64_t>> list)
{
	std::vector<GbCacheEntry> out;
	int i = 0;
	for (const auto &p : list) out.push_back({"f" + std::to_string(i++), p.second, p.first});
	return out;
}

static void Prune()
{
	const int64_t day = 86400;
	const int64_t now = 100 * day;
	CHECK(GbCachePlanPrune(Files({{now - day, 10}, {now - 2 * day, 10}}), 100, 30 * day, now).empty());
	std::vector<std::string> idle = GbCachePlanPrune(Files({{now - 40 * day, 10}, {now - day, 10}, {now - 31 * day, 10}}), 1000, 30 * day, now);
	CHECK_EQ(idle.size(), 2u);
	CHECK_EQ(idle[0], std::string("f0"));
	CHECK_EQ(idle[1], std::string("f2"));
	std::vector<std::string> size = GbCachePlanPrune(Files({{now - 1 * day, 40}, {now - 3 * day, 40}, {now - 2 * day, 40}}), 70, 30 * day, now);
	CHECK_EQ(size.size(), 2u);
	CHECK_EQ(size[0], std::string("f1"));
	CHECK_EQ(size[1], std::string("f2"));
	CHECK_EQ(GbCachePlanPrune(Files({{now, 500}}), 100, 30 * day, now).size(), 1u);
	CHECK(GbCachePlanPrune({}, 100, 30 * day, now).empty());
}

static void PruneOnDisk()
{
	const std::string dir = test::TempDir("gbcache_prune");
	const int64_t now = 1000000;
	CHECK(GbCacheWrite(dir, "old", std::string(1000, 'a'), now));
	CHECK(GbCacheWrite(dir, "new", std::string(1000, 'b'), now));
	std::string body;
	bool stale = false;
	CHECK(GbCacheRead(dir, "new", 0, body, stale, now + 5000));
	GbCacheRead(dir, "old", 0, body, stale, now - 90000);
	const uint64_t freed = GbCachePrune(dir, 100000, 60000, now + 5000);
	CHECK(freed > 0);
	CHECK(!GbCacheRead(dir, "old", 0, body, stale, now));
	CHECK(GbCacheRead(dir, "new", 0, body, stale, now));
	CHECK_EQ(body.size(), 1000u);
	CHECK_EQ(GbCachePrune(dir, 10, 0, now), static_cast<uint64_t>(1000 + 32 + 3));
	CHECK(!GbCacheRead(dir, "new", 0, body, stale, now));
}

int main()
{
	Keys();
	Freshness();
	RoundTrip();
	Corrupt();
	Damaged();
	Prune();
	PruneOnDisk();
	return test::Summary("gbcache");
}
