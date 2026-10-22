// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>

#include "core/rarfile.h"
#include "rar_fixtures.h"

namespace {

const char kUnreadable[] = "the archive could not be read (unsupported format or damaged file)";
const char kTooLarge[] = "the archive is too large to install";
const char kCanceled[] = "canceled";

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

std::string Bytes(const unsigned char *data, size_t size)
{
	return std::string(reinterpret_cast<const char *>(data), size);
}

std::string ReadWide(const std::wstring &path)
{
	HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (h == INVALID_HANDLE_VALUE) return "<missing>";
	std::string data(1 << 16, '\0');
	DWORD got = 0;
	ReadFile(h, &data[0], static_cast<DWORD>(data.size()), &got, nullptr);
	CloseHandle(h);
	data.resize(got);
	return data;
}

std::wstring Wide(const std::string &s)
{
	const int n = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
	std::wstring w(static_cast<size_t>(n), L'\0');
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &w[0], n);
	w.resize(w.size() - 1);
	return w;
}

void CheckExtracted(const std::string &out)
{
	CHECK_EQ(test::ReadFile(out + "\\pack\\a.txt"), std::string("hola mundo\n"));
	CHECK(test::ReadFile(out + "\\pack\\p.bin") == Pattern(2000));
	CHECK(test::ReadFile(out + "\\pack\\sub\\b.bin") == Noise(600));
	CHECK_EQ(ReadWide(Wide(out) + L"\\pack\\\x00f1" L"and\x00fa" L".txt"), std::string("acentos\n"));
	const DWORD empty = GetFileAttributesA((out + "\\pack\\vacio").c_str());
	CHECK(empty != INVALID_FILE_ATTRIBUTES && (empty & FILE_ATTRIBUTE_DIRECTORY));
}

void RecognizesTheFormat(const std::string &dir)
{
	test::WriteFile(dir + "\\plain.rar", Bytes(kRarPlain, sizeof(kRarPlain)));
	test::WriteFile(dir + "\\text.txt", "Rar! not really");
	test::WriteFile(dir + "\\zip.zip", std::string("PK\x03\x04", 4) + "rest");
	CHECK(IsRarFile(dir + "\\plain.rar"));
	CHECK(!IsRarFile(dir + "\\text.txt"));
	CHECK(!IsRarFile(dir + "\\zip.zip"));
	CHECK(!IsRarFile(dir + "\\missing.rar"));
}

void ExtractsBothKinds(const std::string &dir)
{
	for (const char *kind : {"plain", "solid"}) {
		const bool solid = std::string(kind) == "solid";
		const std::string rar = dir + "\\" + kind + ".rar", out = dir + "\\out-" + kind;
		test::WriteFile(rar, solid ? Bytes(kRarSolid, sizeof(kRarSolid)) : Bytes(kRarPlain, sizeof(kRarPlain)));
		uint64_t lastDone = 0, lastTotal = 0;
		int calls = 0;
		std::string detail;
		const std::string err = RarExtract(rar, out, ZipLimits(), [&](uint64_t done, uint64_t total) {
			lastDone = done;
			lastTotal = total;
			calls++;
			return true;
		}, &detail);
		CHECK_EQ(err, std::string());
		CHECK_EQ(detail, std::string());
		CHECK(calls >= 1);
		CHECK_EQ(lastDone, lastTotal);
		CheckExtracted(out);
	}
}

void AppliesTheLimits(const std::string &dir)
{
	ZipLimits few;
	few.maxEntries = 2;
	CHECK_EQ(RarExtract(dir + "\\plain.rar", dir + "\\few", few), std::string(kTooLarge));
	ZipLimits tiny;
	tiny.maxBytes = 100;
	CHECK_EQ(RarExtract(dir + "\\plain.rar", dir + "\\tiny", tiny), std::string(kTooLarge));
	ZipLimits enough;
	enough.maxEntries = 100;
	enough.maxBytes = 1 << 20;
	CHECK_EQ(RarExtract(dir + "\\plain.rar", dir + "\\enough", enough), std::string());
}

void StopsWhenCanceled(const std::string &dir)
{
	CHECK_EQ(RarExtract(dir + "\\plain.rar", dir + "\\canceled", ZipLimits(), [](uint64_t, uint64_t) { return false; }), std::string(kCanceled));
}

void NoticesDamage(const std::string &dir)
{
	std::string rar = Bytes(kRarPlain, sizeof(kRarPlain));
	test::WriteFile(dir + "\\cut.rar", rar.substr(0, rar.size() / 2));
	CHECK(!RarExtract(dir + "\\cut.rar", dir + "\\cut", ZipLimits()).empty());
	std::string flipped = rar;
	flipped[flipped.size() / 2] ^= 0x55;
	flipped[flipped.size() / 2 + 1] ^= 0x55;
	test::WriteFile(dir + "\\flipped.rar", flipped);
	CHECK(!RarExtract(dir + "\\flipped.rar", dir + "\\flipped", ZipLimits()).empty());
	test::WriteFile(dir + "\\noise.rar", Noise(5000));
	CHECK_EQ(RarExtract(dir + "\\noise.rar", dir + "\\noise", ZipLimits()), std::string(kUnreadable));
	CHECK_EQ(RarExtract(dir + "\\missing.rar", dir + "\\missing", ZipLimits()), std::string(kUnreadable));
}

}

int main()
{
	const std::string dir = test::TempDir("rar");
	RecognizesTheFormat(dir);
	ExtractsBothKinds(dir);
	AppliesTheLimits(dir);
	StopsWhenCanceled(dir);
	NoticesDamage(dir);
	test::RemoveTree(dir);
	return test::Summary("rar");
}
