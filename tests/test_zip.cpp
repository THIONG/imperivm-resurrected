// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <cstring>
#include <string>
#include <vector>

#include "core/zipfile.h"
#include "miniz.h"

namespace {

const char kOutside[] = "the archive contains a path outside its folder: it will not be installed";
const char kSpecial[] = "the archive contains links or special files: it will not be installed";
const char kTooLarge[] = "the archive is too large to install";
const char kDamaged[] = "could not extract (unsupported format or damaged file)";

std::wstring Wide(const std::string &s)
{
	const int n = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
	std::wstring w(static_cast<size_t>(n), L'\0');
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &w[0], n);
	w.resize(w.size() - 1);
	return w;
}

std::string Pattern(size_t size)
{
	std::string data(size, '\0');
	for (size_t i = 0; i < size; i++) data[i] = static_cast<char>((i * 7 + i / 3) & 0xff);
	return data;
}

std::string Noise(size_t size)
{
	std::string data(size, '\0');
	uint32_t state = 2463534242u;
	for (char &c : data) {
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		c = static_cast<char>(state);
	}
	return data;
}

std::string MakeZip(const std::vector<std::pair<std::string, std::string>> &files)
{
	mz_zip_archive zip;
	mz_zip_zero_struct(&zip);
	mz_zip_writer_init_heap(&zip, 0, 0);
	for (const auto &f : files) mz_zip_writer_add_mem(&zip, f.first.c_str(), f.second.data(), f.second.size(), MZ_DEFAULT_LEVEL);
	void *buf = nullptr;
	size_t size = 0;
	mz_zip_writer_finalize_heap_archive(&zip, &buf, &size);
	const std::string out(static_cast<const char *>(buf), size);
	mz_free(buf);
	mz_zip_writer_end(&zip);
	return out;
}

void Rename(std::string &zip, const std::string &from, const std::string &to)
{
	for (size_t at = zip.find(from); at != std::string::npos; at = zip.find(from, at + to.size())) zip.replace(at, from.size(), to);
}

void EntryPaths()
{
	std::string rel;
	CHECK(ZipEntryPath("mod.json", rel) && rel == "mod.json");
	CHECK(ZipEntryPath("a\\b.txt", rel) && rel == "a/b.txt");
	CHECK(ZipEntryPath("./a/./b//c.txt", rel) && rel == "a/b/c.txt");
	CHECK(ZipEntryPath("dir/", rel) && rel == "dir");
	CHECK(ZipEntryPath("./", rel) && rel.empty());
	CHECK(!ZipEntryPath("../evil.txt", rel));
	CHECK(!ZipEntryPath("a/../../evil.txt", rel));
	CHECK(!ZipEntryPath("/abs.txt", rel));
	CHECK(!ZipEntryPath("C:/evil.txt", rel));
	CHECK(!ZipEntryPath("a/con.txt", rel));
	CHECK(!ZipEntryPath("a/file.txt:stream", rel));
	CHECK(!ZipEntryPath("a/name ", rel));
}

