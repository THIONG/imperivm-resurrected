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
const char kDamaged[] = "could not extract (unsupported format or damaged file)";
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

bool IsFolder(const std::string &path)
{
	const DWORD attr = GetFileAttributesA(path.c_str());
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

void Le32(std::string &out, uint32_t v)
{
	for (int i = 0; i < 4; i++) out += static_cast<char>(v >> (8 * i));
}

void Le64(std::string &out, uint64_t v)
{
	for (int i = 0; i < 8; i++) out += static_cast<char>(v >> (8 * i));
}

uint32_t Crc(const std::string &data)
{
	return static_cast<uint32_t>(mz_crc32(MZ_CRC32_INIT, reinterpret_cast<const unsigned char *>(data.data()), data.size()));
}

// 7z numbers: one byte below 0x80, two bytes below 0x4000 (all that these files need).
void Num(std::string &out, uint32_t v)
{
	if (v < 0x80) {
		out += static_cast<char>(v);
	}
	else {
		out += static_cast<char>(0x80 | (v >> 8));
		out += static_cast<char>(v & 0xff);
	}
}

// A 7z with one file stored with the Copy method and an unencoded header.
std::string Make7z(const std::string &name, const std::string &data)
{
	std::string names;
	names += '\0';
	for (char c : name) {
		names += c;
		names += '\0';
	}
	names += std::string(2, '\0');
	std::string h;
	h += '\x01';
	h += '\x04';
	h += '\x06';
	Num(h, 0);
	Num(h, 1);
	h += '\x09';
	Num(h, static_cast<uint32_t>(data.size()));
	h += '\x00';
	h += '\x07';
	h += '\x0b';
	Num(h, 1);
	h += '\x00';
	Num(h, 1);
	h += '\x01';
	h += '\x00';
	h += '\x0c';
	Num(h, static_cast<uint32_t>(data.size()));
	h += '\x00';
	h += '\x08';
	h += '\x0a';
	h += '\x01';
	Le32(h, Crc(data));
	h += '\x00';
	h += '\x00';
	h += '\x05';
	Num(h, 1);
	h += '\x11';
	Num(h, static_cast<uint32_t>(names.size()));
	h += names;
	h += '\x00';
	h += '\x00';
	std::string start;
	Le64(start, data.size());
	Le64(start, h.size());
	Le32(start, Crc(h));
	std::string out("7z\xbc\xaf\x27\x1c\x00\x04", 8);
	Le32(out, Crc(start));
	out += start;
	out += data;
	out += h;
	return out;
}

void CheckMod(const std::string &dir)
{
	CHECK_EQ(HashOf(dir + "\\mod\\hello.txt"), std::string("e6578589267daa4e"));
	CHECK_EQ(HashOf(dir + "\\mod\\data\\blob.bin"), std::string("8a558a590011cda3"));
	CHECK_EQ(HashOf(dir + "\\mod\\data\\code.bin"), std::string("8a6dad23f1d87fd0"));
	CHECK_EQ(HashOf(dir + "\\mod\\data\\units.txt"), std::string("154083b8d8b926fd"));
	CHECK_EQ(HashOf(dir + "\\mod\\data\\units2.txt"), std::string("5fabb9268d4b52ff"));
	CHECK(test::Exists(dir + "\\mod\\empty.txt"));
	CHECK_EQ(test::ReadFile(dir + "\\mod\\empty.txt").size(), static_cast<size_t>(0));
	CHECK(IsFolder(dir + "\\empty_dir"));
}

void Detects()
{
	CHECK(IsArchiveFile(Fixture("lzma_solid.7z")));
	CHECK(IsArchiveFile(Fixture("libarchive_empty_archive.7z")));
	CHECK(!IsArchiveFile(Fixture("missing.7z")));
	CHECK(!IsArchiveFile(g_data + "\\.."));
}

void ReadsEveryMethod(const std::string &dir)
{
	for (const char *name : {"lzma_solid", "lzma2_files", "bcj_lzma2", "delta_lzma", "ppmd"}) {
		const std::string out = dir + "\\" + name;
		CHECK_EQ(ArchiveExtract(Fixture((std::string(name) + ".7z").c_str()), out, ZipLimits()), std::string());
		CheckMod(out);
	}
}

void ReadsFilesFromOtherWriters(const std::string &dir)
{
	CHECK_EQ(ArchiveExtract(Fixture("libarchive_bcj_lzma1.7z"), dir + "\\bcj1", ZipLimits()), std::string());
	CHECK_EQ(HashOf(dir + "\\bcj1\\x86exe"), std::string("9f24006793ce80bf"));
	CHECK_EQ(ArchiveExtract(Fixture("libarchive_lzma2_arm64.7z"), dir + "\\arm64", ZipLimits()), std::string());
	CHECK_EQ(HashOf(dir + "\\arm64\\hw-arm64"), std::string("0d8fd07e6202c2da"));
	CHECK_EQ(ArchiveExtract(Fixture("libarchive_empty_archive.7z"), dir + "\\emptyarchive", ZipLimits()), std::string());
	CHECK(IsFolder(dir + "\\emptyarchive"));
}

void ReportsProgress(const std::string &dir)
{
	uint64_t lastDone = 0, lastTotal = 0;
	int calls = 0;
	const std::string err = ArchiveExtract(Fixture("big.7z"), dir + "\\big", ZipLimits(), [&](uint64_t done, uint64_t total) {
		CHECK(done >= lastDone);
		lastDone = done;
		lastTotal = total;
		calls++;
		return true;
	});
	CHECK_EQ(err, std::string());
	CHECK(calls > 1);
	CHECK_EQ(lastDone, lastTotal);
	CHECK_EQ(lastTotal, static_cast<unsigned long long>(test::ReadFile(Fixture("big.7z")).size()));
	const std::string big = test::ReadFile(dir + "\\big\\big.txt");
	CHECK_EQ(big.size(), static_cast<size_t>(20500000));
	CHECK_EQ(Hash(big), std::string("fc1f22e9b5451caa"));
}

void AcceptsGeneratedArchives(const std::string &dir)
{
	test::WriteFile(dir + "\\good.7z", Make7z("mod/data/units.txt", "units"));
	CHECK_EQ(ArchiveExtract(dir + "\\good.7z", dir + "\\good", ZipLimits()), std::string());
	CHECK_EQ(test::ReadFile(dir + "\\good\\mod\\data\\units.txt"), std::string("units"));
}

void RefusesPathsOutsideTheFolder(const std::string &dir)
{
	const std::string out = dir + "\\outside";
	for (const char *name : {"../evil.txt", "a/../../evil.txt", "C:/evil.txt", "/a/evil.txt", "a/con.txt", "a/file.txt:stream"}) {
		test::WriteFile(dir + "\\outside.7z", Make7z(name, "evil"));
		CHECK_EQ(ArchiveExtract(dir + "\\outside.7z", out, ZipLimits()), std::string(kOutside));
	}
	CHECK(!test::Exists(dir + "\\evil.txt"));
	CHECK(!test::Exists(out + "\\evil.txt"));
	CHECK(!test::Exists(out + "\\a\\con.txt"));
}

void RefusesLinks(const std::string &dir)
{
	CHECK_EQ(ArchiveExtract(Fixture("symlink.7z"), dir + "\\link", ZipLimits()), std::string(kSpecial));
}

void RefusesPasswordProtected(const std::string &dir)
{
	CHECK_EQ(ArchiveExtract(Fixture("encrypted.7z"), dir + "\\enc", ZipLimits()), std::string(kEncrypted));
	CHECK_EQ(ArchiveExtract(Fixture("encrypted_header.7z"), dir + "\\enchead", ZipLimits()), std::string(kEncrypted));
}

void RefusesMethodsItDoesNotHave(const std::string &dir)
{
	CHECK_EQ(ArchiveExtract(Fixture("bzip2.7z"), dir + "\\bzip2", ZipLimits()), std::string(kDamaged));
	CHECK_EQ(ArchiveExtract(Fixture("deflate.7z"), dir + "\\deflate", ZipLimits()), std::string(kDamaged));
	CHECK(ArchiveNotUnderstood(kDamaged));
	CHECK(!ArchiveNotUnderstood(kOutside));
	CHECK(!ArchiveNotUnderstood(kEncrypted));
	CHECK(!ArchiveNotUnderstood(kSpecial));
	CHECK(!ArchiveNotUnderstood(kTooLarge));
	CHECK(!ArchiveNotUnderstood(kCanceled));
}

void AppliesTheLimits(const std::string &dir)
{
	ZipLimits entries;
	entries.maxEntries = 3;
	CHECK_EQ(ArchiveExtract(Fixture("lzma_solid.7z"), dir + "\\limits", entries), std::string(kTooLarge));
	ZipLimits bytes;
	bytes.maxBytes = 10000;
	CHECK_EQ(ArchiveExtract(Fixture("lzma_solid.7z"), dir + "\\limits", bytes), std::string(kTooLarge));
	ZipLimits enough;
	enough.maxEntries = 9;
	enough.maxBytes = 47455;
	CHECK_EQ(ArchiveExtract(Fixture("lzma_solid.7z"), dir + "\\limits", enough), std::string());
}

void StopsWhenCanceled(const std::string &dir)
{
	CHECK_EQ(ArchiveExtract(Fixture("big.7z"), dir + "\\cancel", ZipLimits(), [](uint64_t, uint64_t) { return false; }), std::string(kCanceled));
}

void NoticesDamage(const std::string &dir)
{
	std::string sevenz = test::ReadFile(Fixture("lzma2_files.7z"));
	sevenz[sevenz.size() / 2] = static_cast<char>(sevenz[sevenz.size() / 2] ^ 0x5a);
	test::WriteFile(dir + "\\damaged.7z", sevenz);
	CHECK(!ArchiveExtract(dir + "\\damaged.7z", dir + "\\damaged", ZipLimits()).empty());

	std::string cut = test::ReadFile(Fixture("big.7z"));
	cut.resize(cut.size() / 2);
	test::WriteFile(dir + "\\cut.7z", cut);
	CHECK(!ArchiveExtract(dir + "\\cut.7z", dir + "\\cut", ZipLimits()).empty());

	test::WriteFile(dir + "\\not7z.7z", std::string("7z\xbc\xaf\x27\x1c", 6) + " this is not really a 7z");
	CHECK(!ArchiveExtract(dir + "\\not7z.7z", dir + "\\not7z", ZipLimits()).empty());
	CHECK(!ArchiveExtract(dir + "\\missing.7z", dir + "\\missing", ZipLimits()).empty());
}

void HandlesLongPaths(const std::string &dir)
{
	const std::string first(120, 'd'), second(120, 'e'), file = std::string(60, 'f') + ".txt";
	const std::string dest = dir + "\\" + first + "\\" + second;
	test::WriteFile(dir + "\\long.7z", Make7z("sub/" + file, "long"));
	CHECK(dest.size() + 5 + file.size() > MAX_PATH);
	CHECK_EQ(ArchiveExtract(dir + "\\long.7z", dest, ZipLimits()), std::string());
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
		std::printf("usage: test_7z <folder with tests/data/7z>\n");
		return 2;
	}
	g_data = argv[1];
	const std::string dir = test::TempDir("7z");
	Detects();
	ReadsEveryMethod(dir);
	ReadsFilesFromOtherWriters(dir);
	ReportsProgress(dir);
	AcceptsGeneratedArchives(dir);
	RefusesPathsOutsideTheFolder(dir);
	RefusesLinks(dir);
	RefusesPasswordProtected(dir);
	RefusesMethodsItDoesNotHave(dir);
	AppliesTheLimits(dir);
	StopsWhenCanceled(dir);
	NoticesDamage(dir);
	HandlesLongPaths(dir);
	test::RemoveTree(dir);
	return test::Summary("7z");
}
