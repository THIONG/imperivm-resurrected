// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>
#include <set>
#include <map>
#include "MinHook.h"
#include "core/crashlog.h"
#include "core/log.h"
#include "net/steamnet.h"
#include "addr/engine.h"
#include "addr/state.h"
#include "addr/script.h"
#include "addr/world.h"

#include "net/rejoin_internal.h"

using namespace rejoin_impl;

namespace rejoin_impl {
bool SafeCopy(void *dst, const void *src, size_t n)
{
	g_crashLogQuiet++;
	bool ok = true;
	__try {
		memcpy(dst, src, n);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		memset(dst, 0, n);
		ok = false;
	}
	g_crashLogQuiet--;
	return ok;
}
}

namespace rejoin_impl {
namespace {
#pragma pack(push, 1)
struct AiHead {
	uint8_t player;
	uint16_t firstFree;
	uint32_t refX, refY;
	uint16_t count;
};
#pragma pack(pop)

template <class Fn> void ForEachAi(Fn fn)
{
	char *mgr = ThreadManager(0);
	if (!mgr) return;
	std::set<char *> seen;
	ForEachThread(mgr, [&](uint32_t, char *, uint32_t *th) {
		char *owner = th ? reinterpret_cast<char *>(th[5]) : nullptr;
		if (!owner || *reinterpret_cast<uint32_t *>(owner) != addr::kAiVtable || !seen.insert(owner).second) return;
		char *exec = *reinterpret_cast<char **>(owner + 0x28);
		if (exec) fn(*reinterpret_cast<uint32_t *>(owner + 0x24), exec);
	});
}
}
}

namespace rejoin_impl {
void WriteAiState(FILE *f)
{
	ForEachAi([&](uint32_t player, char *exec) {
		uint32_t count = 0, active = 0;
		const uint32_t hash = AiOrdersHash(exec, count, active);
		fprintf(f, "## AI of player %u: %u commands (%u active), first free %d, reference (%d,%d), busy %d, state %08x\n", player,
			count, active, *reinterpret_cast<short *>(exec + 0x10), *reinterpret_cast<int *>(exec + 0x14), *reinterpret_cast<int *>(exec + 0x18),
			AiBusy(exec) ? 1 : 0, hash);
		char *begin = *reinterpret_cast<char **>(exec + 4);
		for (uint32_t k = 0; k < count; k++) {
			char *o = begin + k * 0x14;
			const short type = *reinterpret_cast<short *>(o);
			fprintf(f, "##   %u: type %d prio %d a %u b %u%s byte %u\n", k, type, *reinterpret_cast<short *>(o + 0x10),
				*reinterpret_cast<uint16_t *>(o + 4), *reinterpret_cast<uint16_t *>(o + 6),
				type == 2 ? " (train)" : "", *reinterpret_cast<uint8_t *>(o + 0xc));
		}
	});
}

std::string CollectAi()
{
	std::string out;
	ForEachAi([&](uint32_t player, char *exec) {
		char *begin = *reinterpret_cast<char **>(exec + 4), *end = *reinterpret_cast<char **>(exec + 8);
		const uint32_t n = begin && end > begin ? static_cast<uint32_t>((end - begin) / 0x14) : 0;
		AiHead h = {static_cast<uint8_t>(player), *reinterpret_cast<uint16_t *>(exec + 0x10), *reinterpret_cast<uint32_t *>(exec + 0x14),
			*reinterpret_cast<uint32_t *>(exec + 0x18), static_cast<uint16_t>(n)};
		out.append(reinterpret_cast<const char *>(&h), sizeof(h));
		if (n) out.append(begin, n * 0x14);
	});
	return out;
}

void ApplyAi()
{
	std::map<uint8_t, std::pair<AiHead, std::string>> host;
	for (size_t off = 0; off + sizeof(AiHead) <= g_blobs.ai.size();) {
		AiHead h;
		memcpy(&h, g_blobs.ai.data() + off, sizeof(h));
		off += sizeof(h);
		if (off + h.count * 0x14u > g_blobs.ai.size()) break;
		host[h.player] = {h, g_blobs.ai.substr(off, h.count * 0x14u)};
		off += h.count * 0x14u;
	}
	unsigned ais = 0, changed = 0, orders = 0, mismatched = 0;
	ForEachAi([&](uint32_t player, char *exec) {
		ais++;
		auto it = host.find(static_cast<uint8_t>(player));
		if (it == host.end()) return;
		const AiHead &h = it->second.first;
		char *begin = *reinterpret_cast<char **>(exec + 4), *end = *reinterpret_cast<char **>(exec + 8);
		const uint32_t n = begin && end > begin ? static_cast<uint32_t>((end - begin) / 0x14) : 0;
		bool diff = *reinterpret_cast<uint16_t *>(exec + 0x10) != h.firstFree || *reinterpret_cast<uint32_t *>(exec + 0x14) != h.refX ||
			*reinterpret_cast<uint32_t *>(exec + 0x18) != h.refY;
		*reinterpret_cast<uint16_t *>(exec + 0x10) = h.firstFree;
		*reinterpret_cast<uint32_t *>(exec + 0x14) = h.refX;
		*reinterpret_cast<uint32_t *>(exec + 0x18) = h.refY;
		if (n != h.count) {
			mismatched++;
			LogD("rejoin", "the AI of player %u has %u commands and the host's %u", player, n, h.count);
		}
		else {
			const char *src = it->second.second.data();
			for (uint32_t k = 0; k < n; k++) {
				char *mine = begin + k * 0x14;
				const char *theirs = src + k * 0x14;
				const bool train = *reinterpret_cast<const short *>(mine) == 2 || *reinterpret_cast<const short *>(theirs) == 2;
				if (memcmp(mine, theirs, 8) || memcmp(mine + 0xc, theirs + 0xc, 8) || (!train && memcmp(mine + 8, theirs + 8, 4))) {
					diff = true;
					orders++;
				}
				memcpy(mine, theirs, 8);
				memcpy(mine + 0xc, theirs + 0xc, 8);
				if (!train) memcpy(mine + 8, theirs + 8, 4);
			}
		}
		if (diff) changed++;
	});
	LogW("rejoin", "bot AI: %u, %u with changes copied from the host (%u commands), %u with a different number of commands",
		ais, changed, orders, mismatched);
}
}

