// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>
#include "MinHook.h"
#include "core/log.h"
#include "addr/script.h"
#include "addr/world.h"

#include "net/rejoin_internal.h"

using namespace rejoin_impl;

namespace rejoin_impl {
namespace {
std::vector<int> MapObjectHandles(const std::string &dump)
{
	std::vector<int> out;
	int handle = -1;
	size_t pos = 0;
	while (pos < dump.size()) {
		size_t end = dump.find('\n', pos);
		if (end == std::string::npos) end = dump.size();
		if (dump.compare(pos, 7, "handle=") == 0) handle = atoi(dump.c_str() + pos + 7);
		else if (dump.compare(pos, 14, "ScriptClass = ") == 0 && handle >= 0) {
			out.push_back(handle);
			handle = -1;
		}
		pos = end + 1;
	}
	return out;
}
}
}

namespace rejoin_impl {
std::string CollectObjects(const std::string &dump)
{
	std::string out;
	for (int h : MapObjectHandles(dump)) {
		char *obj = reinterpret_cast<char **>(addr::kHandleTable)[h & 0xffff];
		if (!obj) continue;
		const uint16_t hh = static_cast<uint16_t>(h);
		out.append(reinterpret_cast<const char *>(&hh), 2);
		out.append(obj + kObjFirst, kObjFields * 4);
		uint32_t ship[3] = {};
		if (*reinterpret_cast<uint32_t *>(obj) == addr::kShipVtable) SafeCopy(ship, obj + kShipLoadFields, sizeof(ship));
		out.append(reinterpret_cast<const char *>(ship), sizeof(ship));
	}
	return out;
}

void DumpObjectsBinary(const std::string &dump, const char *file)
{
	EnsureLogDir();
	FILE *f = _fsopen(DiskPath(file).c_str(), "wb", _SH_DENYWR);
	if (!f) return;
	unsigned written = 0, noSize = 0;
	std::vector<char> buf;
	for (int h : MapObjectHandles(dump)) {
		char *obj = reinterpret_cast<char **>(addr::kHandleTable)[h & 0xffff];
		if (!obj) continue;
		SIZE_T size = 0x300;
		buf.assign(size, 0);
		if (!SafeCopy(buf.data(), obj, size)) {
			noSize++;
			size = 0x1d4;
			if (!SafeCopy(buf.data(), obj, size)) continue;
		}
		const uint16_t hh = static_cast<uint16_t>(h);
		const uint32_t vt = *reinterpret_cast<uint32_t *>(buf.data()), sz = static_cast<uint32_t>(size);
		fwrite(&hh, 2, 1, f);
		fwrite(&vt, 4, 1, f);
		fwrite(&sz, 4, 1, f);
		fwrite(buf.data(), 1, size, f);
		written++;
	}
	fclose(f);
	LogD("rejoin", "binary dump of %u objects in %s (%u without heap size)", written, file, noSize);
}

std::string CollectHandles()
{
	std::string out(16 + 0x20000, '\0');
	uint32_t *head = reinterpret_cast<uint32_t *>(&out[0]);
	head[0] = *reinterpret_cast<uint32_t *>(addr::kHandleFirstFreeSync);
	head[1] = *reinterpret_cast<uint32_t *>(addr::kHandleLastFreeSync);
	head[2] = *reinterpret_cast<uint32_t *>(addr::kHandleFirstFreeAsync);
	head[3] = *reinterpret_cast<uint32_t *>(addr::kHandleLastFreeAsync);
	memcpy(&out[16], reinterpret_cast<void *>(addr::kHandleFreeList), 0x20000);
	return out;
}
}

namespace rejoin_impl {
namespace {
static std::vector<uint16_t> FreeOrder(const uint16_t *links, uint32_t first, uint32_t last, const uint32_t *table, size_t n)
{
	std::vector<uint16_t> out;
	uint32_t h = first;
	for (size_t steps = 0; h != last && h < 0x10000 && out.size() < n && steps < 0x10000; steps++) {
		if (!table[h]) out.push_back(static_cast<uint16_t>(h));
		h = links[h];
	}
	return out;
}
}
}

