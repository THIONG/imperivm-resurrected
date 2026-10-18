// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>

#include "core/versioncmp.h"
#include "ui/modui_internal.h"

using namespace modui_impl;

namespace modui_impl {
int DropCurrent(const State &st)
{
	if (st.drop == kDropProfile) {
		for (size_t i = 0; i < st.profileNames.size(); i++) {
			if (st.profileNames[i] == st.profile.name) return static_cast<int>(i);
		}
		return -1;
	}
	if (st.drop == kDropFilter) return st.filter;
	if (st.drop == kDropIoSort) return st.ioSort;
	return -1;
}
}

namespace modui_impl {
namespace {
void Combo(Painter &p, const State &st, int x, int y, int w, int h, const std::string &text, int kind)
{
	PaintCombo(p, x, y, w, h, text, st.drop == kind, true, st.mouseX, st.mouseY, kDropOpen, kind, &g_drop.anchor);
}

void PaintDropList(Painter &p, const State &st)
{
	if (st.drop == kDropNone) return;
	const std::vector<std::string> texts = DropItems(st);
	if (texts.empty()) return;
	std::vector<ui::DropItem> items = ToDropItems(texts);
	const int n = static_cast<int>(items.size());
	for (int i = 0; i < n; i++) {
		if (st.drop == kDropMenu && i == 2) items[i].color = kBad;
		if (st.drop == kDropProfile && (i == 1 || i + 1 == n)) items[i].color = kGold;
	}
	if (st.drop == kDropClashFiles) {
		if (const ModIssues *is = SelectedIssues(st)) {
			const int files = static_cast<int>(is->files.size());
			for (int i = 0; i < n && i < files + static_cast<int>(is->values.size()); i++) {
				const int kind = i < files ? is->files[i].kind : is->values[i - files].kind;
				items[i].color = kind == kModRules ? kGold : kind == kModContent ? RGB(140, 196, 176) : kText;
			}
		}
	}
	const ui::DropLayout d = PaintDropMenu(p, g_drop.anchor, items, DropCurrent(st), st.dropScroll, kDesignW, kDesignH, kDropPick, kDropClose,
		st.mouseX, st.mouseY, st.drop == kDropClashFiles ? kDesignW - kDetailX - 16 : st.drop == kDropVersions ? 360 : 180);
	g_drop.rows = d.rows;
	g_drop.count = d.count;
}

int IoInstalledState(const State &st, const GbMod &m)
{
	int state = 0;
	for (const ModInfo &mi : st.mods) {
		if (m.id && mi.gbId == m.id) {
			if (!m.details || VersionIsCurrent(mi.version, m.entry.version)) return 1;
			state = 2;
		}
	}
	return state;
}

std::string Thousands(uint64_t n)
{
	std::string s = std::to_string(n);
	for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3) s.insert(i, ".");
	return s;
}

std::string UpperText(const std::string &s)
{
	std::wstring w = ui::W(s);
	if (w.empty()) return s;
	CharUpperBuffW(&w[0], static_cast<DWORD>(w.size()));
	const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
	std::string out(n > 0 ? n : 0, '\0');
	if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), &out[0], n, nullptr, nullptr);
	return out;
}