namespace rejoin_impl {
namespace {
const uint32_t kPlayersOffset = 0x12cc;
const uint32_t kPlayerSize = 800;
const uint8_t kExplorationMarker = 0xfe;

char *Exploration()
{
	char *map = *reinterpret_cast<char **>(addr::kMap);
	return map ? *reinterpret_cast<char **>(map + 0xb0) : nullptr;
}

std::vector<uint16_t> ReadDequeU16(char *dq)
{
	std::vector<uint16_t> items;
	uint32_t q[4];
	if (!SafeCopy(q, dq + 4, sizeof(q)) || q[3] > 20000) return items;
	for (uint32_t k = 0; k < q[3]; k++) {
		const uint32_t idx = q[2] + k;
		uint32_t block = idx >> 3;
		if (block >= q[1]) block -= q[1];
		uint16_t *blockPtr = nullptr, item = 0xffff;
		if (q[0]) SafeCopy(&blockPtr, reinterpret_cast<uint16_t **>(q[0]) + block, 4);
		if (blockPtr) SafeCopy(&item, blockPtr + (idx & 7), 2);
		items.push_back(item);
	}
	return items;
}

void WriteDequeU16(char *dq, const uint16_t *items, uint32_t n)
{
	*reinterpret_cast<uint32_t *>(dq + 0x10) = 0;
	for (uint32_t k = 0; k < n; k++) {
		uint16_t item = items[k];
		reinterpret_cast<ThisPtr_t>(addr::kDequeUShortPushBack)(dq, nullptr, &item);
	}
}

const uint32_t kSquadEnemiesDeque = 0x6c;
const uint32_t kSquadSeenDeque = 0x98;

#pragma pack(push, 1)
struct SquadRec {
	uint8_t player;
	uint16_t index;
	uint16_t id;
	int32_t eval;
	uint32_t list3;
	uint32_t units;
	uint16_t nEnemies, nSeen;
};
struct GaikaHead {
	uint8_t marker;
	uint8_t player;
	uint16_t count;
};
#pragma pack(pop)

template <class Fn> void ForEachArmySquad(Fn fn)
{
	char *game = static_cast<char *>(Game());
	if (!game) return;
	for (uint32_t p = 0; p < 16; p++) {
		char *army = *reinterpret_cast<char **>(game + kPlayersOffset + p * kPlayerSize + 0x8c);
		if (!army) continue;
		char **begin = *reinterpret_cast<char ***>(army + 0xc), **end = *reinterpret_cast<char ***>(army + 0x10);
		if (!begin || end < begin || end - begin > 4096) continue;
		for (char **it = begin; it < end; it++) {
			if (*it && *reinterpret_cast<uint32_t *>(*it) == addr::kSquadVtable) fn(p, static_cast<uint16_t>(it - begin), *it);
		}
	}
}
}
}