namespace rejoin_impl {
void ApplyHandles()
{
	if (g_blobs.handles.size() != 16 + 0x20000) {
		LogW("rejoin", "free-number list with an odd size (%u bytes); not copied", static_cast<unsigned>(g_blobs.handles.size()));
		return;
	}
	const uint32_t *head = reinterpret_cast<const uint32_t *>(g_blobs.handles.data());
	if (head[0] > 0xffff || head[1] > 0xffff) {
		LogW("rejoin", "the free-number list has an impossible start or end; not copied");
		return;
	}
	const uint16_t *hostLinks = reinterpret_cast<const uint16_t *>(g_blobs.handles.data() + 16);
	const uint32_t *table = reinterpret_cast<const uint32_t *>(addr::kHandleTable);
	const uint16_t *mineLinks = reinterpret_cast<const uint16_t *>(addr::kHandleFreeList);
	const std::vector<uint16_t> mine = FreeOrder(mineLinks, *reinterpret_cast<uint32_t *>(addr::kHandleFirstFreeSync), *reinterpret_cast<uint32_t *>(addr::kHandleLastFreeSync), table, 4000);
	const std::vector<uint16_t> host = FreeOrder(hostLinks, head[0], head[1], table, 4000);
	size_t diff = 0;
	for (size_t k = 0; k < host.size() && k < mine.size(); k++) diff += host[k] != mine[k];
	char first[96] = "";
	for (size_t k = 0; k < 6 && k < host.size(); k++) sprintf_s(first + strlen(first), sizeof(first) - strlen(first), " %u/%u", host[k], k < mine.size() ? mine[k] : 0);
	LogW("rejoin", "free (synchronous) object numbers: first %u of the host and mine, %u different (host/mine:%s); the host's list is copied",
		static_cast<unsigned>(host.size()), static_cast<unsigned>(diff), first);
	memcpy(reinterpret_cast<void *>(addr::kHandleFreeList), hostLinks, 40000 * 2);
	*reinterpret_cast<uint32_t *>(addr::kHandleFirstFreeSync) = head[0];
	*reinterpret_cast<uint32_t *>(addr::kHandleLastFreeSync) = head[1];
}

void FixObjects()
{
	const size_t rec = 2 + kObjFields * 4 + 12;
	unsigned perField[kObjFields] = {};
	size_t objects = 0, changed = 0, ships = 0, shipsFixed = 0;
	for (size_t off = 0; off + rec <= g_blobs.objects.size(); off += rec) {
		const uint16_t h = *reinterpret_cast<const uint16_t *>(g_blobs.objects.data() + off);
		const uint32_t *host = reinterpret_cast<const uint32_t *>(g_blobs.objects.data() + off + 2);
		char *obj = reinterpret_cast<char **>(addr::kHandleTable)[h];
		if (!obj) continue;
		objects++;
		bool any = false;
		for (unsigned k = 0; k < kObjFields; k++) {
			const unsigned field = kObjFirst + k * 4;
			if (field >= 0x2c && field <= 0x40) continue;
			uint32_t &mine = *reinterpret_cast<uint32_t *>(obj + field);
			if (mine == host[k]) continue;
			perField[k]++;
			mine = host[k];
			any = true;
		}
		if (any) changed++;
		if (*reinterpret_cast<uint32_t *>(obj) == addr::kShipVtable) {
			ships++;
			const uint32_t *tail = host + kObjFields;
			uint32_t *mine = reinterpret_cast<uint32_t *>(obj + kShipLoadFields);
			if (memcmp(mine, tail, 12)) {
				memcpy(mine, tail, 12);
				shipsFixed++;
			}
		}
	}
	LogD("rejoin", "ships: %u, %u with another firing cycle (copied from the host)", static_cast<unsigned>(ships),
		static_cast<unsigned>(shipsFixed));
	std::string fields;
	for (unsigned k = 0; k < kObjFields; k++) {
		if (!perField[k]) continue;
		char b[32];
		sprintf_s(b, " +%02x:%u", kObjFirst + k * 4, perField[k]);
		fields += b;
	}
	LogW("rejoin", "map objects: %u, %u with different fields (copied from the host)%s", static_cast<unsigned>(objects),
		static_cast<unsigned>(changed), fields.empty() ? "" : (", by field:" + fields).c_str());
}
}