void PaintIoImage(Painter &p, int x, int y, int w, int h, const std::string &url)
{
	const std::shared_ptr<const GbImage> img = GbLogo(url);
	if (img) {
		p.Image(x, y, w, h, img->bgra.data(), img->width, img->height);
	}
	else {
		p.Fill(x, y, w, h, RGB(22, 17, 12));
		p.Font(3);
		p.Text(x, y, w, h, url.empty() ? "No image" : "Loading...", kDim, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
}

void PaintRich(Painter &p, const State &st, int x, int y, int w, int h, const std::string &html, const std::string &plain,
	const std::string &noteTitle, const std::string &note, int baseFont)
{
	std::lock_guard<std::mutex> lock(g_rich.mx);
	const std::string src = (html.empty() ? "P" + plain : "H" + html) + '\x01' + noteTitle + '\x01' + note;
	if (src != g_rich.src) {
		g_rich.src = src;
		g_rich.list.scroll = 0;
		g_rich.blocks = html.empty() ? RichFromPlain(plain) : RichFromHtml(html);
		if (!note.empty()) {
			RichBlock gap;
			gap.kind = 5;
			if (!g_rich.blocks.empty()) g_rich.blocks.push_back(gap);
			RichBlock title;
			title.kind = 2;
			title.text = noteTitle;
			g_rich.blocks.push_back(title);
			const std::vector<RichBlock> nb = RichFromPlain(note);
			g_rich.blocks.insert(g_rich.blocks.end(), nb.begin(), nb.end());
		}
	}
	g_rich.view = h;
	g_rich.rect = p.R(x, y, w, h);
	OffsetRect(&g_rich.rect, p.ox, p.oy);
	g_rich.rectValid = true;
	g_rich.rectTick = GetTickCount();
	if (g_rich.blocks.empty()) {
		p.Font(baseFont);
		p.Text(x, y, w, 20, "No description.", kDim);
		g_rich.geom = ListGeom();
		return;
	}
	const int barW = 6, tw = w - barW - 8;
	struct Pos {
		int y, h, indent;
	};
	std::vector<Pos> pos;
	int total = 0;
	for (const RichBlock &b : g_rich.blocks) {
		const int indent = b.kind == 3 ? 16 + b.indent * 14 : 0;
		const int font = b.kind == 1 ? 2 : b.kind == 2 || b.bold ? 1 : baseFont;
		int bh;
		if (b.kind == 4) bh = 9;
		else if (b.kind == 5) bh = 6;
		else {
			p.Font(font);
			bh = p.Measure(tw - indent, b.text);
		}
		const int before = (b.kind == 1 || b.kind == 2) && total > 0 ? 8 : 0;
		const int after = b.kind == 1 || b.kind == 2 ? 3 : b.kind == 3 ? 2 : 6;
		pos.push_back({total + before, bh, indent});
		total += before + bh + after;
	}
	const int maxScroll = std::max(0, total - h);
	g_rich.list.scroll = std::max(0, std::min(g_rich.list.scroll, maxScroll));
	const int scroll = g_rich.list.scroll;
	p.Clip(x, y, w, h);
	for (size_t i = 0; i < g_rich.blocks.size(); i++) {
		const RichBlock &b = g_rich.blocks[i];
		const Pos &ps = pos[i];
		if (ps.y + ps.h < scroll || ps.y > scroll + h) continue;
		const int by = y + ps.y - scroll;
		if (b.kind == 4) {
			p.Fill(x, by + 4, tw, 1, pal::line);
			continue;
		}
		if (b.kind == 5) continue;
		const int font = b.kind == 1 ? 2 : b.kind == 2 || b.bold ? 1 : baseFont;
		const int top = std::max(by, y), bottom = std::min(by + ps.h, y + h);
		const bool hov = !b.link.empty() && bottom > top && p.Hover(x + ps.indent, top, tw - ps.indent, bottom - top, st.mouseX, st.mouseY);
		const COLORREF col = !b.link.empty() ? (hov ? RGB(180, 214, 255) : kLink) : b.kind == 1 || b.kind == 2 ? kGold : kText;
		p.Font(font);
		if (b.kind == 3) p.Text(x + ps.indent - 14, by, 14, ps.h, b.marker, kGold, DT_LEFT | DT_TOP | DT_SINGLELINE);
		p.TextWrapped(x + ps.indent, by, tw - ps.indent, ps.h + 2, b.text, col);
		if (!b.link.empty() && bottom > top) p.AddHit(x + ps.indent, top, tw - ps.indent, bottom - top, kRichLink, static_cast<int>(i));
	}
	p.Unclip();
	{
		ListGeom geom;
		ScrollbarPaint(p, x + w - barW, y, h, total, h, g_rich.list, geom, st.mouseX, st.mouseY);
		g_rich.geom = geom;
	}
}

RECT PaintIoImageFit(Painter &p, int x, int y, int w, int h, const std::string &url, bool big = false)
{
	const std::shared_ptr<const GbImage> img = big ? GbBigImage(url) : GbLogo(url);
	if (!img || img->width <= 0 || img->height <= 0) return {0, 0, 0, 0};
	int dw = w, dh = w * img->height / img->width;
	if (dh > h) {
		dh = h;
		dw = h * img->width / img->height;
	}
	const int dx = x + (w - dw) / 2, dy = y + (h - dh) / 2;
	p.Image(dx, dy, dw, dh, img->bgra.data(), img->width, img->height);
	return {dx, dy, dx + dw, dy + dh};
}

void PaintIoDetail(Painter &p, const State &st, const GbMod &m, int index)
{
	const int mx = st.mouseX, my = st.mouseY;
	const int x = 16, y = kContentY, w = kDesignW - 32, h = kFootY - 8 - kContentY;
	p.Fill(x, y, w, h, pal::wellRaised);
	p.Frame(x, y, w, h, pal::edge);
	const int iw = 176, ih = 99;
	PaintIoImage(p, x + 10, y + 10, iw, ih, m.logoUrl);
	p.Frame(x + 10, y + 10, iw, ih, pal::line);
	const int dx = x + iw + 24, dw = w - iw - 34;
	int ty = y + 8;
	p.Font(2);
	p.Text(dx, ty, dw, 30, m.name, kGold, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
	ty += 32;
	p.Font(0);
	p.Text(dx, ty, dw, 20, Tr("Author: ") + (m.author.empty() ? Tr("unknown") : m.author), kText);
	ty += 22;
	p.Font(3);
	p.Text(dx, ty, dw, 18, (m.entry.version.empty() ? Tr("No version") : Tr("Version ") + m.entry.version) +
		(m.entry.bytes ? "  ·  " + Tr("download") + " " + HumanBytes(m.entry.bytes) : ""), kDim);
	ty += 18;
	p.Text(dx, ty, dw, 18, Thousands(m.downloads) + " " + Tr("downloads") + "  ·  " + Thousands(static_cast<uint64_t>(m.likes)) +
		" " + Tr("likes") + "  ·  " + Thousands(m.views) + " " + Tr("views"), kDim);
	ty += 18;
	const std::string tags = IoTagsText(m);
	if (!tags.empty()) {
		ty += 2 + p.TextWrapped(dx, ty + 2, dw, 36, tags, kGold);
	}
	if (m.dependencies) {
		ty += 4;
		p.Text(dx, ty, dw, 18, "Needs other content: it is installed with it", kGold);
		ty += 18;
	}
	int descY = std::max(y + 10 + ih, ty) + 8;
	p.Fill(x + 10, descY - 4, w - 20, 1, pal::lineDim);
	if (m.shots.size() > 1) {
		const int tw = 124, th = 70, tgap = 8;
		const int perRow = std::max(1, (w - 20 + tgap) / (tw + tgap));
		for (size_t i = 0; i < m.shots.size() && static_cast<int>(i) < perRow; i++) {
			const int sx = x + 10 + static_cast<int>(i) * (tw + tgap);
			const bool hov = p.Hover(sx, descY, tw, th, mx, my);
			PaintIoImage(p, sx, descY, tw, th, m.shots[i].thumb);
			p.Frame(sx, descY, tw, th, hov ? pal::edgeHot : pal::line);
			p.AddHit(sx, descY, tw, th, kIoShotOpen, static_cast<int>(i));
		}
		descY += th + 10;
		p.Fill(x + 10, descY - 4, w - 20, 1, pal::lineDim);
	}
	std::string notes, noteVersion;
	if (m.id) GbChangelog(m.id, "", notes, noteVersion);
	PaintRich(p, st, x + 10, descY, w - 20, y + h - 46 - descY, m.descriptionHtml, !m.description.empty() ? m.description : m.summary,
		Tr("What's new in version ") + noteVersion, notes, 0);
	const int state = IoInstalledState(st, m);
	const std::string label = !m.hasFile ? "No file" : state == 1 ? "Already installed" : state == 2 ? Tr("Update to ") + m.entry.version : "Install";
	p.Button(x + w - 520, y + h - 40, 180, 30, label, kIoInstall, index, mx, my, m.hasFile && state != 1 && !st.dl.active, true);
	Combo(p, st, x + w - 520 - 176, y + h - 40, 170, 30, "Other versions", kDropVersions);
	p.Button(x + w - 334, y + h - 40, 170, 30, "View on GameBanana", kIoPage, index, mx, my, !m.page.empty());
	p.Button(x + w - 158, y + h - 40, 148, 30, "Back", kIoBack, 0, mx, my);
}

void PaintShotViewer(Painter &p, const State &st)
{
	const int mx = st.mouseX, my = st.mouseY;
	if (st.tab != 2 || st.ioDetail < 0 || st.ioDetail >= static_cast<int>(st.io.mods.size())) return;
	const GbMod &m = st.io.mods[st.ioDetail];
	if (st.ioShot >= 0 && st.ioShot < static_cast<int>(m.shots.size())) {
		const int n = static_cast<int>(m.shots.size());
		p.hits->clear();
		p.AddHit(0, 0, kDesignW, kDesignH, kIoShotClose, 0);
		p.Fill(0, 0, kDesignW, kDesignH, RGB(14, 11, 8));
		p.Frame(0, 0, kDesignW, kDesignH, kGold);
		p.Frame(3, 3, kDesignW - 6, kDesignH - 6, pal::line);
		p.Font(2);
		p.Text(24, 10, kDesignW - 240, 34, m.name, kGold, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		p.Font(0);
		p.Text(kDesignW - 210, 14, 130, 26, std::to_string(st.ioShot + 1) + " / " + std::to_string(n), kDim, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
		p.Button(kDesignW - 64, 12, 40, 30, "X", kIoShotClose, 0, mx, my);
		p.Fill(16, 52, kDesignW - 32, 1, pal::line);
		const int ix = 70, iy = 64, iw = kDesignW - 140, ih = 404;
		const GbShot &shot = m.shots[st.ioShot];
		RECT r = PaintIoImageFit(p, ix, iy, iw, ih, shot.big, true);
		const bool loaded = r.right > r.left;
		if (!loaded) {
			p.Fill(ix, iy, iw, ih, RGB(10, 8, 6));
			r = PaintIoImageFit(p, ix, iy, iw, ih, shot.thumb);
		}
		if (r.right > r.left) {
			p.Frame(r.left - 1, r.top - 1, r.right - r.left + 2, r.bottom - r.top + 2, pal::edge);
			p.AddHit(r.left, r.top, r.right - r.left, r.bottom - r.top, n > 1 ? kIoShotStep : kNone, 1);
		}
		if (!loaded) {
			p.Font(3);
			p.Text(ix, iy + ih / 2 - 10, iw, 20, "Loading...", RGB(230, 210, 170), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
		if (n > 1) {
			p.Button(14, iy + ih / 2 - 36, 44, 72, "<", kIoShotStep, -1, mx, my);
			p.Button(kDesignW - 58, iy + ih / 2 - 36, 44, 72, ">", kIoShotStep, 1, mx, my);
		}
		const int tw = 120, th = 68, tgap = 8;
		const int shown = std::min(n, std::max(1, (kDesignW - 60 + tgap) / (tw + tgap)));
		const int stripW = shown * tw + (shown - 1) * tgap;
		const int tx0 = (kDesignW - stripW) / 2, ty = iy + ih + 18;
		for (int i = 0; i < shown; i++) {
			const int sx = tx0 + i * (tw + tgap);
			const bool cur = i == st.ioShot, hov = p.Hover(sx, ty, tw, th, mx, my);
			PaintIoImage(p, sx, ty, tw, th, m.shots[i].thumb);
			p.Frame(sx, ty, tw, th, cur ? RGB(255, 226, 150) : hov ? pal::edgeHot : pal::divider);
			if (cur) p.Frame(sx + 1, ty + 1, tw - 2, th - 2, RGB(255, 226, 150));
			p.AddHit(sx, ty, tw, th, kIoShotOpen, i);
		}
		p.Font(3);
		p.Text(0, kDesignH - 36, kDesignW, 20, "Esc closes the image  ·  the arrow keys change screenshot", kDim, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}
}
}
}

namespace modui_impl {
void PaintFilterPanel(Painter &p, const State &st)
{
	if (!st.ioFilterOpen || st.tab != 2) return;
	const int mx = st.mouseX, my = st.mouseY;
	struct Chip {
		int arg, x, y, w;
		std::string text;
		bool on;
		bool enabled;
	};
	struct Line {
		const char *label;
		int y;
		const char *empty;
	};
	const int x = 16, y = kContentY + 32, w = kDesignW - 32, labelW = 200, chipH = 28, rowStep = 34;
	const CatalogFilter &f = st.ioFilter;
	const CatalogCounts &counts = st.io.counts;
	std::vector<Chip> chips;
	std::vector<Line> lines;
	int cy = y + 16;
	p.Font(0);
	const auto addLine = [&](const char *label, int group, uint32_t mask, const std::vector<std::pair<int, std::string>> &items, const char *empty,
							 uint32_t disabled = 0) {
		lines.push_back({label, cy, items.empty() ? empty : nullptr});
		int cx = x + labelW;
		for (const auto &it : items) {
			const int cw = std::max(34, p.TextWidth(it.second) + 28);
			if (cx + cw > x + w - 16) {
				cx = x + labelW;
				cy += rowStep;
			}
			chips.push_back({group * kIoFilterGroupStride + it.first, cx, cy, cw, it.second, ((mask >> it.first) & 1) != 0, !((disabled >> it.first) & 1)});
			cx += cw + 8;
		}
		cy += rowStep + 8;
	};
	static const char *const kGroupNames[] = {"Gameplay", "Maps and campaigns", "Cosmetic"};
	bool players = false, language = false;
	for (int g = 0; g < 3; g++) {
		std::vector<std::pair<int, std::string>> items;
		uint32_t empty = 0;
		for (int c = g * 3; c < g * 3 + 3; c++) {
			const bool on = (f.types >> c) & 1;
			if (!counts.types[c] && !on) empty |= 1u << c;
			items.push_back({c, Tr(CatalogCategoryLabel(c)) + " (" + std::to_string(counts.types[c]) + ")"});
			players = players || (on && CatalogHasPlayers(c));
			language = language || (on && CatalogHasLanguage(c));
		}
		addLine(kGroupNames[g], kIoFilterTypes, f.types, items, nullptr, empty);
	}
	if (players) {
		std::vector<std::pair<int, std::string>> items;
		for (int n = kCatalogMinPlayers; n <= kCatalogMaxPlayers; n++) {
			if (counts.players[n] || ((f.players >> n) & 1)) items.push_back({n, std::to_string(n)});
		}
		addLine("Players", kIoFilterPlayers, f.players, items, "No map says how many players it is for yet");
	}
	if (language) {
		std::vector<std::pair<int, std::string>> items;
		for (int l = 0; l < kLangCount; l++) {
			if (CatalogLanguageCount(counts, f.types, l) || ((f.languages >> l) & 1)) items.push_back({l, Tr(CatalogLanguageLabel(l))});
		}
		addLine("Language", kIoFilterLanguages, f.languages, items, "None of them says its language yet");
	}
	const int h = cy - y + 46;
	p.hits->clear();
	p.AddHit(0, 0, kDesignW, kDesignH, kIoFilterClose, 0);
	p.Fill(x + 4, y + 4, w, h, pal::shadow);
	p.Fill(x, y, w, h, pal::dialog);
	p.Frame(x, y, w, h, pal::edgeStrong);
	p.AddHit(x, y, w, h, kNone, 0);
	for (const Line &l : lines) {
		p.Font(1);
		p.Text(x + 16, l.y, labelW - 24, chipH, l.label, kGold, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		if (l.empty) {
			p.Font(3);
			p.Text(x + labelW, l.y, w - labelW - 16, chipH, l.empty, kDim);
		}
	}
	for (const Chip &c : chips) {
		const bool hov = c.enabled && p.Hover(c.x, c.y, c.w, chipH, mx, my);
		p.Fill(c.x, c.y, c.w, chipH, c.on ? (hov ? RGB(150, 104, 40) : RGB(122, 82, 30)) : (hov ? pal::fieldHover : pal::field));
		p.Frame(c.x, c.y, c.w, chipH, c.on ? pal::focus : hov ? pal::edgeHot : c.enabled ? pal::edge : pal::divider);
		p.Font(c.on ? 1 : 0);
		p.Text(c.x, c.y, c.w, chipH, c.text, c.on ? RGB(255, 244, 210) : c.enabled ? kText : kDim, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		if (c.enabled) p.AddHit(c.x, c.y, c.w, chipH, kIoFilterToggle, c.arg);
	}
	const int n = CatalogFilterSize(f);
	p.Button(x + w - 296, y + h - 40, 140, 30, "Clear filters", kIoFilterClear, 0, mx, my, n > 0);
	p.Button(x + w - 148, y + h - 40, 132, 30, "Close", kIoFilterClose, 0, mx, my, true, true);
}
}

namespace modui_impl {
namespace {
void PaintExplore(Painter &p, const State &st)
{
	const int mx = st.mouseX, my = st.mouseY;
	const GbResults &io = st.io;
	if (st.ioDetail >= 0 && st.ioDetail < static_cast<int>(io.mods.size())) {
		PaintIoDetail(p, st, io.mods[st.ioDetail], st.ioDetail);
		return;
	}
	const int nFilters = CatalogFilterSize(st.ioFilter);
	PaintField(p, st, 16, kContentY, 300, 26, st.ioSearch, "Search...", st.focus == kIoSearchField, false, kIoFocusSearch, mx, my);
	p.Button(322, kContentY, 190, 26, nFilters ? Tr("Filters (") + std::to_string(nFilters) + ")" : "Filters", kIoFilterOpen, 0, mx, my, true, nFilters > 0, 0);
	Combo(p, st, 518, kContentY, 170, 26, kIoSortNames[st.ioSort], kDropIoSort);
	const int pages = std::max(1, (io.total + kIoPerPage - 1) / kIoPerPage);
	p.Button(kDesignW - 16 - 24, kContentY, 24, 26, ">", kIoNext, 0, mx, my, st.ioPage + 1 < pages && !io.loading, false, 0);
	p.Font(3);
	p.Text(kDesignW - 16 - 24 - 46, kContentY, 44, 26, std::to_string(st.ioPage + 1) + "/" + std::to_string(pages), kDim, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	p.Button(kDesignW - 16 - 24 - 46 - 28, kContentY, 24, 26, "<", kIoPrev, 0, mx, my, st.ioPage > 0 && !io.loading, false, 0);

	const int gx = 16, gy = kContentY + 34, cw = 237, ch = 222, gap = 4;
	if (io.mods.empty()) {
		p.Fill(gx, gy, kDesignW - 32, kFootY - 8 - gy, pal::well);
		p.Frame(gx, gy, kDesignW - 32, kFootY - 8 - gy, pal::line);
		p.Font(0);
		std::string text = io.loading || !st.ioRequested ? "Searching GameBanana..."
			: !io.error.empty()								? Tr("Could not connect to GameBanana: ") + Tr(io.error)
			: !st.ioSearch.empty() || nFilters				? "No content matches the search."
															: "There is no content published for Imperivm on GameBanana yet. Be the first to upload some!";
		p.TextWrapped(gx + 20, gy + 20, kDesignW - 72, 60, text, io.error.empty() ? kDim : kBad);
		if (!io.error.empty() && !io.loading) p.Button(gx + 20, gy + 80, 160, 28, "Retry", kIoRetry, 0, mx, my);
		return;
	}
	for (size_t i = 0; i < io.mods.size() && i < static_cast<size_t>(kIoPerPage); i++) {
		const GbMod &m = io.mods[i];
		const int x = gx + static_cast<int>(i % 4) * (cw + gap), y = gy + static_cast<int>(i / 4) * (ch + gap);
		const bool hov = p.Hover(x, y, cw, ch, mx, my);
		p.Fill(x, y, cw, ch, hov ? pal::fieldOpen : RGB(26, 21, 15));
		const int imgH = 152;
		PaintIoImage(p, x + 1, y + 1, cw - 2, imgH, m.logoUrl);
		p.Frame(x, y, cw, ch, hov ? pal::edgeHot : pal::line);
		const std::string catText = m.facts.category >= 0 ? Tr(CatalogCategoryLabel(m.facts.category)) : m.category;
		if (!catText.empty()) {
			p.Font(4);
			const std::string cat = UpperText(catText);
			const int chipW = std::min(cw - 16, p.TextWidth(cat) + 16), chipH = 22, chipX = x + 8, chipY = y + 1 + imgH - chipH - 8;
			p.Fill(chipX, chipY, chipW, chipH, RGB(22, 17, 12));
			p.Text(chipX + 8, chipY, chipW - 12, chipH, cat, kText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		}
		const int state = IoInstalledState(st, m);
		if (state == 1 || state == 2) {
			p.Font(4);
			const std::string badge = UpperText(Tr(state == 1 ? "Already installed" : "Update available"));
			const int badgeW = p.TextWidth(badge) + 16;
			p.Fill(x + cw - 8 - badgeW, y + 9, badgeW, 20, RGB(22, 17, 12));
			p.Text(x + cw - 8 - badgeW, y + 9, badgeW, 20, badge, state == 1 ? kOk : kGold, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
		p.Font(1);
		p.Text(x + 10, y + imgH + 8, cw - 20, 22, UpperText(m.name), kText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		p.Font(3);
		p.Text(x + 10, y + imgH + 31, cw - 20, 16, Tr("By") + " " + m.author, kDim, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		p.Text(x + 10, y + imgH + 50, cw - 20, 16, Thousands(m.downloads) + " " + Tr(m.downloads == 1 ? "download" : "downloads"), kDim,
			DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		p.AddHit(x, y, cw, ch, kIoOpen, static_cast<int>(i));
	}
	if (io.loading) {
		p.Font(3);
		p.Text(gx, gy + 2 * (ch + gap) - 4, kDesignW - 32, 16, "Updating...", kDim, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
	}
}
}
}

namespace modui_impl {
void PaintPanel(Painter &p, const State &st)
{
	const int mx = st.mouseX, my = st.mouseY;
	PaintWindowFrame(p, kDesignW, kDesignH);
	PaintTitle(p, 20, 12, 130, "Content");
	p.Font(0);
	p.Text(152, 16, 50, 28, "Profile", kDim);
	if (st.nameMode != kNameNone) {
		const char *what = st.nameMode == kNameRename ? "New name" : "Profile name";
		PaintField(p, st, 200, 16, 266, 28, st.nameText, what, st.focus == kNameField, false, kFocusName, mx, my);
		p.Button(472, 16, 90, 28, st.nameMode == kNameRename ? "Rename" : "Save", kNameOk, 0, mx, my, true, true);
		p.Button(566, 16, 86, 28, "Cancel", kNameCancel, 0, mx, my);
	}
	else {
		Combo(p, st, 200, 16, 236, 28, ModsProfileLabel(st.profile.name), kDropProfile);
		if (st.profile.name == kNoProfileName) {
			p.Button(442, 16, 170, 28, "Save as profile", kSaveAs, 0, mx, my, !st.profile.mods.empty());
		}
		else if (st.profile.name != kOriginalName) {
			p.Button(442, 16, 36, 28, "...", kDropOpen, kDropMenu, mx, my);
			if (st.drop == kDropMenu) g_drop.anchor = {442, 16, 478, 44};
		}
		if (st.restartNeeded) {
			p.Font(3);
			p.Text(kDesignW - 16 - 204, 16, 204, 28, "Changes: on restart", kGold, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
		}
	}
	PaintRule(p, 54, kDesignW);
	PaintTabBar(p, kDesignW,
		{{Tr("Installed") + " (" + std::to_string(st.mods.size()) + ")", kTabInstalled, 0, st.tab == 0, true},
			{"Browse content", kTabExplore, 0, st.tab == 2, true}},
		mx, my);
	{
		const std::vector<std::string> &problems = st.problems;
		if (!problems.empty()) {
			const std::string label = std::to_string(problems.size()) + " " + Tr(problems.size() == 1 ? "problem in the profile" : "problems in the profile");
			p.Font(3);
			const int bw = p.TextWidth(label) + 46;
			const bool hov = p.Hover(kDesignW - 16 - bw, 60, bw, 26, st.mouseX, st.mouseY);
			p.Fill(kDesignW - 16 - bw, 60, bw, 26, hov ? RGB(86, 36, 26) : RGB(60, 26, 20));
			p.Frame(kDesignW - 16 - bw, 60, bw, 26, hov ? RGB(255, 150, 130) : kBad);
			PaintAlertIcon(p, kDesignW - 16 - bw + 8, 65, true);
			p.Font(3);
			p.Text(kDesignW - 16 - bw + 30, 60, bw - 38, 26, label, kBad, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
			p.AddHit(kDesignW - 16 - bw, 60, bw, 26, kShowProblems, 0);
		}
	}
	if (st.tab == 2) {
		PaintExplore(p, st);
	}
	else {
		PaintInstalled(p, st);
	}
	PaintFooter(p, st);
	PaintFilterPanel(p, st);
	PaintShotViewer(p, st);
	PaintDropList(p, st);
	PaintConfirm(p, st);
}

void PaintInstalled(Painter &p, const State &st)
{
	const int mx = st.mouseX, my = st.mouseY;
	PaintField(p, st, kListX, kContentY, 300, 24, st.searchText, "Search your content...", st.focus == kSearchField, false, kFocusSearch, mx, my);
	Combo(p, st, kListX + 306, kContentY, kListW - 306, 24, kFilterNames[st.filter], kDropFilter);

	p.Fill(kListX, kListY, kListW, kInstalledListH, pal::well);
	p.Frame(kListX, kListY, kListW, kInstalledListH, pal::line);
	const std::vector<int> view = VisibleRows(st);
	if (st.rows.empty()) {
		p.Font(0);
		p.TextWrapped(kListX + 12, kListY + 12, kListW - 24, 120,
			"No content installed. Find it in the Browse content tab, install a .zip with \"Install file\", or "
			"copy each pack into its own folder inside \"mods\" (in the game folder) and click \"Rescan\".",
			kDim);
	}
	else if (view.empty()) {
		p.Font(0);
		p.TextWrapped(kListX + 12, kListY + 12, kListW - 24, 60, "No content matches the search or filter.", kDim);
	}
	const int visible = kInstalledListH / kRowH;
	for (int v = st.list.scroll; v < static_cast<int>(view.size()) && v < st.list.scroll + visible; v++) {
		const int i = view[v];
		const Row &r = st.rows[i];
		const int y = kListY + (v - st.list.scroll) * kRowH;
		const ModInfo *m = FindModIn(st.mods, r.folder);
		const bool pending = Pending(st, r.folder);
		const bool sel = i == st.selected;
		if (sel) p.Fill(kListX + 1, y, kListW - 2, kRowH, pal::lineDim);
		else if (p.Hover(kListX, y, kListW, kRowH, mx, my)) p.Fill(kListX + 1, y, kListW - 2, kRowH, RGB(44, 34, 24));
		p.AddHit(kListX + 40, y, kListW - 72, kRowH, kSelect, i);
		if (!pending) {
			const bool boxHov = p.Hover(kListX + 6, y, 34, kRowH, mx, my);
			p.Fill(kListX + 14, y + 6, 16, 16, RGB(10, 8, 6));
			p.Frame(kListX + 14, y + 6, 16, 16, boxHov ? pal::focus : kGold);
			if (r.active) p.Fill(kListX + 18, y + 10, 8, 8, kGold);
			p.AddHit(kListX + 6, y, 34, kRowH, kToggle, i);
		}
		std::string name = !m ? r.folder + Tr(" (missing)")
			: m->version.empty() || m->name.find(m->version) != std::string::npos ? m->name : m->name + " " + m->version;
		if (m && name.find(m->version) == std::string::npos) {
			for (const ModInfo &o : st.mods) {
				if (&o != m && _stricmp(o.id.c_str(), m->id.c_str()) == 0) name += " " + m->version;
			}
		}
		if (pending) name += " (uninstalled on restart)";
		p.Font(r.active ? 1 : 0);
		p.Text(kListX + 40, y, 300, kRowH, name, pending || !m ? kBad : r.active ? kText : kDim,
			DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		p.Font(3);
		if (m && !pending) {
			const auto issues = st.issues->find(r.folder);
			const bool bad = issues != st.issues->end() && !issues->second.problems.empty();
			CatalogEntry upd;
			const bool hasUpd = !bad && FindUpdateFor(st, *m, upd);
			p.Text(kListX + 350, y, 100, kRowH, bad ? "With errors" : hasUpd ? "\xE2\x86\x91 " + upd.version : m->gameplay ? "Gameplay" : m->kind == kModContent ? "Maps" : "Cosmetic",
				bad ? kBad : hasUpd ? RGB(255, 226, 140) : m->gameplay ? kGold : m->kind == kModContent ? RGB(140, 196, 176) : kDim);
			const auto isIt = st.issues->find(r.folder);
			if (isIt != st.issues->end() && (!isIt->second.problems.empty() || isIt->second.overwrites > 0 || !isIt->second.notices.empty())) {
				PaintAlertIcon(p, kListX + 456, y + 6, !isIt->second.problems.empty());
			}
		}
	}
	{
		ListGeom geom;
		ScrollbarPaint(p, kListX + kListW - 10, kListY, kInstalledListH, static_cast<int>(view.size()), visible, st.list, geom, mx, my);
		g_listGeomOut = geom;
	}

	const int dx = kDetailX, dw = kDesignW - dx - 16;
	const ModInfo *m = nullptr;
	if (st.selected >= 0 && st.selected < static_cast<int>(st.rows.size())) {
		m = FindModIn(st.mods, st.rows[st.selected].folder);
	}
	if (!m) return;
	const std::shared_ptr<const GbMod> io = m->gbId ? GbInfo(m->gbId) : nullptr;
	int y = kContentY;
	p.Font(2);
	p.Text(dx, y, dw, 28, m->name, kGold, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
	y += 30;
	std::string by;
	for (size_t i = 0; i < m->authors.size(); i++) by += (i ? ", " : "") + m->authors[i];
	if (by.empty() && io) by = io->author;
	p.Font(3);
	if (m->gbId) {
		const int iw = 112, ih = 63;
		PaintIoImage(p, dx, y, iw, ih, io ? io->logoUrl : "");
		p.Frame(dx, y, iw, ih, pal::line);
		const int tx = dx + iw + 10, tw = dw - iw - 10;
		p.Text(tx, y + 2, tw, 18, m->version.empty() ? std::string("No version") : Tr("Version ") + m->version, kText);
		p.Text(tx, y + 22, tw, 18, by.empty() ? std::string() : (by.find(',') == std::string::npos ? Tr("Author: ") : Tr("Authors: ")) + by, kText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		p.Text(tx, y + 42, tw, 18, HumanBytes(m->totalBytes), kDim);
		y += ih + 6;
	}
	else {
		p.Text(dx, y, dw, 18, (m->version.empty() ? std::string("No version") : Tr("Version ") + m->version) +
			(by.empty() ? "" : std::string("  ·  ") + (by.find(',') == std::string::npos ? Tr("Author: ") : Tr("Authors: ")) + by) + "  ·  " + HumanBytes(m->totalBytes), kText,
			DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		y += 18;
	}
	p.Text(dx, y, dw, 18, m->gameplay ? "Gameplay: everyone must have it online" : m->kind == kModContent ? "Maps and campaigns: only needed by whoever plays them" : "Cosmetic: does not affect games",
		m->gameplay ? kGold : m->kind == kModContent ? RGB(140, 196, 176) : kDim, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
	y += 18;
	{
		const auto it = st.usedBy.find(m->folder);
		std::string used;
		if (it != st.usedBy.end()) {
			for (const std::string &n : it->second) used += (used.empty() ? "" : ", ") + ModsProfileLabel(n);
		}
		p.Text(dx, y, dw, 18, used.empty() ? "Not in any profile" : Tr("In profiles: ") + used, kDim,
			DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		y += 18;
	}
	const auto isIt = st.issues->find(m->folder);
	if (isIt != st.issues->end()) {
		const ModIssues &is = isIt->second;
		for (size_t k = 0; k < is.problems.size() && k < 3; k++) {
			PaintAlertIcon(p, dx, y + 2, true, 14);
			y += p.TextWrapped(dx + 20, y, dw - 20, 54, is.problems[k], kBad);
		}
		for (const std::string &notice : is.notices) {
			if (y > kListY + kListH - 110) break;
			p.Font(3);
			y += 2 + p.TextWrapped(dx, y + 2, dw, 36, notice, kGold);
		}
		for (size_t k = 0; k < is.overlaps.size(); k++) {
			const ModOverlap &ov = is.overlaps[k];
			if (ov.problem) continue;
			if (y > kListY + kListH - 110) break;
			p.Font(3);
			y += 2 + p.TextWrapped(dx, y + 2, dw, 36, OverlapText(ov), kGold);
			if (!ov.choosable) continue;
			if (ov.canUseMine || ov.canUseTheirs) {
				const int bw = (dw - 6) / 2;
				const int arg = static_cast<int>(k) * 2;
				int bx = dx;
				if (ov.canUseMine) {
					p.Button(bx, y + 4, bw, 24, "Use this pack's files", kUseFiles, arg, mx, my, !st.busy, false, 0);
					bx += bw + 6;
				}
				if (ov.canUseTheirs) p.Button(bx, y + 4, bw, 24, TrFmt("Use the files of %s", {ov.name}), kUseFiles, arg + 1, mx, my, !st.busy, false, 0);
				y += 30;
			}
			else {
				y += 2 + p.TextWrapped(dx, y + 2, dw, 36, "They are inside .pak files, so you can't choose which ones are used", kDim);
			}
		}
		if (!is.files.empty() || !is.values.empty()) {
			const std::string files = std::to_string(is.files.size()), values = std::to_string(is.values.size());
			std::string label = TrFmt("Shared files (%s) and values (%s)", {files, values});
			if (is.values.empty()) label = TrFmt("Shared files (%s)", {files});
			else if (is.files.empty()) label = TrFmt("Shared values (%s)", {values});
			Combo(p, st, dx, y + 4, dw, 24, label, kDropClashFiles);
			y += 30;
		}
	}
	const int btnY = kListY + kListH + 6;
	CatalogEntry update;
	const bool hasUpdate = FindUpdateFor(st, *m, update);
	const bool canVersions = m->gbId != 0;
	const int textBottom = btnY - (hasUpdate || canVersions ? 38 : 6);
	if (textBottom - y > 40) {
		std::string notes, noteVersion;
		if (m->gbId) GbChangelog(m->gbId, m->version, notes, noteVersion);
		p.Fill(dx, y + 2, dw, 1, pal::lineDim);
		const bool own = !m->description.empty();
		PaintRich(p, st, dx, y + 8, dw, textBottom - y - 8, own || !io ? "" : io->descriptionHtml,
			own ? m->description : io ? (!io->description.empty() ? io->description : io->summary) : "",
			Tr("What's new in version ") + noteVersion, notes, 3);
	}
	const int vw = 176;
	if (hasUpdate) {
		p.Button(dx, btnY - 32, canVersions ? dw - vw - 6 : dw, 26, Tr("Update to version ") + update.version, kUpdateMod, 0, mx, my, !st.dl.active, true);
	}
	if (canVersions) Combo(p, st, hasUpdate ? dx + dw - vw : dx, btnY - 32, hasUpdate ? vw : dw, 26, "Other versions", kDropVersions);
	const bool pending = Pending(st, m->folder);
	p.Button(dx, btnY, 90, 26, "Folder", kOpenMod, 0, mx, my, true, false, 0);
	int uninstallX = dx + 96;
	if (m->gbId) {
		p.Button(dx + 96, btnY, 110, 26, "GameBanana", kOpenSource, 0, mx, my, io && !io->page.empty(), false, 0);
		uninstallX = dx + 212;
	}
	p.Button(uninstallX, btnY, dw - (uninstallX - dx), 26, pending ? "On restart" : "Uninstall", kUninstall, 0, mx, my, !pending && !st.busy, false, 0);
}

void PaintField(Painter &p, const State &st, int x, int y, int w, int h, const std::string &text, const std::string &placeholder,
	bool focused, bool secret, int action, int mx, int my)
{
	(void)mx;
	(void)my;
	PaintEditField(p, x, y, w, h, text, placeholder, focused, secret, st.edit, action);
}

void PaintFooter(Painter &p, const State &st)
{
	const int mx = st.mouseX, my = st.mouseY;
	p.Fill(16, kFootY, kDesignW - 32, 1, pal::line);
	const bool restart = st.restartNeeded;
	const int by = kFootY + 10;
	int updates = 0;
	for (const ModInfo &m : st.mods) {
		CatalogEntry e;
		if (FindUpdateFor(st, m, e)) updates++;
	}
	if (updates) {
		p.Button(488, by, 210, 30, Tr("Update all (") + std::to_string(updates) + ")", kUpdateAll, 0, mx, my, !st.dl.active, true);
	}
	p.Button(16, by, 150, 30, "Open folder", kOpenMods, 0, mx, my);
	p.Button(172, by, 140, 30, "Rescan", kRescan, 0, mx, my);
	p.Button(318, by, 164, 30, "Install file...", kPickArchive, 0, mx, my);
	p.Button(kDesignW - 16 - 100 - 6 - 170, by, 170, 30, restart ? "Apply" : "Profile loaded", kApply, 0, mx, my, restart, true);
	p.Button(kDesignW - 16 - 100, by, 100, 30, "Close", kClose, 0, mx, my);
}
}
