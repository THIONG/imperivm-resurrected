// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#include <windows.h>
#include <cmath>
#include "addr/state.h"
#include "net/spectator.h"
#include "net/steamnet.h"

#include "ui/modui_internal.h"

using namespace modui_impl;

namespace modui_impl {
void DrawSpectatorBar(uint32_t *buf, int W, int H, float s, int topY)
{
	if (SpectatorCovering()) return;
	SpectatorState st;
	if (!SpectatorControls(st)) {
		std::lock_guard<std::mutex> lock(g_lock);
		g_spec.barShown = false;
		g_spec.drag = false;
		g_spec.move = false;
		return;
	}
	POINT mouse;
	bool drag;
	double dragFrac;
	{
		std::lock_guard<std::mutex> lock(g_lock);
		mouse = {g_hover.x, g_hover.y};
		drag = g_spec.drag;
		dragFrac = g_spec.dragFrac;
	}
	double posX = 0.5, posTop = -1;
	SpectatorBarPos(posX, posTop, 0);
	EnsureFonts(s);
	const COLORREF panel = pal::dialog, frameOuter = kGold, frameInner = pal::line;
	const COLORREF btn = pal::rowHover, btnHot = RGB(92, 72, 48), strong = RGB(122, 82, 30), strongHot = RGB(150, 104, 40);
	const COLORREF btnFrame = pal::edgeStrong, label = RGB(240, 226, 180), red = RGB(226, 70, 58);
	const float line = std::max(1.0f, s), rad = 2.0f * s;
	const float pad = 12 * s, gap = 8 * s;
	const float pillH = 32 * s;
	const bool collapsed = st.collapsed;
	const bool pinned = st.broadcast && st.atLive;
	const unsigned shownCur = pinned ? st.total : st.cur;
	const float barH = collapsed ? 40 * s : 58 * s;
	const float barW = collapsed ? 150 * s : std::min(static_cast<float>(W) - 24, (st.broadcast ? 900.0f : 790.0f) * s);
	float bx = posX * W - barW / 2, by = posTop >= 0 ? static_cast<float>(posTop * H) : static_cast<float>(topY);
	bx = std::max(4.0f, std::min(static_cast<float>(W) - barW - 4, std::floor(bx)));
	by = std::max(2.0f, std::min(static_cast<float>(H) - barH - 4, by));
	const float cy = by + barH / 2;

	const float mx = static_cast<float>(mouse.x), my = static_cast<float>(mouse.y);
	FillRound(buf, W, H, bx, by, barW, barH, rad, panel, 0.96f);
	StrokeRound(buf, W, H, bx, by, barW, barH, rad, line, frameOuter, 1.0f);
	StrokeRound(buf, W, H, bx + 3 * s, by + 3 * s, barW - 6 * s, barH - 6 * s, rad, line, frameInner, 1.0f);

	auto button = [&](const RectF &r, bool isStrong, bool active, bool hot) {
		const bool useStrong = isStrong || active;
		FillRound(buf, W, H, r.x, r.y, r.w, r.h, rad, useStrong ? (hot ? strongHot : strong) : (hot ? btnHot : btn), 1.0f);
		StrokeRound(buf, W, H, r.x, r.y, r.w, r.h, rad, line, btnFrame, 1.0f);
	};
	auto chevron = [&](float cx, float cyy, float hgt, int dir, COLORREF c) {
		const float wd = hgt * 0.62f;
		if (dir < 0) FillTriangle(buf, W, H, cx + wd / 2, cyy - hgt / 2, cx + wd / 2, cyy + hgt / 2, cx - wd / 2, cyy, c, 1.0f);
		else FillTriangle(buf, W, H, cx - wd / 2, cyy - hgt / 2, cx - wd / 2, cyy + hgt / 2, cx + wd / 2, cyy, c, 1.0f);
	};
	auto arrow = [&](float cx, float cyy, float wd, bool up, COLORREF c) {
		if (up) FillTriangle(buf, W, H, cx - wd / 2, cyy + wd / 4, cx + wd / 2, cyy + wd / 4, cx, cyy - wd / 4, c, 1.0f);
		else FillTriangle(buf, W, H, cx - wd / 2, cyy - wd / 4, cx + wd / 2, cyy - wd / 4, cx, cyy + wd / 4, c, 1.0f);
	};

	RectF rCollapse{bx + pad, cy - 13 * s, 26 * s, 26 * s};
	RectF rBack, rPlay, rFwd, rSpeed, rUi, rLive, rCur, rTotal, rTrack;
	std::string tipFor;
	std::vector<ControlKey> tipKeys;
	float tipX = bx;

	if (collapsed) {
		rCollapse = {bx + pad, cy - 13 * s, barW - pad * 2, 26 * s};
		const bool h = rCollapse.Has(mx, my);
		button(rCollapse, false, false, h);
		arrow(rCollapse.x + 16 * s, cy, 12 * s, false, label);
		DrawLabel(buf, W, H, ClockText(shownCur), rCollapse.x + 30 * s, rCollapse.y, rCollapse.w - 36 * s, rCollapse.h, DT_LEFT, label, 1.0f);
		if (h) { tipFor = "Show the bar"; tipKeys = {ControlsModKey(kModKeySpecBar)}; tipX = rCollapse.x; }
	}
	else {
		float x = rCollapse.x + rCollapse.w + gap;
		rBack = {x, cy - pillH / 2, 64 * s, pillH};
		x += rBack.w + gap;
		rPlay = {x, cy - 19 * s, 54 * s, 38 * s};
		x += rPlay.w + gap;
		rFwd = {x, cy - pillH / 2, 64 * s, pillH};
		x += rFwd.w + gap * 1.5f;
		float rx = bx + barW - pad;
		if (st.broadcast) {
			rLive = {rx - 104 * s, cy - pillH / 2, 104 * s, pillH};
			rx -= rLive.w + gap;
		}
		rUi = {rx - 40 * s, cy - pillH / 2, 40 * s, pillH};
		rx -= rUi.w + gap;
		rSpeed = {rx - 52 * s, cy - pillH / 2, 52 * s, pillH};
		rx -= rSpeed.w + gap * 1.5f;
		const float lab = 50 * s;
		rCur = {x, cy - 12 * s, lab, 24 * s};
		x += lab + gap * 0.5f;
		rTotal = {rx - lab, cy - 12 * s, lab, 24 * s};
		rx -= lab + gap * 0.5f;
		rTrack = {x, cy - 14 * s, std::max(40.0f, rx - x), 28 * s};

		const bool hCollapse = rCollapse.Has(mx, my), hBack = rBack.Has(mx, my), hPlay = rPlay.Has(mx, my), hFwd = rFwd.Has(mx, my);
		const bool hSpeed = rSpeed.Has(mx, my), hUi = rUi.Has(mx, my), hLive = st.broadcast && rLive.Has(mx, my), hTrack = rTrack.Has(mx, my) || drag;

		button(rCollapse, false, false, hCollapse);
		arrow(rCollapse.x + rCollapse.w / 2, cy, 12 * s, true, label);
		button(rBack, false, false, hBack);
		{
			const float cx = rBack.x + rBack.w / 2;
			chevron(cx - 17 * s, cy, 12 * s, -1, label);
			chevron(cx - 9 * s, cy, 12 * s, -1, label);
			DrawLabel(buf, W, H, L"10", cx - 1 * s, cy - 12 * s, 26 * s, 24 * s, DT_LEFT, label, 1.0f);
		}
		button(rFwd, false, false, hFwd);
		{
			const float fx = rFwd.x + rFwd.w / 2;
			DrawLabel(buf, W, H, L"10", fx - 25 * s, cy - 12 * s, 24 * s, 24 * s, DT_RIGHT, label, 1.0f);
			chevron(fx + 9 * s, cy, 12 * s, 1, label);
			chevron(fx + 17 * s, cy, 12 * s, 1, label);
		}
		button(rPlay, true, false, hPlay);
		{
			const float cx = rPlay.x + rPlay.w / 2;
			if (st.paused) {
				FillTriangle(buf, W, H, cx - 5 * s, cy - 9 * s, cx - 5 * s, cy + 9 * s, cx + 9 * s, cy, label, 1.0f);
			}
			else {
				FillRound(buf, W, H, cx - 7 * s, cy - 8 * s, 5 * s, 16 * s, 1.0f * s, label, 1.0f);
				FillRound(buf, W, H, cx + 2 * s, cy - 8 * s, 5 * s, 16 * s, 1.0f * s, label, 1.0f);
			}
		}
		button(rSpeed, false, st.speed > 1, hSpeed);
		{
			wchar_t text[12];
			swprintf_s(text, L"×%d", st.speed);
			DrawLabel(buf, W, H, text, rSpeed.x, rSpeed.y, rSpeed.w, rSpeed.h, DT_CENTER, st.broadcast && st.atLive ? kDim : label, 1.0f);
		}
		button(rUi, false, st.uiHidden, hUi);
		DrawLabel(buf, W, H, L"UI", rUi.x, rUi.y, rUi.w, rUi.h, DT_CENTER, label, 1.0f);

		if (st.broadcast) {
			button(rLive, st.ended, st.atLive, hLive);
			const float dx = rLive.x + 16 * s;
			if (st.atLive && !st.ended) FillDisc(buf, W, H, dx, cy, 6.5f * s, red, 0.3f);
			if (!st.ended) FillDisc(buf, W, H, dx, cy, 4 * s, st.atLive ? red : RGB(150, 130, 100), 1.0f);
			std::wstring text = ::W(Tr(st.ended ? "Leave" : "Go live"));
			CharUpperBuffW(&text[0], static_cast<DWORD>(text.size()));
			DrawLabel(buf, W, H, text, st.ended ? rLive.x : rLive.x + 28 * s, rLive.y, st.ended ? rLive.w : rLive.w - 34 * s, rLive.h, st.ended ? DT_CENTER : (DT_LEFT | DT_END_ELLIPSIS), label, 1.0f);
		}
		double frac = st.total ? std::min(1.0, static_cast<double>(shownCur) / st.total) : 0.0;
		if (drag) frac = dragFrac;
		const float trackX0 = rTrack.x + 6 * s, trackX1 = rTrack.x + rTrack.w - 6 * s, trackLen = std::max(1.0f, trackX1 - trackX0);
		{
			const float th = (hTrack ? 10.0f : 8.0f) * s;
			FillRound(buf, W, H, trackX0, cy - th / 2, trackLen, th, 1.0f * s, pal::well, 1.0f);
			StrokeRound(buf, W, H, trackX0, cy - th / 2, trackLen, th, 1.0f * s, line, frameInner, 1.0f);
			const float fillLen = static_cast<float>(trackLen * frac);
			if (fillLen > 1.0f) FillRound(buf, W, H, trackX0 + line, cy - th / 2 + line, std::max(1.0f, fillLen - line), th - 2 * line, 0.5f * s, st.busy ? RGB(120, 160, 210) : kGold, 1.0f);
			const float hw = (hTrack ? 10.0f : 8.0f) * s, hh = (hTrack ? 22.0f : 18.0f) * s, hx = trackX0 + fillLen;
			FillRound(buf, W, H, hx - hw / 2, cy - hh / 2, hw, hh, rad, hTrack ? strongHot : strong, 1.0f);
			StrokeRound(buf, W, H, hx - hw / 2, cy - hh / 2, hw, hh, rad, line, btnFrame, 1.0f);
			DrawLabel(buf, W, H, ClockText(drag ? static_cast<unsigned>(dragFrac * st.total) : shownCur), rCur.x, rCur.y, rCur.w, rCur.h, DT_RIGHT, kText, 1.0f);
			DrawLabel(buf, W, H, ClockText(st.total), rTotal.x, rTotal.y, rTotal.w, rTotal.h, DT_LEFT, kDim, 1.0f);
			if (hTrack && !drag) {
				const double f = std::max(0.0, std::min(1.0, static_cast<double>((mx - trackX0) / trackLen)));
				const std::wstring t = ClockText(static_cast<unsigned>(f * st.total));
				const float bw = 58 * s, bh = 26 * s, bxx = std::max(bx + 4 * s, std::min(bx + barW - bw - 4 * s, mx - bw / 2));
				const float byy = by + barH + 8 * s + bh > H ? by - bh - 8 * s : by + barH + 8 * s;
				FillRound(buf, W, H, bxx, byy, bw, bh, rad, panel, 0.97f);
				StrokeRound(buf, W, H, bxx, byy, bw, bh, rad, line, frameOuter, 1.0f);
				DrawLabel(buf, W, H, t, bxx, byy, bw, bh, DT_CENTER, label, 1.0f);
			}
		}
		if (!drag) {
			if (hCollapse) { tipFor = "Hide the bar"; tipKeys = {ControlsModKey(kModKeySpecBar)}; tipX = rCollapse.x; }
			else if (hBack) { tipFor = "Back 10 s"; tipKeys = {ControlsModKey(kModKeySpecBack)}; tipX = rBack.x; }
			else if (hPlay) { tipFor = st.paused ? "Resume" : "Pause"; tipKeys = {ControlsModKey(kModKeySpecPause)}; tipX = rPlay.x; }
			else if (hFwd) { tipFor = "Forward 10 s"; tipKeys = {ControlsModKey(kModKeySpecForward)}; tipX = rFwd.x; }
			else if (hSpeed) { tipFor = "Speed: click, or"; tipKeys = {ControlsModKey(kModKeySpecFaster), ControlsModKey(kModKeySpecSlower)}; tipX = rSpeed.x - 120 * s; }
			else if (hUi) { tipFor = st.uiHidden ? "Show the game interface" : "Hide the game interface"; tipKeys = {ControlsModKey(kModKeySpecUi)}; tipX = rUi.x - 100 * s; }
			else if (hLive) { tipFor = st.ended ? "Leave to the menu" : "Jump to live"; tipKeys = {ControlsModKey(kModKeySpecLive)}; tipX = rLive.x - 40 * s; }
		}
	}
	if (!tipFor.empty()) {
		const float tipY = by + barH + 30 * s > H - 40 * s ? by - 30 * s : by + barH + 30 * s;
		ModUiDrawHoverCard(buf, W, H, tipFor, kGold, static_cast<int>(std::max(4.0f, tipX)), static_cast<int>(tipY), KnownKeys(tipKeys));
	}
	std::lock_guard<std::mutex> lock(g_lock);
	auto store = [&](int i, const RectF &r) { g_spec.btn[i] = {static_cast<LONG>(r.x), static_cast<LONG>(r.y), static_cast<LONG>(r.x + r.w), static_cast<LONG>(r.y + r.h)}; };
	for (int i = 0; i < 9; i++) g_spec.btn[i] = RECT{};
	if (collapsed) {
		store(7, rCollapse);
	}
	else {
		store(0, rPlay);
		store(1, rSpeed);
		store(2, rBack);
		store(3, rFwd);
		if (st.broadcast) store(4, rLive);
		store(6, rUi);
		store(7, rCollapse);
		const float trackX0 = rTrack.x + 6 * s, trackX1 = rTrack.x + rTrack.w - 6 * s;
		g_spec.btn[5] = {static_cast<LONG>(trackX0), static_cast<LONG>(rTrack.y), static_cast<LONG>(trackX1), static_cast<LONG>(rTrack.y + rTrack.h)};
	}
	g_spec.bar = {static_cast<LONG>(bx), static_cast<LONG>(by), static_cast<LONG>(bx + barW), static_cast<LONG>(by + barH)};
	g_spec.winW = W;
	g_spec.winH = H;
	g_spec.barShown = true;
}

void DrawWatchersBadge(uint32_t *buf, int W, int H, float s)
{
	const int watchers = SteamRoomWatchers();
	if (watchers <= 0 || !*reinterpret_cast<void **>(addr::kGame)) return;
	SpectatorState watching;
	if (SpectatorControls(watching)) return;
	EnsureFonts(s);
	const COLORREF panel = pal::dialog, frame = kGold, ink = RGB(240, 226, 180);
	wchar_t text[16];
	swprintf_s(text, L"%d", watchers);
	const float h = 26 * s, eyeW = 20 * s, textW = (6 + 8 * static_cast<float>(wcslen(text))) * s, w = 10 * s + eyeW + 6 * s + textW + 10 * s;
	const float x = std::floor(static_cast<float>(W) - w - 14 * s), y = std::floor(96 * s);
	FillRound(buf, W, H, x, y, w, h, 4 * s, panel, 0.88f);
	StrokeRound(buf, W, H, x, y, w, h, 4 * s, std::max(1.0f, s), frame, 1.0f);
	const float cx = x + 10 * s + eyeW / 2, cy = y + h / 2;
	StrokeRound(buf, W, H, cx - eyeW / 2, cy - 5 * s, eyeW, 10 * s, 5 * s, std::max(1.0f, 1.3f * s), ink, 1.0f);
	FillDisc(buf, W, H, cx, cy, 3 * s, ink, 1.0f);
	DrawLabel(buf, W, H, text, x + 10 * s + eyeW + 6 * s, y, textW, h, DT_LEFT, ink, 1.0f);
}
}
