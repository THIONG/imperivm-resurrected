// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "core/zipfile.h"

#include <windows.h>

#include <algorithm>
#include <cstring>
#include <ctime>

#include "core/archivefs.h"
#include "core/pathsafe.h"
#include "miniz.h"

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

const uint32_t kUnixTypeMask = 0170000;
const uint32_t kUnixRegular = 0100000;
const uint32_t kUnixDirectory = 0040000;
const unsigned kMadeByUnix = 3;
const unsigned kMadeByMac = 19;
const mz_uint16 kUtf8Names = 1u << 11;
const uint64_t kProgressStep = 1u << 20;
const unsigned long long kUnixEpochAsFileTime = 116444736000000000ULL;

std::string EntryNameUtf8(const char *raw, mz_uint16 flags)
{
	const std::string name = raw;
	if (name.empty()) return name;
	std::wstring wide = Widen(name, CP_UTF8);
	if (wide.empty() && !(flags & kUtf8Names)) wide = Widen(name, CP_OEMCP);
	return Utf8(wide);
}

size_t ReadAt(void *opaque, mz_uint64 offset, void *buffer, size_t n)
{
	OVERLAPPED at = {};
	at.Offset = static_cast<DWORD>(offset);
	at.OffsetHigh = static_cast<DWORD>(offset >> 32);
	DWORD got = 0;
	return ReadFile(static_cast<HANDLE>(opaque), buffer, static_cast<DWORD>(n), &got, &at) ? got : 0;
}

size_t WriteAt(void *opaque, mz_uint64 offset, const void *buffer, size_t n)
{
	OVERLAPPED at = {};
	at.Offset = static_cast<DWORD>(offset);
	at.OffsetHigh = static_cast<DWORD>(offset >> 32);
	DWORD put = 0;
	return WriteFile(static_cast<HANDLE>(opaque), buffer, static_cast<DWORD>(n), &put, &at) && put == n ? n : 0;
}

struct Sink {
	HANDLE out = INVALID_HANDLE_VALUE;
	uint64_t done = 0;
	uint64_t total = 0;
	uint64_t nextReport = 0;
	const ZipProgress *progress = nullptr;
	bool canceled = false;
};

size_t WriteNext(void *opaque, mz_uint64, const void *buffer, size_t n)
{
	Sink &s = *static_cast<Sink *>(opaque);
	DWORD put = 0;
	if (!WriteFile(s.out, buffer, static_cast<DWORD>(n), &put, nullptr) || put != n) return 0;
	s.done += n;
	if (*s.progress && s.done >= s.nextReport) {
		s.nextReport = s.done + kProgressStep;
		if (!(*s.progress)(s.done, s.total)) {
			s.canceled = true;
			return 0;
		}
	}
	return n;
}

