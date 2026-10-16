// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <set>
#include <map>
#include <tuple>
#include "MinHook.h"
#include "core/log.h"
#include "net/steamnet.h"
#include "addr/engine.h"
#include "addr/script.h"
#include "addr/world.h"

#include "net/rejoin_internal.h"

using namespace rejoin_impl;

namespace rejoin_impl {
namespace {
uint32_t OwnerTag(char *owner)
{
	if (!owner) return 0;
	uint32_t vt = 0;
	if (!SafeCopy(&vt, owner, 4)) return 0;
	if (vt == addr::kAiVtable) {
		uint32_t player = 0;
		SafeCopy(&player, owner + 0x24, 4);
		return 0x80000000u | (player & 0xff);
	}
	if (vt != addr::kSquadVtable) return 0;
	uint32_t id = 0, q[4] = {};
	SafeCopy(&id, owner + 0x34, 4);
	uint16_t first = 0xffff;
	if (SafeCopy(q, owner + 0x44, sizeof(q)) && q[0] && q[1] && q[3]) {
		uint32_t block = q[2] >> 3;
		if (block >= q[1]) block -= q[1];
		uint16_t *blockPtr = nullptr;
		if (SafeCopy(&blockPtr, reinterpret_cast<uint16_t **>(q[0]) + block, 4) && blockPtr) SafeCopy(&first, blockPtr + (q[2] & 7), 2);
	}
	return (id << 16) | first;
}

typedef std::tuple<uint16_t, uint32_t, uint32_t, uint32_t> ThreadKey;

uint16_t ThreadOwnerHandle(uint32_t *thread)
{
	char *owner = reinterpret_cast<char *>(thread[5]);
	return owner ? *reinterpret_cast<uint16_t *>(owner + 8) : 0xffff;
}

const uint16_t kStoppedBucket = 0xffff;

template <class Fn> void ForEachStoppedThread(char *mgr, Fn fn)
{
	std::set<char *> owners;
	ForEachThread(mgr, [&](uint32_t, char *, uint32_t *th) {
		if (th && th[5]) owners.insert(reinterpret_cast<char *>(th[5]));
	});
	for (char *owner : owners) {
		uint32_t **arr = *reinterpret_cast<uint32_t ***>(owner + 0xc);
		const uint32_t n = *reinterpret_cast<uint32_t *>(owner + 0x10);
		for (uint32_t k = 0; arr && k < n && k < 1000; k++) {
			if (arr[k] && arr[k][1] == 0xffffffff) fn(owner, arr[k]);
		}
	}
}
}
}

namespace rejoin_impl {
std::string CollectThreads()
{
	std::string out;
	for (int m = 0; m < 2; m++) {
		char *mgr = ThreadManager(m);
		if (!mgr) continue;
		auto add = [&](uint16_t bucket, uint32_t *th) {
			const uint32_t vt = th[5] ? *reinterpret_cast<uint32_t *>(th[5]) : 0;
			const uint16_t handle = ThreadOwnerHandle(th);
			const uint32_t tag = handle == 0xffff ? OwnerTag(reinterpret_cast<char *>(th[5])) : 0;
			ThreadRec r = {static_cast<uint8_t>(m), bucket, handle, th[4], th[1], vt, th[2], th[3], tag};
			out.append(reinterpret_cast<const char *>(&r), sizeof(r));
		};
		ForEachThread(mgr, [&](uint32_t b, char *, uint32_t *th) {
			if (th) add(static_cast<uint16_t>(b), th);
		});
		if (m == 0) ForEachStoppedThread(mgr, [&](char *, uint32_t *th) { add(kStoppedBucket, th); });
	}
	return out;
}
}

