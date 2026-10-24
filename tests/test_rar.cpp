// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <cstring>
#include <string>
#include <vector>

#include "core/archivefile.h"
#include "core/sha256.h"
#include "miniz.h"

namespace {

const char kOutside[] = "the archive contains a path outside its folder: it will not be installed";
const char kSpecial[] = "the archive contains links or special files: it will not be installed";
const char kTooLarge[] = "the archive is too large to install";
const char kEncrypted[] = "the archive is protected with a password: it cannot be installed";
const char kCanceled[] = "canceled";

std::string g_data;

std::string Fixture(const char *name)
{
	return g_data + "\\" + name;
}

std::wstring Wide(const std::string &s)
{
	const int n = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
	std::wstring w(static_cast<size_t>(n), L'\0');
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &w[0], n);
	w.resize(w.size() - 1);
	return w;
}

std::string Hash(const std::string &data)
{
	Sha256 h;
	h.Add(data);
	return h.Hex().substr(0, 16);
}

std::string HashOf(const std::string &path)
{
	return Hash(test::ReadFile(path));
}

void Vint(std::string &out, uint64_t v)
{
	while (v >= 0x80) {
		out += static_cast<char>(v | 0x80);
		v >>= 7;
	}
	out += static_cast<char>(v);
}

void Le32(std::string &out, uint32_t v)
{
	for (int i = 0; i < 4; i++) out += static_cast<char>(v >> (8 * i));
}

void Block(std::string &out, const std::string &body)
{
	std::string sized;
	Vint(sized, body.size());
	sized += body;
	Le32(out, static_cast<uint32_t>(mz_crc32(MZ_CRC32_INIT, reinterpret_cast<const unsigned char *>(sized.data()), sized.size())));
	out += sized;
}

std::string MakeRar5(const std::string &name, const std::string &data)
{
	std::string out("Rar!\x1a\x07\x01\x00", 8);
	std::string main;
	Vint(main, 1);
	Vint(main, 0);
	Vint(main, 0);
	Block(out, main);
	std::string file;
	Vint(file, 2);
	Vint(file, 2);
	Vint(file, data.size());
	Vint(file, 4);
	Vint(file, data.size());
	Vint(file, 0);
	Le32(file, static_cast<uint32_t>(mz_crc32(MZ_CRC32_INIT, reinterpret_cast<const unsigned char *>(data.data()), data.size())));
	Vint(file, 0);
	Vint(file, 0);
	Vint(file, name.size());
	file += name;
	Block(out, file);
	out += data;
	std::string end;
	Vint(end, 5);
	Vint(end, 0);
	Vint(end, 0);
	Block(out, end);
	return out;
}

void Detects()
{
	CHECK(IsArchiveFile(Fixture("rar5_stored.rar")));
	CHECK(IsArchiveFile(Fixture("rar3_windows.rar")));
	CHECK(!IsArchiveFile(Fixture("missing.rar")));
	CHECK(!IsArchiveFile(g_data + "\\.."));
}

void ReadsRar5(const std::string &dir)
{
	CHECK_EQ(ArchiveExtract(Fixture("rar5_stored.rar"), dir + "\\stored", ZipLimits()), std::string());
	CHECK_EQ(test::ReadFile(dir + "\\stored\\helloworld.txt"), std::string("hello libarchive test suite!\n"));

	CHECK_EQ(ArchiveExtract(Fixture("rar5_compressed.rar"), dir + "\\compressed", ZipLimits()), std::string());
	CHECK_EQ(HashOf(dir + "\\compressed\\test.bin"), std::string("588870a2dade35c2"));

	CHECK_EQ(ArchiveExtract(Fixture("rar5_solid.rar"), dir + "\\solid", ZipLimits()), std::string());
	CHECK_EQ(HashOf(dir + "\\solid\\test.bin"), std::string("588870a2dade35c2"));
	CHECK_EQ(HashOf(dir + "\\solid\\test1.bin"), std::string("7d89f86f9f69d744"));
	CHECK_EQ(HashOf(dir + "\\solid\\test6.bin"), std::string("0b79ce23670b7c2e"));

	CHECK_EQ(ArchiveExtract(Fixture("rar5_win32.rar"), dir + "\\win32", ZipLimits()), std::string());
	CHECK_EQ(HashOf(dir + "\\win32\\test5.bin"), std::string("b0622b648b174abd"));
	const DWORD folder = GetFileAttributesA((dir + "\\win32\\testdir").c_str());
	CHECK(folder != INVALID_FILE_ATTRIBUTES && (folder & FILE_ATTRIBUTE_DIRECTORY));
}