namespace rejoin_impl {
std::string CollectSquads()
{
	std::string out;
	if (char *ex = Exploration()) {
		out += static_cast<char>(kExplorationMarker);
		out.append(ex + 0xf0, 4);
	}
	ForEachArmySquad([&](uint32_t p, uint16_t index, char *sq) {
		const std::vector<uint16_t> enemies = ReadDequeU16(sq + kSquadEnemiesDeque), seen = ReadDequeU16(sq + kSquadSeenDeque);
		SquadRec r = {static_cast<uint8_t>(p), index, *reinterpret_cast<uint16_t *>(sq + 0x34), *reinterpret_cast<int32_t *>(sq + 0x1c),
			*reinterpret_cast<uint32_t *>(sq + 0xa8), *reinterpret_cast<uint32_t *>(sq + 0x50), static_cast<uint16_t>(enemies.size()),
			static_cast<uint16_t>(seen.size())};
		out.append(reinterpret_cast<const char *>(&r), sizeof(r));
		if (!enemies.empty()) out.append(reinterpret_cast<const char *>(enemies.data()), enemies.size() * 2);
		if (!seen.empty()) out.append(reinterpret_cast<const char *>(seen.data()), seen.size() * 2);
	});
	ForEachAi([&](uint32_t player, char *) {
		char *game = static_cast<char *>(Game());
		char *ai = *reinterpret_cast<char **>(game + kPlayersOffset + player * kPlayerSize + 0x88);
		if (!ai) return;
		char *begin = *reinterpret_cast<char **>(ai + 0x30), *end = *reinterpret_cast<char **>(ai + 0x34);
		const uint32_t n = begin && end > begin ? static_cast<uint32_t>((end - begin) / 0x18) : 0;
		GaikaHead h = {0xff, static_cast<uint8_t>(player), static_cast<uint16_t>(n)};
		out.append(reinterpret_cast<const char *>(&h), sizeof(h));
		if (n) out.append(begin, n * 0x18);
	});
	return out;
}

void ApplySquads()
{
	std::map<std::pair<uint8_t, uint16_t>, std::pair<SquadRec, std::vector<uint16_t>>> host;
	std::map<uint8_t, std::string> gaikas;
	size_t off = 0;
	while (off < g_blobs.squads.size()) {
		if (static_cast<uint8_t>(g_blobs.squads[off]) == kExplorationMarker) {
			if (off + 5 > g_blobs.squads.size()) break;
			uint32_t revealed;
			memcpy(&revealed, g_blobs.squads.data() + off + 1, 4);
			off += 5;
			if (char *ex = Exploration()) {
				uint32_t &mine = *reinterpret_cast<uint32_t *>(ex + 0xf0);
				LogD("rejoin", "map fully revealed (exploration +0xf0): mine %u, host's %u%s", mine, revealed,
					mine != revealed ? " (copied)" : "");
				mine = revealed;
			}
			continue;
		}
		if (static_cast<uint8_t>(g_blobs.squads[off]) == 0xff) {
			if (off + sizeof(GaikaHead) > g_blobs.squads.size()) break;
			GaikaHead h;
			memcpy(&h, g_blobs.squads.data() + off, sizeof(h));
			off += sizeof(h);
			if (off + h.count * 0x18u > g_blobs.squads.size()) break;
			gaikas[h.player] = g_blobs.squads.substr(off, h.count * 0x18u);
			off += h.count * 0x18u;
			continue;
		}
		if (off + sizeof(SquadRec) > g_blobs.squads.size()) break;
		SquadRec r;
		memcpy(&r, g_blobs.squads.data() + off, sizeof(r));
		off += sizeof(r);
		const size_t n = static_cast<size_t>(r.nEnemies) + r.nSeen;
		if (off + n * 2 > g_blobs.squads.size()) break;
		std::vector<uint16_t> items(n);
		if (n) memcpy(items.data(), g_blobs.squads.data() + off, n * 2);
		off += n * 2;
		host[{r.player, r.index}] = {r, items};
	}
	unsigned squads = 0, evals = 0, missing = 0, otherId = 0, list3 = 0, units = 0, logged = 0, enemiesDiff = 0, seenDiff = 0;
	ForEachArmySquad([&](uint32_t p, uint16_t index, char *sq) {
		squads++;
		auto it = host.find({static_cast<uint8_t>(p), index});
		if (it == host.end()) {
			missing++;
			return;
		}
		const SquadRec r = it->second.first;
		const std::vector<uint16_t> items = it->second.second;
		host.erase(it);
		if (*reinterpret_cast<uint16_t *>(sq + 0x34) != r.id) {
			otherId++;
			return;
		}
		if (ReadDequeU16(sq + kSquadEnemiesDeque) != std::vector<uint16_t>(items.begin(), items.begin() + r.nEnemies)) {
			enemiesDiff++;
			WriteDequeU16(sq + kSquadEnemiesDeque, items.data(), r.nEnemies);
		}
		if (ReadDequeU16(sq + kSquadSeenDeque) != std::vector<uint16_t>(items.begin() + r.nEnemies, items.end())) {
			seenDiff++;
			WriteDequeU16(sq + kSquadSeenDeque, items.data() + r.nEnemies, r.nSeen);
		}
		int32_t &eval = *reinterpret_cast<int32_t *>(sq + 0x1c);
		if (eval != r.eval) {
			if (logged++ < 10) LogD("rejoin", "squad %u of player %u: rating %d, the host's %d", index, p, eval, r.eval);
			evals++;
			eval = r.eval;
		}
		if (*reinterpret_cast<uint32_t *>(sq + 0xa8) != r.list3) list3++;
		if (*reinterpret_cast<uint32_t *>(sq + 0x50) != r.units) units++;
	});
	LogD("rejoin", "squads: %u, %u with another rating (copied from the host), %u that the host does not have, "
		"%u of theirs that I do not have, %u with another number, %u with another number of units; lists "
		"copied from the host: %u of enemies, %u of what they see (%u remain different)",
		squads, evals, missing, static_cast<unsigned>(host.size()), otherId, units, enemiesDiff, seenDiff, list3);
	unsigned ais = 0, changed = 0, cells = 0, mismatched = 0;
	ForEachAi([&](uint32_t player, char *) {
		char *game = static_cast<char *>(Game());
		char *ai = *reinterpret_cast<char **>(game + kPlayersOffset + player * kPlayerSize + 0x88);
		auto it = gaikas.find(static_cast<uint8_t>(player));
		if (!ai || it == gaikas.end()) return;
		ais++;
		char *begin = *reinterpret_cast<char **>(ai + 0x30), *end = *reinterpret_cast<char **>(ai + 0x34);
		const size_t n = begin && end > begin ? static_cast<size_t>((end - begin) / 0x18) : 0;
		if (n * 0x18 != it->second.size()) {
			mismatched++;
			LogD("rejoin", "the bot of player %u has %u zones and the host's %u", player, static_cast<unsigned>(n),
				static_cast<unsigned>(it->second.size() / 0x18));
			return;
		}
		unsigned diff = 0;
		for (size_t k = 0; k < n; k++) {
			if (memcmp(begin + k * 0x18, it->second.data() + k * 0x18, 0x18)) diff++;
		}
		if (diff) {
			changed++;
			cells += diff;
			memcpy(begin, it->second.data(), n * 0x18);
		}
	});
	LogW("rejoin", "bot zones: %u bots, %u with different zones (%u zones copied from the host), %u with another number of zones",
		ais, changed, cells, mismatched);
}
}