namespace rejoin_impl {
namespace {
const uint32_t kSettlementMarkerThread = 8;

bool RunThreadNow(void *owner, uint32_t *thread)
{
	__try {
		reinterpret_cast<ThisPtr_t>(Vt(owner)[3])(owner, nullptr, thread);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}
}
}

namespace rejoin_impl {
void FixThreads()
{
	const size_t count = g_blobs.threads.size() / sizeof(ThreadRec);
	const ThreadRec *host = reinterpret_cast<const ThreadRec *>(g_blobs.threads.data());
	for (int m = 0; m < 1; m++) {
		char *mgr = ThreadManager(m);
		if (!mgr) continue;
		std::map<ThreadKey, std::vector<uint32_t *>> mine;
		size_t mineCount = 0;
		ForEachThread(mgr, [&](uint32_t, char *, uint32_t *th) {
			if (!th) return;
			const uint16_t handle = ThreadOwnerHandle(th);
			const uint32_t vt = th[5] ? *reinterpret_cast<uint32_t *>(th[5]) : 0;
			const uint32_t tag = handle == 0xffff ? OwnerTag(reinterpret_cast<char *>(th[5])) : 0;
			mine[ThreadKey(handle, vt, tag, th[4])].push_back(th);
			mineCount++;
		});
		std::map<uint32_t, std::vector<uint32_t *>> wanted;
		size_t missing = 0, dueFixed = 0, matched = 0, created = 0;
		std::map<uint32_t, std::vector<void *>> ownersByVt;
		ForEachThread(mgr, [&](uint32_t, char *, uint32_t *th) {
			if (!th || !th[5] || ThreadOwnerHandle(th) != 0xffff) return;
			std::vector<void *> &v = ownersByVt[*reinterpret_cast<uint32_t *>(th[5])];
			if (std::find(v.begin(), v.end(), reinterpret_cast<void *>(th[5])) == v.end()) v.push_back(reinterpret_cast<void *>(th[5]));
		});
		for (size_t i = 0; i < count; i++) {
			const ThreadRec &r = host[i];
			if (r.mgr != m || r.bucket == kStoppedBucket) continue;
			auto it = mine.find(ThreadKey(r.handle, r.vt, r.tag, r.id));
			uint32_t *th = nullptr;
			if (it != mine.end() && !it->second.empty()) {
				th = it->second.front();
				it->second.erase(it->second.begin());
				matched++;
			}
			else {
				void *owner = nullptr;
				if (r.handle != 0xffff) owner = reinterpret_cast<char **>(addr::kHandleTable)[r.handle];
				else {
					for (void *o : ownersByVt[r.vt]) {
						if (r.tag && OwnerTag(static_cast<char *>(o)) != r.tag) continue;
						bool has = false;
						const uint32_t n = *reinterpret_cast<uint32_t *>(static_cast<char *>(o) + 0x10);
						uint32_t **arr = *reinterpret_cast<uint32_t ***>(static_cast<char *>(o) + 0xc);
						for (uint32_t k = 0; arr && k < n && k < 1000; k++) has = has || (arr[k] && arr[k][4] == r.id);
						if (!has) {
							owner = o;
							break;
						}
					}
				}
				if (owner) {
					th = static_cast<uint32_t *>(reinterpret_cast<StartThread_t>(addr::kStartThread)(owner, nullptr, r.id, r.due));
					if (th) {
						th[2] = r.p2;
						th[3] = r.p3;
						created++;
					}
				}
				if (!th) {
					missing++;
					if (missing <= 10) {
						LogD("rejoin", "host thread I do not have: manager %d, owner %u (class %08x), id %u, time %u", m, r.handle, r.vt, r.id, r.due);
					}
					continue;
				}
			}
			if (th[1] != r.due) {
				reinterpret_cast<ThisPtr_t>(addr::kUnlinkThread)(mgr, nullptr, th);
				th[1] = r.due;
				reinterpret_cast<ThisPtr_t>(addr::kLinkThread)(mgr, nullptr, th);
				dueFixed++;
			}
			wanted[r.bucket].push_back(th);
		}
		size_t extra = 0, markers = 0;
		for (auto &kv : mine) {
			for (uint32_t *th : kv.second) {
				extra++;
				if (extra <= 10) {
					LogD("rejoin", "extra thread of mine (deleted): manager %d, owner %u (class %08x), id %u, time %u", m, ThreadOwnerHandle(th),
						th[5] ? *reinterpret_cast<uint32_t *>(th[5]) : 0, th[4], th[1]);
				}
				if (!th[5]) continue;
				if (th[4] == kSettlementMarkerThread && *reinterpret_cast<uint32_t *>(th[5]) == addr::kSettlementVtable) {
					if (RunThreadNow(reinterpret_cast<void *>(th[5]), th)) markers++;
				}
				reinterpret_cast<DeleteThread_t>(addr::kDeleteThread)(reinterpret_cast<void *>(th[5]), nullptr, th[4]);
			}
		}
		if (markers) LogD("rejoin", "outlines of %u settlements created", static_cast<unsigned>(markers));
		if (m == 0) {
			std::map<ThreadKey, int> hostStopped;
			for (size_t i = 0; i < count; i++) {
				const ThreadRec &r = host[i];
				if (r.mgr == 0 && r.bucket == kStoppedBucket) hostStopped[ThreadKey(r.handle, r.vt, r.tag, r.id)]++;
			}
			std::vector<std::pair<char *, uint32_t>> extraStopped;
			size_t stoppedMine = 0;
			ForEachStoppedThread(mgr, [&](char *owner, uint32_t *th) {
				stoppedMine++;
				const uint16_t handle = ThreadOwnerHandle(th);
				const uint32_t vt = *reinterpret_cast<uint32_t *>(owner);
				const ThreadKey key(handle, vt, handle == 0xffff ? OwnerTag(owner) : 0, th[4]);
				auto it = hostStopped.find(key);
				if (it != hostStopped.end() && it->second > 0) it->second--;
				else extraStopped.push_back({owner, th[4]});
			});
			for (size_t k = 0; k < extraStopped.size(); k++) {
				char *owner = extraStopped[k].first;
				if (k < 10) {
					LogD("rejoin", "extra stopped thread of mine (deleted): owner %u (class %08x), id %u", *reinterpret_cast<uint16_t *>(owner + 8),
						*reinterpret_cast<uint32_t *>(owner), extraStopped[k].second);
				}
				reinterpret_cast<DeleteThread_t>(addr::kDeleteThread)(owner, nullptr, extraStopped[k].second);
			}
			size_t missingStopped = 0;
			for (const auto &kv : hostStopped) {
				if (kv.second <= 0) continue;
				if (missingStopped < 10) {
					LogD("rejoin", "stopped host thread I do not have: owner %u (class %08x), id %u", std::get<0>(kv.first),
						std::get<1>(kv.first), std::get<3>(kv.first));
				}
				missingStopped += kv.second;
			}
			LogD("rejoin", "stopped threads: %u mine, %u extra of mine (deleted), %u of the host I do not have",
				static_cast<unsigned>(stoppedMine), static_cast<unsigned>(extraStopped.size()), static_cast<unsigned>(missingStopped));
		}
		size_t reordered = 0, badBuckets = 0;
		std::map<uint32_t, std::vector<char *>> nodes;
		ForEachThread(mgr, [&](uint32_t b, char *node, uint32_t *) { nodes[b].push_back(node); });
		for (auto &kv : wanted) {
			std::vector<char *> &list = nodes[kv.first];
			if (list.size() != kv.second.size()) {
				badBuckets++;
				continue;
			}
			bool changed = false;
			for (size_t k = 0; k < list.size(); k++) {
				uint32_t *&slot = *reinterpret_cast<uint32_t **>(list[k] + 8);
				if (slot != kv.second[k]) {
					slot = kv.second[k];
					changed = true;
				}
			}
			if (changed) reordered++;
		}
		LogD("rejoin", "threads of manager %d: %u from the host, %u mine, %u paired, %u created, %u unpaired, %u "
			"extra of mine (deleted), %u times corrected, %u buckets reordered, %u buckets with a different "
			"number",
			m, static_cast<unsigned>(std::count_if(host, host + count, [m](const ThreadRec &r) { return r.mgr == m; })),
			static_cast<unsigned>(mineCount), static_cast<unsigned>(matched), static_cast<unsigned>(created), static_cast<unsigned>(missing),
			static_cast<unsigned>(extra), static_cast<unsigned>(dueFixed), static_cast<unsigned>(reordered), static_cast<unsigned>(badBuckets));
	}
}

void FixCommandIds()
{
	size_t fixed = 0, objects = 0, mismatched = 0;
	const std::string &d = g_blobs.hostDump;
	size_t pos = 0;
	int handle = -1;
	bool inQueue = false;
	std::vector<int> ids;
	auto flush = [&]() {
		if (handle < 0 || ids.empty()) return;
		char *obj = reinterpret_cast<char **>(addr::kHandleTable)[handle & 0xffff];
		if (!obj) return;
		objects++;
		char **map = *reinterpret_cast<char ***>(obj + 0x8c);
		const uint32_t mapSize = *reinterpret_cast<uint32_t *>(obj + 0x90);
		const uint32_t off = *reinterpret_cast<uint32_t *>(obj + 0x94);
		const uint32_t size = *reinterpret_cast<uint32_t *>(obj + 0x98);
		if (!map || size != ids.size() || size > 1000) {
			mismatched++;
			return;
		}
		for (uint32_t k = 0; k < size; k++) {
			const uint32_t idx = off + k;
			uint32_t block = idx >> 2;
			if (block >= mapSize) block -= mapSize;
			char *cmd = reinterpret_cast<char **>(map[block])[idx & 3];
			if (cmd && *reinterpret_cast<int *>(cmd + 0x28) != ids[k]) {
				*reinterpret_cast<int *>(cmd + 0x28) = ids[k];
				fixed++;
			}
		}
	};
	while (pos < d.size()) {
		size_t end = d.find('\n', pos);
		if (end == std::string::npos) end = d.size();
		const std::string line = d.substr(pos, end - pos);
		pos = end + 1;
		if (!line.empty() && line[0] == '[') {
			flush();
			handle = -1;
			inQueue = false;
			ids.clear();
		}
		else if (line.compare(0, 7, "handle=") == 0) {
			handle = atoi(line.c_str() + 7);
		}
		else if (line == "command queue") {
			inQueue = true;
		}
		else if (inQueue && line.compare(0, 7, "script=") == 0) {
			const size_t at = line.find(" id=");
			if (at != std::string::npos) ids.push_back(atoi(line.c_str() + at + 4));
		}
	}
	flush();
	LogW("rejoin", "unit commands: %u objects, %u numbers corrected, %u objects with different queues",
		static_cast<unsigned>(objects), static_cast<unsigned>(fixed), static_cast<unsigned>(mismatched));
}
}