void RoundTrip(const std::string &dir)
{
	const std::string src = dir + "\\src", out = dir + "\\out", zip = dir + "\\round.zip";
	CreateDirectoryA(src.c_str(), nullptr);
	CreateDirectoryA((src + "\\sub").c_str(), nullptr);
	CreateDirectoryA((src + "\\empty").c_str(), nullptr);
	test::WriteFile(src + "\\mod.json", "{\"id\":\"test\"}");
	const std::string big = Pattern(3u << 20);
	const std::wstring accented = Wide(src) + L"\\sub\\\x00f1and\x00fa.bin";
	HANDLE h = CreateFileW(accented.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
	DWORD put = 0;
	WriteFile(h, big.data(), static_cast<DWORD>(big.size()), &put, nullptr);
	CloseHandle(h);

	CHECK(ZipCreate(zip, src, {"mod.json", "sub", "empty"}));
	CHECK(IsZipFile(zip));
	CHECK(!IsZipFile(src + "\\mod.json"));

	uint64_t lastDone = 0, lastTotal = 0;
	int calls = 0;
	const std::string err = ZipExtract(zip, out, ZipLimits(), [&](uint64_t done, uint64_t total) {
		lastDone = done;
		lastTotal = total;
		calls++;
		return true;
	});
	CHECK_EQ(err, std::string());
	CHECK_EQ(test::ReadFile(out + "\\mod.json"), std::string("{\"id\":\"test\"}"));
	CHECK(calls > 1);
	CHECK_EQ(lastDone, lastTotal);
	CHECK_EQ(lastTotal, static_cast<unsigned long long>(big.size() + 13));
	const std::wstring back = Wide(out) + L"\\sub\\\x00f1and\x00fa.bin";
	h = CreateFileW(back.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	CHECK(h != INVALID_HANDLE_VALUE);
	std::string read(big.size() + 16, '\0');
	DWORD got = 0;
	ReadFile(h, &read[0], static_cast<DWORD>(read.size()), &got, nullptr);
	CloseHandle(h);
	read.resize(got);
	CHECK(read == big);
	const DWORD empty = GetFileAttributesA((out + "\\empty").c_str());
	CHECK(empty != INVALID_FILE_ATTRIBUTES && (empty & FILE_ATTRIBUTE_DIRECTORY));
}

void RefusesPathsOutsideTheFolder(const std::string &dir)
{
	const std::string out = dir + "\\outside";
	for (const char *name : {"../evil.txt", "C:/evil.txt", "/a/evil.txt"}) {
		std::string zip = MakeZip({{"aa/evil.txt", "evil"}});
		Rename(zip, "aa/evil.txt", name);
		test::WriteFile(dir + "\\outside.zip", zip);
		CHECK_EQ(ZipExtract(dir + "\\outside.zip", out, ZipLimits()), std::string(kOutside));
	}
	CHECK(!test::Exists(dir + "\\evil.txt"));
	CHECK(!test::Exists(out + "\\evil.txt"));
}

void RefusesLinks(const std::string &dir)
{
	std::string zip = MakeZip({{"link", "target.txt"}});
	const size_t central = zip.find(std::string("PK\x01\x02", 4));
	CHECK(central != std::string::npos);
	zip[central + 5] = 3;
	const uint32_t symlink = 0120777u << 16;
	memcpy(&zip[central + 38], &symlink, sizeof(symlink));
	test::WriteFile(dir + "\\link.zip", zip);
	CHECK_EQ(ZipExtract(dir + "\\link.zip", dir + "\\link", ZipLimits()), std::string(kSpecial));
}

void AppliesTheLimits(const std::string &dir)
{
	test::WriteFile(dir + "\\limits.zip", MakeZip({{"a.txt", Pattern(100)}, {"b.txt", Pattern(100)}}));
	ZipLimits entries;
	entries.maxEntries = 1;
	CHECK_EQ(ZipExtract(dir + "\\limits.zip", dir + "\\limits", entries), std::string(kTooLarge));
	ZipLimits bytes;
	bytes.maxBytes = 150;
	CHECK_EQ(ZipExtract(dir + "\\limits.zip", dir + "\\limits", bytes), std::string(kTooLarge));
	ZipLimits enough;
	enough.maxEntries = 2;
	enough.maxBytes = 200;
	CHECK_EQ(ZipExtract(dir + "\\limits.zip", dir + "\\limits", enough), std::string());
}

void StopsWhenCanceled(const std::string &dir)
{
	test::WriteFile(dir + "\\cancel.zip", MakeZip({{"big.bin", Pattern(4u << 20)}}));
	CHECK_EQ(ZipExtract(dir + "\\cancel.zip", dir + "\\cancel", ZipLimits(), [](uint64_t, uint64_t) { return false; }), std::string("canceled"));
}

void NoticesDamage(const std::string &dir)
{
	const std::string data = Noise(64 * 1024);
	std::string zip = MakeZip({{"data.bin", data}});
	const size_t local = zip.find(std::string("PK\x03\x04", 4));
	const size_t payload = local + 30 + 8;
	zip[payload + 40] = static_cast<char>(zip[payload + 40] ^ 0x5a);
	test::WriteFile(dir + "\\damaged.zip", zip);
	CHECK_EQ(ZipExtract(dir + "\\damaged.zip", dir + "\\damaged", ZipLimits()), std::string(kDamaged));
	test::WriteFile(dir + "\\notzip.zip", "PK\x03\x04 this is not really a zip");
	CHECK(!ZipExtract(dir + "\\notzip.zip", dir + "\\notzip", ZipLimits()).empty());
}

void KnowsWhenToTryTarExe(const std::string &dir)
{
	std::string zip = MakeZip({{"data.bin", Pattern(1000)}});
	const size_t local = zip.find(std::string("PK\x03\x04", 4));
	const size_t central = zip.find(std::string("PK\x01\x02", 4));
	zip[local + 8] = 14;
	zip[central + 10] = 14;
	test::WriteFile(dir + "\\lzma.zip", zip);
	const std::string error = ZipExtract(dir + "\\lzma.zip", dir + "\\lzma", ZipLimits());
	CHECK_EQ(error, std::string(kDamaged));
	CHECK(ZipNotUnderstood(error));
	CHECK(!ZipNotUnderstood(""));
	CHECK(!ZipNotUnderstood(kOutside));
	CHECK(!ZipNotUnderstood(kSpecial));
	CHECK(!ZipNotUnderstood(kTooLarge));
	CHECK(!ZipNotUnderstood("canceled"));
}

void HandlesLongPaths(const std::string &dir)
{
	const std::string first(120, 'd'), second(120, 'e'), file = std::string(60, 'f') + ".txt";
	const std::string dest = dir + "\\" + first + "\\" + second;
	test::WriteFile(dir + "\\long.zip", MakeZip({{"sub/" + file, "long"}}));
	CHECK(dest.size() + 5 + file.size() > MAX_PATH);
	CHECK_EQ(ZipExtract(dir + "\\long.zip", dest, ZipLimits()), std::string());
	const std::wstring prefix = L"\\\\?\\" + Wide(dir);
	const std::wstring full = prefix + L"\\" + Wide(first) + L"\\" + Wide(second) + L"\\sub\\" + Wide(file);
	CHECK(GetFileAttributesW(full.c_str()) != INVALID_FILE_ATTRIBUTES);
	DeleteFileW(full.c_str());
	RemoveDirectoryW((prefix + L"\\" + Wide(first) + L"\\" + Wide(second) + L"\\sub").c_str());
	RemoveDirectoryW((prefix + L"\\" + Wide(first) + L"\\" + Wide(second)).c_str());
	RemoveDirectoryW((prefix + L"\\" + Wide(first)).c_str());
}

void ReadsWindowsZips(const std::string &dir)
{
	char system32[MAX_PATH];
	GetSystemDirectoryA(system32, MAX_PATH);
	const std::string tar = std::string(system32) + "\\tar.exe";
	if (!test::Exists(tar)) return;
	const std::string src = dir + "\\win";
	CreateDirectoryA(src.c_str(), nullptr);
	CreateDirectoryA((src + "\\data").c_str(), nullptr);
	test::WriteFile(src + "\\data\\units.txt", Pattern(200000));
	const std::string zip = dir + "\\win.zip";
	const std::string command = "\"\"" + tar + "\" -a -c -f \"" + zip + "\" -C \"" + src + "\" data\"";
	CHECK(std::system(command.c_str()) == 0);
	CHECK_EQ(ZipExtract(zip, dir + "\\winout", ZipLimits()), std::string());
	CHECK(test::ReadFile(dir + "\\winout\\data\\units.txt") == Pattern(200000));
}

}

int main()
{
	const std::string dir = test::TempDir("zip");
	EntryPaths();
	RoundTrip(dir);
	RefusesPathsOutsideTheFolder(dir);
	RefusesLinks(dir);
	AppliesTheLimits(dir);
	StopsWhenCanceled(dir);
	NoticesDamage(dir);
	KnowsWhenToTryTarExe(dir);
	HandlesLongPaths(dir);
	ReadsWindowsZips(dir);
	test::RemoveTree(dir);
	return test::Summary("zip");
}