void ReadsRar3(const std::string &dir)
{
	CHECK_EQ(ArchiveExtract(Fixture("rar3_windows.rar"), dir + "\\rar3", ZipLimits()), std::string());
	CHECK_EQ(HashOf(dir + "\\rar3\\testdir\\test.txt"), std::string("2d45c5f87d1b6cef"));
	CHECK_EQ(HashOf(dir + "\\rar3\\test.txt"), std::string("2d45c5f87d1b6cef"));
	CHECK_EQ(HashOf(dir + "\\rar3\\testshortcut.lnk"), std::string("08b633f146f22534"));
	const DWORD empty = GetFileAttributesA((dir + "\\rar3\\testemptydir").c_str());
	CHECK(empty != INVALID_FILE_ATTRIBUTES && (empty & FILE_ATTRIBUTE_DIRECTORY));

	CHECK_EQ(ArchiveExtract(Fixture("rar3_noeof.rar"), dir + "\\noeof", ZipLimits()), std::string());
	CHECK_EQ(HashOf(dir + "\\noeof\\test.txt"), std::string("5a5f16e01faf8adf"));
}

void ReportsProgress(const std::string &dir)
{
	uint64_t lastDone = 0, lastTotal = 0;
	int calls = 0;
	const std::string err = ArchiveExtract(Fixture("rar3_big.rar"), dir + "\\big", ZipLimits(), [&](uint64_t done, uint64_t total) {
		CHECK(done >= lastDone);
		lastDone = done;
		lastTotal = total;
		calls++;
		return true;
	});
	CHECK_EQ(err, std::string());
	CHECK(calls > 1);
	CHECK_EQ(lastDone, lastTotal);
	CHECK_EQ(lastTotal, static_cast<unsigned long long>(test::ReadFile(Fixture("rar3_big.rar")).size()));
	const std::string big = test::ReadFile(dir + "\\big\\multi_lzss_blocks_test.txt");
	CHECK_EQ(big.size(), static_cast<size_t>(20131111));
	CHECK_EQ(Hash(big), std::string("49a84a381599f749"));
}

void AcceptsGeneratedArchives(const std::string &dir)
{
	test::WriteFile(dir + "\\good.rar", MakeRar5("mod/data/units.txt", "units"));
	CHECK_EQ(ArchiveExtract(dir + "\\good.rar", dir + "\\good", ZipLimits()), std::string());
	CHECK_EQ(test::ReadFile(dir + "\\good\\mod\\data\\units.txt"), std::string("units"));
}

void RefusesPathsOutsideTheFolder(const std::string &dir)
{
	const std::string out = dir + "\\outside";
	for (const char *name : {"../evil.txt", "a/../../evil.txt", "C:/evil.txt", "/a/evil.txt", "a/con.txt", "a/file.txt:stream"}) {
		test::WriteFile(dir + "\\outside.rar", MakeRar5(name, "evil"));
		CHECK_EQ(ArchiveExtract(dir + "\\outside.rar", out, ZipLimits()), std::string(kOutside));
	}
	CHECK(!test::Exists(dir + "\\evil.txt"));
	CHECK(!test::Exists(out + "\\evil.txt"));
	CHECK(!test::Exists(out + "\\a\\con.txt"));
}

void RefusesLinks(const std::string &dir)
{
	CHECK_EQ(ArchiveExtract(Fixture("rar5_symlink.rar"), dir + "\\link", ZipLimits()), std::string(kSpecial));
}

void RefusesPasswordProtected(const std::string &dir)
{
	CHECK_EQ(ArchiveExtract(Fixture("rar5_encrypted.rar"), dir + "\\enc5", ZipLimits()), std::string(kEncrypted));
	CHECK_EQ(ArchiveExtract(Fixture("rar3_encrypted.rar"), dir + "\\enc3", ZipLimits()), std::string(kEncrypted));
}