namespace rejoin_impl {
namespace {
const unsigned kHandleCount = 0x10000;

bool IsComplexQuery(char *obj)
{
	uint32_t *vt = nullptr;
	uint32_t slot = 0;
	return obj && SafeCopy(&vt, obj, 4) && vt && SafeCopy(&slot, vt + 8, 4) && slot == addr::kQueryGetResults;
}
}
}

namespace rejoin_impl {
std::string CollectQueries()
{
	std::string out;
	unsigned queries = 0;
	for (unsigned h = 0; h < kHandleCount; h++) {
		char *obj = reinterpret_cast<char **>(addr::kHandleTable)[h];
		if (!IsComplexQuery(obj)) continue;
		uint32_t q[5];
		uint32_t times[2];
		if (!SafeCopy(q, obj + 0x28, sizeof(q)) || !SafeCopy(times, obj + 0x4c, sizeof(times)) || q[3] > 5000) continue;
		std::vector<uint16_t> items;
		for (uint32_t k = 0; k < q[3]; k++) {
			const uint32_t idx = q[2] + k;
			uint32_t block = idx >> 3;
			if (block >= q[1]) block -= q[1];
			uint16_t *blockPtr = nullptr, item = 0xffff;
			if (q[0]) SafeCopy(&blockPtr, reinterpret_cast<uint16_t **>(q[0]) + block, 4);
			if (blockPtr) SafeCopy(&item, blockPtr + (idx & 7), 2);
			items.push_back(item);
		}
		const uint16_t hh = static_cast<uint16_t>(h);
		const uint32_t n = static_cast<uint32_t>(items.size());
		out.append(reinterpret_cast<const char *>(&hh), 2);
		out.append(reinterpret_cast<const char *>(times), 8);
		out.append(reinterpret_cast<const char *>(&n), 4);
		if (n) out.append(reinterpret_cast<const char *>(items.data()), n * 2);
		queries++;
	}
	LogD("rejoin", "%u script queries sent", queries);
	return out;
}

void ApplyQueries()
{
	const std::string &d = g_blobs.queries;
	size_t off = 0;
	unsigned applied = 0, changed = 0, missing = 0;
	while (off + 14 <= d.size()) {
		const uint16_t h = *reinterpret_cast<const uint16_t *>(d.data() + off);
		const uint32_t *times = reinterpret_cast<const uint32_t *>(d.data() + off + 2);
		const uint32_t n = *reinterpret_cast<const uint32_t *>(d.data() + off + 10);
		off += 14;
		if (off + n * 2 > d.size()) break;
		const uint16_t *items = reinterpret_cast<const uint16_t *>(d.data() + off);
		off += n * 2;
		char *obj = reinterpret_cast<char **>(addr::kHandleTable)[h];
		if (!IsComplexQuery(obj)) {
			missing++;
			continue;
		}
		bool same = *reinterpret_cast<uint32_t *>(obj + 0x4c) == times[0] && *reinterpret_cast<uint32_t *>(obj + 0x34) == n;
		*reinterpret_cast<uint32_t *>(obj + 0x4c) = times[0];
		*reinterpret_cast<uint32_t *>(obj + 0x50) = times[1];
		*reinterpret_cast<uint32_t *>(obj + 0x34) = 0;
		for (uint32_t k = 0; k < n; k++) {
			uint16_t item = items[k];
			reinterpret_cast<ThisPtr_t>(addr::kDequeUShortPushBack)(obj + 0x24, nullptr, &item);
		}
		applied++;
		if (!same) changed++;
	}
	LogW("rejoin", "script queries: %u copied from the host (%u were different), %u that I do not have", applied, changed, missing);
}
}