time_t TimeOf(const FILETIME &ft)
{
	const unsigned long long value = (static_cast<unsigned long long>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
	return value > kUnixEpochAsFileTime ? static_cast<time_t>((value - kUnixEpochAsFileTime) / 10000000ULL) : 0;
}

struct Item {
	mz_uint index = 0;
	std::wstring path;
	bool directory = false;
	time_t time = 0;
};

std::string Plan(mz_zip_archive &zip, const ZipLimits &limits, std::vector<Item> &items, uint64_t &total)
{
	const mz_uint count = mz_zip_reader_get_num_files(&zip);
	if (limits.maxEntries && count > limits.maxEntries) return kTooLarge;
	for (mz_uint i = 0; i < count; i++) {
		mz_zip_archive_file_stat st;
		if (!mz_zip_reader_file_stat(&zip, i, &st)) return kUnreadable;
		if (!st.m_is_supported || st.m_is_encrypted) return kDamaged;
		const unsigned madeBy = st.m_version_made_by >> 8;
		if (madeBy == kMadeByUnix || madeBy == kMadeByMac) {
			const uint32_t type = (st.m_external_attr >> 16) & kUnixTypeMask;
			if (type && type != kUnixRegular && type != kUnixDirectory) return kSpecial;
		}
		const std::string name = EntryNameUtf8(st.m_filename, st.m_bit_flag);
		if (name.empty() && st.m_filename[0]) return kUnreadable;
		std::string rel;
		if (!ZipEntryPath(name, rel)) return kOutside;
		if (rel.empty()) continue;
		std::replace(rel.begin(), rel.end(), '/', '\\');
		total += st.m_uncomp_size;
		if (limits.maxBytes && total > limits.maxBytes) return kTooLarge;
		items.push_back({i, Widen(rel, CP_UTF8), st.m_is_directory != 0, st.m_time});
	}
	return "";
}

std::string Unpack(mz_zip_archive &zip, const std::wstring &root, const std::vector<Item> &items, uint64_t total, const ZipProgress &progress)
{
	Sink sink;
	sink.total = total;
	sink.progress = &progress;
	for (const Item &it : items) {
		const std::wstring full = root + L"\\" + it.path;
		if (it.directory) {
			MakeDirs(full);
			continue;
		}
		const size_t slash = full.find_last_of(L'\\');
		if (slash != std::wstring::npos) MakeDirs(full.substr(0, slash));
		sink.out = CreateFileW(full.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (sink.out == INVALID_HANDLE_VALUE) return kCannotWrite;
		const bool ok = mz_zip_reader_extract_to_callback(&zip, it.index, WriteNext, &sink, 0) != MZ_FALSE;
		if (ok && it.time > 0) {
			const FILETIME ft = FileTimeOf(it.time);
			SetFileTime(sink.out, nullptr, nullptr, &ft);
		}
		CloseHandle(sink.out);
		if (sink.canceled) return kCanceled;
		if (!ok) return kDamaged;
	}
	if (progress && !progress(total, total)) return kCanceled;
	return "";
}

bool AddPath(mz_zip_archive &zip, const std::wstring &root, const std::wstring &rel)
{
	const std::wstring full = root + L"\\" + rel;
	const DWORD attributes = GetFileAttributesW(full.c_str());
	if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return true;
	std::wstring name = rel;
	std::replace(name.begin(), name.end(), L'\\', L'/');
	if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
		if (!mz_zip_writer_add_mem(&zip, (Utf8(name) + "/").c_str(), nullptr, 0, MZ_NO_COMPRESSION)) return false;
		WIN32_FIND_DATAW fd;
		HANDLE find = FindFirstFileW((full + L"\\*").c_str(), &fd);
		if (find == INVALID_HANDLE_VALUE) return true;
		bool ok = true;
		do {
			const std::wstring child = fd.cFileName;
			if (child != L"." && child != L"..") ok = AddPath(zip, root, rel + L"\\" + child);
		} while (ok && FindNextFileW(find, &fd));
		FindClose(find);
		return ok;
	}
	HANDLE in = CreateFileW(full.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (in == INVALID_HANDLE_VALUE) return false;
	LARGE_INTEGER size = {};
	FILETIME written = {};
	GetFileSizeEx(in, &size);
	GetFileTime(in, nullptr, nullptr, &written);
	const MZ_TIME_T time = TimeOf(written);
	const bool ok = mz_zip_writer_add_read_buf_callback(&zip, Utf8(name).c_str(), ReadAt, in, static_cast<mz_uint64>(size.QuadPart), &time, nullptr, 0,
						MZ_DEFAULT_LEVEL, nullptr, 0, nullptr, 0) != MZ_FALSE;
	CloseHandle(in);
	return ok;
}
}

bool IsZipFile(const std::string &path)
{
	HANDLE f = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (f == INVALID_HANDLE_VALUE) return false;
	unsigned char magic[4] = {};
	DWORD got = 0;
	ReadFile(f, magic, sizeof(magic), &got, nullptr);
	CloseHandle(f);
	return got == sizeof(magic) && magic[0] == 'P' && magic[1] == 'K' && ((magic[2] == 3 && magic[3] == 4) || (magic[2] == 5 && magic[3] == 6));
}

bool ZipEntryPath(const std::string &name, std::string &rel)
{
	rel.clear();
	std::string path = name;
	std::replace(path.begin(), path.end(), '\\', '/');
	if (!path.empty() && path[0] == '/') return false;
	size_t start = 0;
	while (start <= path.size()) {
		size_t end = path.find('/', start);
		if (end == std::string::npos) end = path.size();
		const std::string part = path.substr(start, end - start);
		if (!part.empty() && part != ".") rel += (rel.empty() ? "" : "/") + part;
		start = end + 1;
	}
	return rel.empty() || pathsafe::SafeRelative(rel);
}

std::string ZipExtract(const std::string &zipPath, const std::string &dest, const ZipLimits &limits, const ZipProgress &progress)
{
	HANDLE file = CreateFileA(zipPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);
	if (file == INVALID_HANDLE_VALUE) return kUnreadable;
	LARGE_INTEGER size = {};
	GetFileSizeEx(file, &size);
	mz_zip_archive zip;
	mz_zip_zero_struct(&zip);
	zip.m_pRead = ReadAt;
	zip.m_pIO_opaque = file;
	std::string error;
	if (!mz_zip_reader_init(&zip, static_cast<mz_uint64>(size.QuadPart), 0)) {
		error = kUnreadable;
	}
	else {
		std::vector<Item> items;
		uint64_t total = 0;
		error = Plan(zip, limits, items, total);
		if (error.empty()) {
			const std::wstring root = LongPath(dest);
			MakeDirs(root);
			error = Unpack(zip, root, items, total, progress);
		}
		mz_zip_reader_end(&zip);
	}
	CloseHandle(file);
	return error;
}

bool ZipCreate(const std::string &zipPath, const std::string &folder, const std::vector<std::string> &names)
{
	HANDLE out = CreateFileA(zipPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (out == INVALID_HANDLE_VALUE) return false;
	mz_zip_archive zip;
	mz_zip_zero_struct(&zip);
	zip.m_pWrite = WriteAt;
	zip.m_pIO_opaque = out;
	bool ok = mz_zip_writer_init(&zip, 0) != MZ_FALSE;
	const std::wstring root = LongPath(folder);
	for (size_t i = 0; ok && i < names.size(); i++) ok = AddPath(zip, root, Widen(names[i], CP_ACP));
	ok = ok && mz_zip_writer_finalize_archive(&zip) != MZ_FALSE;
	mz_zip_writer_end(&zip);
	CloseHandle(out);
	if (!ok) DeleteFileA(zipPath.c_str());
	return ok;
}