void AppliesTheLimits(const std::string &dir)
{
	ZipLimits entries;
	entries.maxEntries = 3;
	CHECK_EQ(ArchiveExtract(Fixture("rar5_solid.rar"), dir + "\\limits", entries), std::string(kTooLarge));
	ZipLimits bytes;
	bytes.maxBytes = 10000;
	CHECK_EQ(ArchiveExtract(Fixture("rar5_solid.rar"), dir + "\\limits", bytes), std::string(kTooLarge));
	ZipLimits enough;
	enough.maxEntries = 7;
	enough.maxBytes = 4096 * 6 + 1200;
	CHECK_EQ(ArchiveExtract(Fixture("rar5_solid.rar"), dir + "\\limits", enough), std::string());
}

void StopsWhenCanceled(const std::string &dir)
{
	CHECK_EQ(ArchiveExtract(Fixture("rar3_big.rar"), dir + "\\cancel", ZipLimits(), [](uint64_t, uint64_t) { return false; }), std::string(kCanceled));
}

void NoticesDamage(const std::string &dir)
{
	std::string rar = test::ReadFile(Fixture("rar5_compressed.rar"));
	rar[rar.size() / 2] = static_cast<char>(rar[rar.size() / 2] ^ 0x5a);
	test::WriteFile(dir + "\\damaged.rar", rar);
	CHECK(!ArchiveExtract(dir + "\\damaged.rar", dir + "\\damaged", ZipLimits()).empty());

	std::string cut = test::ReadFile(Fixture("rar3_big.rar"));
	cut.resize(cut.size() / 2);
	test::WriteFile(dir + "\\cut.rar", cut);
	CHECK(!ArchiveExtract(dir + "\\cut.rar", dir + "\\cut", ZipLimits()).empty());

	test::WriteFile(dir + "\\notrar.rar", "Rar!\x1a\x07\x00 this is not really a rar");
	CHECK(!ArchiveExtract(dir + "\\notrar.rar", dir + "\\notrar", ZipLimits()).empty());
	CHECK(!ArchiveExtract(dir + "\\missing.rar", dir + "\\missing", ZipLimits()).empty());
}

void HandlesLongPaths(const std::string &dir)
{
	const std::string first(120, 'd'), second(120, 'e'), file = std::string(60, 'f') + ".txt";
	const std::string dest = dir + "\\" + first + "\\" + second;
	test::WriteFile(dir + "\\long.rar", MakeRar5("sub/" + file, "long"));
	CHECK(dest.size() + 5 + file.size() > MAX_PATH);
	CHECK_EQ(ArchiveExtract(dir + "\\long.rar", dest, ZipLimits()), std::string());
	const std::wstring prefix = L"\\\\?\\" + Wide(dir);
	const std::wstring full = prefix + L"\\" + Wide(first) + L"\\" + Wide(second) + L"\\sub\\" + Wide(file);
	CHECK(GetFileAttributesW(full.c_str()) != INVALID_FILE_ATTRIBUTES);
	DeleteFileW(full.c_str());
	RemoveDirectoryW((prefix + L"\\" + Wide(first) + L"\\" + Wide(second) + L"\\sub").c_str());
	RemoveDirectoryW((prefix + L"\\" + Wide(first) + L"\\" + Wide(second)).c_str());
	RemoveDirectoryW((prefix + L"\\" + Wide(first)).c_str());
}

}

int main(int argc, char **argv)
{
	if (argc < 2) {
		std::printf("usage: test_rar <folder with tests/data/rar>\n");
		return 2;
	}
	g_data = argv[1];
	const std::string dir = test::TempDir("rar");
	Detects();
	ReadsRar5(dir);
	ReadsRar3(dir);
	ReportsProgress(dir);
	AcceptsGeneratedArchives(dir);
	RefusesPathsOutsideTheFolder(dir);
	RefusesLinks(dir);
	RefusesPasswordProtected(dir);
	AppliesTheLimits(dir);
	StopsWhenCanceled(dir);
	NoticesDamage(dir);
	HandlesLongPaths(dir);
	test::RemoveTree(dir);
	return test::Summary("rar");
}
