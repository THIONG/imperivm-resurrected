// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "core/rarfile.h"

#define LIBARCHIVE_STATIC 1

#include <windows.h>

#include <cstdint>
#include <cstring>
#include <vector>

#include "archive.h"
#include "archive_entry.h"
#include "core/winpath.h"

namespace {
using winpath::FileTimeOf;
using winpath::LongPath;
using winpath::MakeDirs;
using winpath::Utf8;
using winpath::Widen;

const char kUnreadable[] = "the archive could not be read (unsupported format or damaged file)";
const char kDamaged[] = "could not extract (unsupported format or damaged file)";
const char kSpecial[] = "the archive contains links or special files: it will not be installed";
const char kTooLarge[] = "the archive is too large to install";
const char kOutside[] = "the archive contains a path outside its folder: it will not be installed";
const char kCannotWrite[] = "could not write the extracted files";
const char kEncrypted[] = "the archive is protected with a password: it cannot be installed";
const char kCanceled[] = "canceled";

const size_t kBlock = 1u << 16;
const uint64_t kProgressStep = 1u << 20;

struct Reader {
	archive *a = archive_read_new();
	~Reader() { archive_read_free(a); }
};

std::string EntryName(archive_entry *entry)
{
	const wchar_t *wide = archive_entry_pathname_w(entry);
	if (wide && *wide) return Utf8(wide);
	const char *raw = archive_entry_pathname(entry);
	return raw ? Utf8(Widen(raw, CP_OEMCP)) : std::string();
}

std::string ReadFailure(archive *a)
{
	return archive_read_has_encrypted_entries(a) == 1 ? kEncrypted : kDamaged;
}

struct Job {
	const ZipLimits *limits = nullptr;
	const ZipProgress *progress = nullptr;
	uint64_t archiveSize = 0;
	uint64_t declared = 0;
	uint64_t written = 0;
	uint64_t nextReport = 0;
	size_t entries = 0;
};

std::string Unpack(archive *a, archive_entry *entry, const std::wstring &full, Job &job, std::vector<char> &buffer)
{
	const size_t slash = full.find_last_of(L'\\');
	if (slash != std::wstring::npos) MakeDirs(full.substr(0, slash));
	HANDLE out = CreateFileW(full.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (out == INVALID_HANDLE_VALUE) return kCannotWrite;
	std::string error;
	for (;;) {
		const la_ssize_t got = archive_read_data(a, buffer.data(), buffer.size());
		if (got == 0) break;
		if (got < 0) {
			error = ReadFailure(a);
			break;
		}
		DWORD put = 0;
		if (!WriteFile(out, buffer.data(), static_cast<DWORD>(got), &put, nullptr) || put != static_cast<DWORD>(got)) {
			error = kCannotWrite;
			break;
		}
		job.written += static_cast<uint64_t>(got);
		if (job.limits->maxBytes && job.written > job.limits->maxBytes) {
			error = kTooLarge;
			break;
		}
		if (*job.progress && job.written >= job.nextReport) {
			job.nextReport = job.written + kProgressStep;
			if (!(*job.progress)(static_cast<uint64_t>(archive_filter_bytes(a, -1)), job.archiveSize)) {
				error = kCanceled;
				break;
			}
		}
	}
	if (error.empty() && archive_entry_mtime_is_set(entry) && archive_entry_mtime(entry) > 0) {
		const FILETIME ft = FileTimeOf(archive_entry_mtime(entry));
		SetFileTime(out, nullptr, nullptr, &ft);
	}
	CloseHandle(out);
	return error;
}

std::string Walk(archive *a, const std::wstring &root, Job &job)
{
	std::vector<char> buffer(kBlock);
	archive_entry *entry = nullptr;
	for (;;) {
		const int r = archive_read_next_header(a, &entry);
		if (r == ARCHIVE_EOF) return "";
		if (r < ARCHIVE_WARN) return ReadFailure(a);
		const unsigned type = archive_entry_filetype(entry);
		if ((type != AE_IFREG && type != AE_IFDIR) || archive_entry_hardlink(entry) || archive_entry_symlink(entry)) return kSpecial;
		if (archive_entry_is_data_encrypted(entry) || archive_entry_is_metadata_encrypted(entry)) return kEncrypted;
		const std::string name = EntryName(entry);
		if (name.empty()) return kUnreadable;
		std::string rel;
		if (!ZipEntryPath(name, rel)) return kOutside;
		if (rel.empty()) continue;
		if (job.limits->maxEntries && ++job.entries > job.limits->maxEntries) return kTooLarge;
		if (archive_entry_size_is_set(entry)) job.declared += static_cast<uint64_t>(archive_entry_size(entry));
		if (job.limits->maxBytes && job.declared > job.limits->maxBytes) return kTooLarge;
		for (char &c : rel) {
			if (c == '/') c = '\\';
		}
		const std::wstring full = root + L"\\" + Widen(rel, CP_UTF8);
		if (type == AE_IFDIR) {
			MakeDirs(full);
			continue;
		}
		const std::string error = Unpack(a, entry, full, job, buffer);
		if (!error.empty()) return error;
	}
}
}

bool IsRarFile(const std::string &path)
{
	HANDLE f = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (f == INVALID_HANDLE_VALUE) return false;
	unsigned char magic[8] = {};
	DWORD got = 0;
	ReadFile(f, magic, sizeof(magic), &got, nullptr);
	CloseHandle(f);
	static const unsigned char kRar4[7] = {'R', 'a', 'r', '!', 0x1a, 0x07, 0x00};
	static const unsigned char kRar5[8] = {'R', 'a', 'r', '!', 0x1a, 0x07, 0x01, 0x00};
	return (got >= sizeof(kRar4) && !memcmp(magic, kRar4, sizeof(kRar4))) || (got == sizeof(kRar5) && !memcmp(magic, kRar5, sizeof(kRar5)));
}

std::string RarExtract(const std::string &rar, const std::string &dest, const ZipLimits &limits, const ZipProgress &progress)
{
	WIN32_FILE_ATTRIBUTE_DATA info = {};
	if (!GetFileAttributesExW(Widen(rar, CP_ACP).c_str(), GetFileExInfoStandard, &info)) return kUnreadable;
	Reader reader;
	archive_read_support_format_rar(reader.a);
	archive_read_support_format_rar5(reader.a);
	if (archive_read_open_filename_w(reader.a, Widen(rar, CP_ACP).c_str(), kBlock) != ARCHIVE_OK) return kUnreadable;
	Job job;
	job.limits = &limits;
	job.progress = &progress;
	job.archiveSize = (static_cast<uint64_t>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
	const std::wstring root = LongPath(dest);
	MakeDirs(root);
	std::string error = Walk(reader.a, root, job);
	if (error.empty() && progress && !progress(job.archiveSize, job.archiveSize)) error = kCanceled;
	return error;
}
