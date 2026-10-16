// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "ui/richtext.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace {

std::string Lower(std::string s)
{
	for (char &c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
	return s;
}

void AppendUtf8(std::string &o, unsigned cp)
{
	if (cp < 0x80) o += static_cast<char>(cp);
	else if (cp < 0x800) {
		o += static_cast<char>(0xC0 | (cp >> 6));
		o += static_cast<char>(0x80 | (cp & 0x3F));
	}
	else if (cp < 0x10000) {
		o += static_cast<char>(0xE0 | (cp >> 12));
		o += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		o += static_cast<char>(0x80 | (cp & 0x3F));
	}
	else {
		o += static_cast<char>(0xF0 | (cp >> 18));
		o += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
		o += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		o += static_cast<char>(0x80 | (cp & 0x3F));
	}
}

bool DecodeEntity(const std::string &name, std::string &out)
{
	static const struct {
		const char *n;
		unsigned cp;
	} kNamed[] = {{"amp", '&'}, {"lt", '<'}, {"gt", '>'}, {"quot", '"'}, {"apos", '\''}, {"nbsp", ' '},
		{"hellip", 0x2026}, {"ndash", 0x2013}, {"mdash", 0x2014}, {"bull", 0x2022}, {"lsquo", 0x2018},
		{"rsquo", 0x2019}, {"ldquo", 0x201C}, {"rdquo", 0x201D}, {"copy", 0xA9}, {"reg", 0xAE}, {"deg", 0xB0},
		{"times", 0xD7}, {"middot", 0xB7}, {"iexcl", 0xA1}, {"iquest", 0xBF}, {"aacute", 0xE1}, {"eacute", 0xE9},
		{"iacute", 0xED}, {"oacute", 0xF3}, {"uacute", 0xFA}, {"ntilde", 0xF1}, {"uuml", 0xFC}, {"Aacute", 0xC1},
		{"Eacute", 0xC9}, {"Iacute", 0xCD}, {"Oacute", 0xD3}, {"Uacute", 0xDA}, {"Ntilde", 0xD1}, {"Uuml", 0xDC},
		{"agrave", 0xE0}, {"egrave", 0xE8}, {"igrave", 0xEC}, {"ograve", 0xF2}, {"ugrave", 0xF9}, {"acirc", 0xE2},
		{"ecirc", 0xEA}, {"ocirc", 0xF4}, {"ccedil", 0xE7}, {"auml", 0xE4}, {"ouml", 0xF6}, {"szlig", 0xDF},
		{"laquo", 0xAB}, {"raquo", 0xBB}, {"euro", 0x20AC}, {"trade", 0x2122}};
	if (name.empty()) return false;
	if (name[0] == '#') {
		const bool hex = name.size() > 1 && (name[1] == 'x' || name[1] == 'X');
		const unsigned long cp = strtoul(name.c_str() + (hex ? 2 : 1), nullptr, hex ? 16 : 10);
		if (cp == 0 || cp > 0x10FFFF) return false;
		if (cp == 0xA0) out += ' ';
		else AppendUtf8(out, static_cast<unsigned>(cp));
		return true;
	}
	for (const auto &e : kNamed) {
		if (name == e.n) {
			AppendUtf8(out, e.cp);
			return true;
		}
	}
	return false;
}

std::string AttrValue(const std::string &tag, const char *attr)
{
	const std::string low = Lower(tag);
	size_t p = low.find(std::string(attr) + "=");
	if (p == std::string::npos) return "";
	p += strlen(attr) + 1;
	if (p >= tag.size()) return "";
	if (tag[p] == '"' || tag[p] == '\'') {
		const size_t e = tag.find(tag[p], p + 1);
		return tag.substr(p + 1, e == std::string::npos ? std::string::npos : e - p - 1);
	}
	const size_t e = tag.find_first_of(" \t\r\n>", p);
	return tag.substr(p, e == std::string::npos ? std::string::npos : e - p);
}

bool IsHttp(const std::string &u)
{
	const std::string l = Lower(u);
	return l.compare(0, 8, "https://") == 0 || l.compare(0, 7, "http://") == 0;
}

const char kBullet[] = "\xE2\x80\xA2";

}

