// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "core/rarfile.h"

#include <windows.h>

#include <algorithm>
#include <cstdint>

#ifndef LIBARCHIVE_STATIC
#define LIBARCHIVE_STATIC
#endif
#include "archive.h"
#include "archive_entry.h"

#include "core/archivefs.h"

namespace {
using archivefs::FileTimeOf;
using archivefs::LongPath;
using archivefs::MakeDirs;
using archivefs::Utf8;
using archivefs::Widen;

const char kUnreadable[] = "the archive could not be read (unsupported format or damaged file)";
const char kDamaged[] = "could not extract (unsupported format or damaged file)";
const char kSpecial[] = "the archive contains links or special files: it will not be installed";
const char kTooLarge[] = "the archive is too large to install";
const char kOutside[] = "the archive contains a path outside its folder: it will not be installed";
const char kCannotWrite[] = "could not write the extracted files";
const char kCanceled[] = "canceled";

const size_t kReadBlock = 1u << 20;
const uint64_t kProgressStep = 1u << 20;

struct Reader {
	archive *a = nullptr;
	~Reader()
	{
		if (a) archive_read_free(a);
	}
};

std::string Detail(archive *a)
{
	const char *text = archive_error_string(a);
	return text ? text : "";
}

bool WriteAll(HANDLE out, const void *data, size_t n)
{
	const char *p = static_cast<const char *>(data);
	while (n) {
		const DWORD chunk = static_cast<DWORD>(std::min<size_t>(n, 1u << 30));
		DWORD put = 0;
		if (!WriteFile(out, p, chunk, &put, nullptr) || !put) return false;
		p += put;
		n -= put;
	}
	return true;
}

std::string EntryRelative(archive_entry *entry, std::string &rel)
{
	const wchar_t *wide = archive_entry_pathname_w(entry);
	if (!wide) return kUnreadable;
	const std::string name = Utf8(wide);
	if (name.empty() && *wide) return kUnreadable;
	if (!ZipEntryPath(name, rel)) return kOutside;
	std::replace(rel.begin(), rel.end(), '/', '\\');
	return "";
}

std::string WriteEntry(archive *a, HANDLE out, uint64_t &written, uint64_t &nextReport, uint64_t archiveSize, const ZipLimits &limits, const ZipProgress &progress,
		std::string *detail)
{
	LARGE_INTEGER at = {};
	for (;;) {
		const void *block = nullptr;
		size_t size = 0;
		la_int64_t offset = 0;
		const int got = archive_read_data_block(a, &block, &size, &offset);
		if (got == ARCHIVE_EOF) return "";
		if (got < ARCHIVE_WARN) {
			if (detail) *detail = Detail(a);
			return kDamaged;
		}
		if (offset != at.QuadPart) {
			at.QuadPart = offset;
			if (!SetFilePointerEx(out, at, nullptr, FILE_BEGIN)) return kCannotWrite;
		}
		written += size;
		if (limits.maxBytes && written > limits.maxBytes) return kTooLarge;
		if (!WriteAll(out, block, size)) return kCannotWrite;
		at.QuadPart += static_cast<LONGLONG>(size);
		const uint64_t consumed = static_cast<uint64_t>(std::max<la_int64_t>(0, archive_filter_bytes(a, 0)));
		if (progress && consumed >= nextReport) {
			nextReport = consumed + kProgressStep;
			if (!progress(consumed, archiveSize)) return kCanceled;
		}
	}
}

std::string Unpack(archive *a, const std::wstring &root, uint64_t archiveSize, const ZipLimits &limits, const ZipProgress &progress, std::string *detail)
{
	size_t entries = 0;
	uint64_t written = 0;
	uint64_t nextReport = 0;
	archive_entry *entry = nullptr;
	for (;;) {
		const int status = archive_read_next_header(a, &entry);
		if (status == ARCHIVE_EOF) break;
		if (status < ARCHIVE_WARN) {
			if (detail) *detail = Detail(a);
			return kDamaged;
		}
		if (limits.maxEntries && ++entries > limits.maxEntries) return kTooLarge;
		const unsigned type = archive_entry_filetype(entry);
		if (type != AE_IFREG && type != AE_IFDIR) return kSpecial;
		if (archive_entry_hardlink(entry) || archive_entry_symlink(entry)) return kSpecial;
		std::string rel;
		const std::string bad = EntryRelative(entry, rel);
		if (!bad.empty()) return bad;
		if (rel.empty()) continue;
		const std::wstring full = root + L"\\" + Widen(rel, CP_UTF8);
		if (type == AE_IFDIR) {
			MakeDirs(full);
			continue;
		}
		const size_t slash = full.find_last_of(L'\\');
		if (slash != std::wstring::npos) MakeDirs(full.substr(0, slash));
		HANDLE out = CreateFileW(full.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (out == INVALID_HANDLE_VALUE) return kCannotWrite;
		const std::string error = WriteEntry(a, out, written, nextReport, archiveSize, limits, progress, detail);
		if (error.empty() && archive_entry_mtime_is_set(entry) && archive_entry_mtime(entry) > 0) {
			const FILETIME ft = FileTimeOf(static_cast<time_t>(archive_entry_mtime(entry)));
			SetFileTime(out, nullptr, nullptr, &ft);
		}
		CloseHandle(out);
		if (!error.empty()) return error;
	}
	if (progress && !progress(archiveSize, archiveSize)) return kCanceled;
	return "";
}
}

bool IsRarFile(const std::string &path)
{
	HANDLE f = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (f == INVALID_HANDLE_VALUE) return false;
	unsigned char magic[7] = {};
	DWORD got = 0;
	ReadFile(f, magic, sizeof(magic), &got, nullptr);
	CloseHandle(f);
	static const unsigned char kRar[] = {'R', 'a', 'r', '!', 0x1a, 0x07};
	return got == sizeof(magic) && std::equal(kRar, kRar + sizeof(kRar), magic) && (magic[6] == 0 || magic[6] == 1);
}

std::string RarExtract(const std::string &rarPath, const std::string &dest, const ZipLimits &limits, const ZipProgress &progress, std::string *detail)
{
	HANDLE file = CreateFileA(rarPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (file == INVALID_HANDLE_VALUE) return kUnreadable;
	LARGE_INTEGER size = {};
	GetFileSizeEx(file, &size);
	CloseHandle(file);

	Reader reader;
	reader.a = archive_read_new();
	if (!reader.a) return kUnreadable;
	archive_read_support_format_rar(reader.a);
	archive_read_support_format_rar5(reader.a);
	if (archive_read_open_filename_w(reader.a, Widen(rarPath, CP_ACP).c_str(), kReadBlock) != ARCHIVE_OK) {
		if (detail) *detail = Detail(reader.a);
		return kUnreadable;
	}
	const std::wstring root = LongPath(dest);
	MakeDirs(root);
	return Unpack(reader.a, root, static_cast<uint64_t>(size.QuadPart), limits, progress, detail);
}
