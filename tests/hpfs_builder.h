// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

struct HpfsBuilder {
	struct Node {
		std::string name;
		bool directory = false;
		std::vector<unsigned char> data;
		std::vector<Node> children;
	};

	uint32_t pageSize = 64;
	std::vector<unsigned char> bytes;

	static void Put32(unsigned char *p, uint32_t v)
	{
		for (int i = 0; i < 4; i++) p[i] = static_cast<unsigned char>(v >> (8 * i));
	}

	uint32_t NewPage()
	{
		bytes.resize(bytes.size() + pageSize, 0);
		return static_cast<uint32_t>(bytes.size() / pageSize - 1);
	}

	uint32_t AddStream(const std::vector<unsigned char> &data)
	{
		const uint32_t perPage = pageSize / 4;
		const uint32_t rootPage = NewPage();
		std::vector<uint32_t> level;
		for (size_t at = 0; at < data.size(); at += pageSize) {
			const uint32_t page = NewPage();
			memcpy(&bytes[page * pageSize], data.data() + at, std::min<size_t>(pageSize, data.size() - at));
			level.push_back(page);
		}
		int depth = 0;
		while (level.size() > perPage - 2) {
			std::vector<uint32_t> next;
			for (size_t at = 0; at < level.size(); at += perPage) {
				const uint32_t page = NewPage();
				for (size_t k = 0; k < perPage && at + k < level.size(); k++) Put32(&bytes[page * pageSize + k * 4], level[at + k]);
				next.push_back(page);
			}
			level = next;
			depth++;
		}
		Put32(&bytes[rootPage * pageSize], static_cast<uint32_t>(data.size()));
		bytes[rootPage * pageSize + 4] = static_cast<unsigned char>(depth);
		for (size_t k = 0; k < level.size(); k++) Put32(&bytes[rootPage * pageSize + 8 + k * 4], level[k]);
		return rootPage;
	}

	uint32_t AddDirectory(const std::vector<Node> &nodes)
	{
		std::vector<unsigned char> dir;
		for (const Node &n : nodes) {
			const uint32_t handle = n.directory ? AddDirectory(n.children) : AddStream(n.data);
			unsigned char head[8] = {};
			Put32(head, handle);
			head[4] = n.directory ? 1 : 0;
			head[6] = static_cast<unsigned char>(n.name.size());
			dir.insert(dir.end(), head, head + 8);
			dir.insert(dir.end(), n.name.begin(), n.name.end());
		}
		return AddStream(dir);
	}

	std::vector<unsigned char> Build(const std::vector<Node> &root)
	{
		bytes.assign(pageSize, 0);
		memcpy(bytes.data(), "HPFS", 4);
		Put32(&bytes[4], pageSize);
		const uint32_t handle = AddDirectory(root);
		Put32(&bytes[0x1c], handle);
		return bytes;
	}

	static Node File(const std::string &name, const std::string &text)
	{
		Node n;
		n.name = name;
		n.data.assign(text.begin(), text.end());
		return n;
	}

	static Node Dir(const std::string &name, const std::vector<Node> &children)
	{
		Node n;
		n.name = name;
		n.directory = true;
		n.children = children;
		return n;
	}
};