std::vector<RichBlock> RichFromHtml(const std::string &html)
{
	std::vector<RichBlock> out;
	std::string cur, link, marker;
	int kind = 0, baseKind = 0, boldDepth = 0, boldChars = 0, textChars = 0;
	std::vector<int> counters;
	bool pre = false, skip = false, lastSpace = true;

	auto flush = [&]() {
		while (!cur.empty() && (cur.back() == ' ' || cur.back() == '\n')) cur.pop_back();
		size_t s = 0;
		while (s < cur.size() && (cur[s] == ' ' || cur[s] == '\n')) s++;
		cur.erase(0, s);
		if (!cur.empty()) {
			RichBlock b;
			b.kind = kind;
			b.text = cur;
			b.link = link;
			b.marker = marker;
			b.indent = counters.empty() ? 0 : static_cast<int>(counters.size()) - 1;
			b.bold = textChars > 0 && boldChars >= textChars;
			out.push_back(b);
			marker.clear();
		}
		cur.clear();
		link.clear();
		boldChars = textChars = 0;
		lastSpace = true;
	};
	auto addChar = [&](char c) {
		const bool space = c == ' ' || c == '\t' || c == '\r' || c == '\n';
		if (pre) {
			if (c != '\r') cur += c;
			return;
		}
		if (space) {
			if (!lastSpace) cur += ' ';
			lastSpace = true;
			return;
		}
		cur += c;
		lastSpace = false;
		textChars++;
		if (boldDepth > 0) boldChars++;
	};

	const size_t n = html.size();
	size_t i = 0;
	while (i < n) {
		const char c = html[i];
		if (c == '<') {
			if (html.compare(i, 4, "<!--") == 0) {
				const size_t e = html.find("-->", i);
				i = e == std::string::npos ? n : e + 3;
				continue;
			}
			const size_t end = html.find('>', i);
			if (end == std::string::npos) break;
			std::string tag = html.substr(i + 1, end - i - 1);
			i = end + 1;
			bool closing = false;
			if (!tag.empty() && tag[0] == '/') {
				closing = true;
				tag.erase(0, 1);
			}
			size_t ne = 0;
			while (ne < tag.size() && !isspace(static_cast<unsigned char>(tag[ne])) && tag[ne] != '/') ne++;
			const std::string name = Lower(tag.substr(0, ne));
			if (skip) {
				if (closing && (name == "script" || name == "style")) skip = false;
				continue;
			}
			if (name == "script" || name == "style") {
				if (!closing) skip = true;
				continue;
			}
			if (name == "br") {
				if (!pre && !cur.empty() && cur.back() != '\n') cur += '\n';
				lastSpace = true;
			}
			else if (name == "p" || name == "div" || name == "section" || name == "article" || name == "blockquote" ||
				name == "table" || name == "tr" || name == "td" || name == "th" || name == "thead" || name == "tbody") {
				flush();
			}
			else if (name.size() == 2 && name[0] == 'h' && name[1] >= '1' && name[1] <= '6') {
				flush();
				kind = closing ? baseKind : name[1] == '1' ? 1 : 2;
			}
			else if (name == "ul" || name == "ol") {
				flush();
				if (!closing) counters.push_back(name == "ol" ? 1 : 0);
				else if (!counters.empty()) counters.pop_back();
				baseKind = counters.empty() ? 0 : 3;
				kind = baseKind;
			}
			else if (name == "li") {
				flush();
				if (!closing) {
					baseKind = kind = 3;
					if (counters.empty() || counters.back() == 0) marker = kBullet;
					else marker = std::to_string(counters.back()++) + ".";
				}
				else {
					kind = baseKind = counters.empty() ? 0 : 3;
				}
			}
			else if (name == "hr") {
				flush();
				RichBlock b;
				b.kind = 4;
				out.push_back(b);
			}
			else if (name == "strong" || name == "b") {
				boldDepth = closing ? std::max(0, boldDepth - 1) : boldDepth + 1;
			}
			else if (name == "a") {
				if (!closing && link.empty()) {
					const std::string href = AttrValue(tag, "href");
					if (IsHttp(href)) link = href;
				}
			}
			else if (name == "pre") {
				flush();
				pre = !closing;
			}
			continue;
		}
		if (skip) {
			i++;
			continue;
		}
		if (c == '&') {
			const size_t semi = html.find(';', i);
			if (semi != std::string::npos && semi - i <= 10) {
				std::string decoded;
				if (DecodeEntity(html.substr(i + 1, semi - i - 1), decoded)) {
					for (char d : decoded) addChar(d);
					i = semi + 1;
					continue;
				}
			}
		}
		addChar(c);
		i++;
	}
	flush();
	return out;
}

std::vector<RichBlock> RichFromPlain(const std::string &text)
{
	std::vector<RichBlock> out;
	size_t pos = 0;
	bool blank = false;
	while (pos <= text.size()) {
		size_t e = text.find('\n', pos);
		if (e == std::string::npos) e = text.size();
		std::string line = text.substr(pos, e - pos);
		pos = e + 1;
		while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
		const size_t s = line.find_first_not_of(" \t");
		if (s == std::string::npos) {
			if (!out.empty() && !blank) {
				RichBlock b;
				b.kind = 5;
				out.push_back(b);
			}
			blank = true;
			continue;
		}
		blank = false;
		line.erase(0, s);
		RichBlock b;
		if (line.size() > 1 && (line[0] == '*' || line[0] == '-') && line[1] == ' ') {
			b.kind = 3;
			b.marker = kBullet;
			line.erase(0, 2);
		}
		else if (line.compare(0, 3, kBullet) == 0) {
			b.kind = 3;
			b.marker = kBullet;
			line.erase(0, 3);
			line.erase(0, std::min(line.find_first_not_of(' '), line.size()));
		}
		b.text = line;
		out.push_back(b);
	}
	while (!out.empty() && out.back().kind == 5) out.pop_back();
	return out;
}
