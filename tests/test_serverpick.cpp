// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <string>
#include <vector>

#include "core/json.h"
#include "mods/serverpick.h"

namespace {

const double MB = 1048576.0;

std::vector<std::string> Servers(std::initializer_list<int> numbers)
{
	std::vector<std::string> out;
	for (int n : numbers) out.push_back(GbFileServerHost(n));
	return out;
}

std::vector<FileServer> Weighted(std::initializer_list<int> numbers)
{
	std::vector<FileServer> out;
	for (int n : numbers) out.push_back({GbFileServerHost(n), 1});
	return out;
}

void RecognisesFileServers()
{
	CHECK_EQ(GbFileServerNumber("filecache46.gamebanana.com"), 46);
	CHECK_EQ(GbFileServerNumber("FileCache7.GameBanana.com"), 7);
	CHECK_EQ(GbFileServerNumber("files.gamebanana.com"), -1);
	CHECK_EQ(GbFileServerNumber("filecache46.gamebanana.com.evil.net"), -1);
	CHECK_EQ(GbFileServerNumber("filecache.gamebanana.com"), -1);
	CHECK_EQ(GbFileServerNumber("filecachex1.gamebanana.com"), -1);
	CHECK_EQ(GbFileServerNumber("evilfilecache46.gamebanana.com"), -1);
	CHECK_EQ(GbFileServerHost(27), std::string("filecache27.gamebanana.com"));
	CHECK(IsGbFileServer("filecache51.gamebanana.com"));
}

void EveryPlayerTriesTheSameServersForAFile()
{
	const std::vector<std::string> a = RendezvousOrder("/mods/caesar_mod.zip", Weighted({20, 27, 28, 45, 50}));
	const std::vector<std::string> b = RendezvousOrder("/mods/caesar_mod.zip", Weighted({50, 45, 28, 27, 20, 27}));
	CHECK_EQ(a, b);
	CHECK_EQ(a.size(), static_cast<size_t>(5));
	const std::vector<std::string> c = RendezvousOrder("/mods/community_mod.zip", Weighted({20, 27, 28, 45, 50}));
	CHECK(a != c);
	std::vector<std::string> fewer = RendezvousOrder("/mods/caesar_mod.zip", Weighted({20, 27, 28, 45}));
	std::vector<std::string> aWithout50;
	for (const std::string &s : a) {
		if (s != GbFileServerHost(50)) aWithout50.push_back(s);
	}
	CHECK_EQ(fewer, aWithout50);
}

void FasterServersAreTriedMoreOften()
{
	const std::vector<FileServer> servers = {{GbFileServerHost(42), 200000}, {GbFileServerHost(43), 3400000}, {GbFileServerHost(53), 2200000}};
	int first43 = 0, first42 = 0;
	for (int i = 0; i < 1000; i++) {
		const std::string top = RendezvousOrder("/mods/file" + std::to_string(i) + ".zip", servers).front();
		if (top == GbFileServerHost(43)) first43++;
		if (top == GbFileServerHost(42)) first42++;
	}
	CHECK(first43 > 450);
	CHECK(first42 < 80);
}

void ReadsTheOfficialList()
{
	Json j;
	CHECK(Json::Parse("{\"_aRecords\":["
		"{\"_idRow\":1,\"_sDomain\":\"filecache20\",\"_sState\":\"up\",\"_aStats\":{\"_a1hr\":{\"_fRate\":2177734}}},"
		"{\"_idRow\":2,\"_sDomain\":\"filecache25\",\"_sState\":\"terminated\"},"
		"{\"_idRow\":3,\"_sDomain\":\"FileCache43\",\"_sState\":\"up\"},"
		"{\"_idRow\":4,\"_sDomain\":\"evil.example.com/x\",\"_sState\":\"up\"},"
		"{\"_idRow\":5,\"_sDomain\":\"filecache44\",\"_sState\":\"down\"}]}", j));
	const std::vector<FileServer> list = ParseFileservers(j);
	CHECK_EQ(list.size(), static_cast<size_t>(2));
	CHECK_EQ(list[0].host, GbFileServerHost(20));
	CHECK_EQ(static_cast<int>(list[0].weight), 2177734);
	CHECK_EQ(list[1].host, GbFileServerHost(43));
	CHECK_EQ(static_cast<int>(list[1].weight), 1);
	CHECK(ParseFileservers(Json()).empty());
}

void ConnectionReference()
{
	std::vector<double> recent;
	CHECK_EQ(static_cast<int>(ConnectionRate(recent)), 0);
	recent = RememberRate(recent, 0);
	CHECK(recent.empty());
	recent = RememberRate(recent, 20 * MB);
	for (int i = 0; i < 4; i++) recent = RememberRate(recent, 0.7 * MB);
	CHECK_EQ(static_cast<int>(ConnectionRate(recent) / MB), 20);
	recent = RememberRate(recent, 0.7 * MB);
	CHECK_EQ(recent.size(), kRecentDownloads);
	CHECK_EQ(static_cast<int>(ConnectionRate(recent) * 10 / MB), 7);
}

void AGoodServerIsLeftAlone()
{
	ServerPicker p(GbFileServerHost(27), 16 * MB);
	CHECK(!p.WantsCandidates(15 * MB));
	CHECK_EQ(p.OnWindow(15 * MB), ServerPicker::kStay);
	CHECK_EQ(p.Current(), GbFileServerHost(27));
	CHECK_EQ(p.OnWindow(17 * MB), ServerPicker::kStay);
	CHECK_EQ(p.Current(), GbFileServerHost(27));
}

void ASlowLinkIsNotMistakenForASlowServer()
{
	ServerPicker p(GbFileServerHost(46), 1.5 * MB);
	CHECK(!p.WantsCandidates(1.3 * MB));
	CHECK_EQ(p.OnWindow(1.3 * MB), ServerPicker::kStay);
	CHECK_EQ(p.Current(), GbFileServerHost(46));
}

void ASlowServerIsReplaced()
{
	ServerPicker p(GbFileServerHost(46), 16 * MB);
	CHECK(p.WantsCandidates(0.4 * MB));
	p.SetCandidates(Servers({27, 46, 45}));
	CHECK_EQ(p.OnWindow(0.4 * MB), ServerPicker::kTry);
	CHECK_EQ(p.Current(), GbFileServerHost(27));
	CHECK(p.Testing());
	CHECK_EQ(p.OnWindow(15 * MB), ServerPicker::kKeepNew);
	CHECK_EQ(p.Current(), GbFileServerHost(27));
	CHECK_EQ(p.Best(), GbFileServerHost(27));
	CHECK(!p.Testing());
	CHECK_EQ(p.OnWindow(16 * MB), ServerPicker::kStay);
}

void WithoutAReferenceItTriesUntilNothingIsClearlyFaster()
{
	ServerPicker p(GbFileServerHost(46), 0);
	CHECK(p.WantsCandidates(20 * MB));
	p.SetCandidates(Servers({27, 45, 50, 20}));
	CHECK_EQ(p.OnWindow(0.5 * MB), ServerPicker::kTry);
	CHECK_EQ(p.OnWindow(10 * MB), ServerPicker::kTry);
	CHECK_EQ(p.Best(), GbFileServerHost(27));
	CHECK_EQ(p.Current(), GbFileServerHost(45));
	CHECK_EQ(p.OnWindow(11 * MB), ServerPicker::kBack);
	CHECK_EQ(p.Current(), GbFileServerHost(27));
	CHECK_EQ(p.OnWindow(10 * MB), ServerPicker::kStay);
	CHECK_EQ(static_cast<int>(p.FastestSeen() / MB), 11);
}

void WhenEveryoneIsAsSlowItGoesBackToTheAssignedServer()
{
	ServerPicker p(GbFileServerHost(46), 0);
	p.SetCandidates(Servers({27, 45}));
	CHECK_EQ(p.OnWindow(1.2 * MB), ServerPicker::kTry);
	CHECK_EQ(p.OnWindow(1.3 * MB), ServerPicker::kBack);
	CHECK_EQ(p.Current(), GbFileServerHost(46));
	CHECK_EQ(p.OnWindow(1.2 * MB), ServerPicker::kStay);
	CHECK_EQ(p.Current(), GbFileServerHost(46));
}

void ADropLeadsToAnotherTry()
{
	ServerPicker p(GbFileServerHost(27), 16 * MB);
	CHECK_EQ(p.OnWindow(15 * MB), ServerPicker::kStay);
	CHECK_EQ(p.OnWindow(10 * MB), ServerPicker::kStay);
	CHECK(p.WantsCandidates(5 * MB));
	p.SetCandidates(Servers({45, 50}));
	CHECK_EQ(p.OnWindow(5 * MB), ServerPicker::kTry);
	CHECK_EQ(p.Current(), GbFileServerHost(45));
	CHECK_EQ(p.OnWindow(14 * MB), ServerPicker::kKeepNew);
	CHECK_EQ(p.Current(), GbFileServerHost(45));
}

void KeepsLookingWhileEverythingIsFarBelowTheConnection()
{
	ServerPicker p(GbFileServerHost(39), 20 * MB);
	p.SetCandidates(Servers({41, 27, 45, 50}));
	CHECK_EQ(p.OnWindow(0.8 * MB), ServerPicker::kTry);
	CHECK_EQ(p.Current(), GbFileServerHost(41));
	CHECK_EQ(p.OnWindow(0.3 * MB), ServerPicker::kTry);
	CHECK_EQ(p.Best(), GbFileServerHost(39));
	CHECK_EQ(p.Current(), GbFileServerHost(27));
	CHECK_EQ(p.OnWindow(18 * MB), ServerPicker::kKeepNew);
	CHECK_EQ(p.Current(), GbFileServerHost(27));

	ServerPicker q(GbFileServerHost(39), 20 * MB);
	q.SetCandidates(Servers({41, 27, 45, 50}));
	CHECK_EQ(q.OnWindow(0.8 * MB), ServerPicker::kTry);
	CHECK_EQ(q.OnWindow(0.3 * MB), ServerPicker::kTry);
	CHECK_EQ(q.OnWindow(0.4 * MB), ServerPicker::kTry);
	CHECK_EQ(q.OnWindow(0.5 * MB), ServerPicker::kBack);
	CHECK_EQ(q.Current(), GbFileServerHost(39));
	CHECK(!q.Testing());
}

void FailuresMoveOn()
{
	ServerPicker p(GbFileServerHost(46), 16 * MB);
	p.SetCandidates(Servers({27, 45}));
	CHECK_EQ(p.OnWindow(0.4 * MB), ServerPicker::kTry);
	CHECK_EQ(p.OnFailure(), ServerPicker::kTry);
	CHECK_EQ(p.Current(), GbFileServerHost(45));
	CHECK_EQ(p.OnFailure(), ServerPicker::kBack);
	CHECK_EQ(p.Current(), GbFileServerHost(46));

	ServerPicker q(GbFileServerHost(44), 16 * MB);
	q.SetCandidates(Servers({27}));
	CHECK_EQ(q.OnFailure(), ServerPicker::kTry);
	CHECK_EQ(q.Current(), GbFileServerHost(27));
	CHECK_EQ(q.OnWindow(0.2 * MB), ServerPicker::kKeepNew);
	CHECK_EQ(q.Current(), GbFileServerHost(27));
}

void OnlyComparedRatesAreRemembered()
{
	ServerPicker alone(GbFileServerHost(46), 0);
	alone.SetCandidates(Servers({50}));
	CHECK(!alone.RateWorthRemembering());
	CHECK_EQ(alone.OnWindow(0.7 * MB), ServerPicker::kTry);
	CHECK(!alone.RateWorthRemembering());
	CHECK_EQ(alone.OnWindow(20 * MB), ServerPicker::kKeepNew);
	CHECK(alone.RateWorthRemembering());

	ServerPicker confirmed(GbFileServerHost(31), 16 * MB);
	CHECK_EQ(confirmed.OnWindow(11 * MB), ServerPicker::kStay);
	CHECK(confirmed.RateWorthRemembering());

	ServerPicker stuck(GbFileServerHost(46), 16 * MB);
	CHECK_EQ(stuck.OnWindow(0.7 * MB), ServerPicker::kStay);
	CHECK(!stuck.RateWorthRemembering());

	ServerPicker quick(GbFileServerHost(39), 0);
	quick.SetCandidates(Servers({54}));
	CHECK_EQ(quick.OnWindow(0.37 * MB), ServerPicker::kTry);
	quick.NoteRate(GbFileServerHost(54), 19 * MB);
	CHECK(quick.RateWorthRemembering());
	CHECK_EQ(static_cast<int>(quick.FastestSeen() / MB), 19);
}

void TriesAreLimited()
{
	ServerPicker p(GbFileServerHost(46), 100 * MB);
	p.SetCandidates(Servers({20, 27, 28, 30, 31}));
	double rate = 0.1 * MB;
	CHECK_EQ(p.OnWindow(rate), ServerPicker::kTry);
	CHECK_EQ(p.OnWindow(rate *= 2), ServerPicker::kTry);
	CHECK_EQ(p.OnWindow(rate *= 2), ServerPicker::kTry);
	CHECK_EQ(p.OnWindow(rate *= 2), ServerPicker::kKeepNew);
	CHECK(!p.Testing());
	CHECK_EQ(p.OnWindow(rate / 4), ServerPicker::kStay);
}

void NoCandidatesMeansNoChange()
{
	ServerPicker p(GbFileServerHost(46), 16 * MB);
	p.SetCandidates({GbFileServerHost(46)});
	CHECK_EQ(p.OnWindow(0.4 * MB), ServerPicker::kStay);
	CHECK_EQ(p.Current(), GbFileServerHost(46));
	CHECK_EQ(p.OnFailure(), ServerPicker::kStay);
	CHECK_EQ(p.Current(), GbFileServerHost(46));
}

}

int main()
{
	RecognisesFileServers();
	EveryPlayerTriesTheSameServersForAFile();
	FasterServersAreTriedMoreOften();
	ReadsTheOfficialList();
	ConnectionReference();
	AGoodServerIsLeftAlone();
	ASlowLinkIsNotMistakenForASlowServer();
	ASlowServerIsReplaced();
	WithoutAReferenceItTriesUntilNothingIsClearlyFaster();
	WhenEveryoneIsAsSlowItGoesBackToTheAssignedServer();
	ADropLeadsToAnotherTry();
	KeepsLookingWhileEverythingIsFarBelowTheConnection();
	FailuresMoveOn();
	OnlyComparedRatesAreRemembered();
	TriesAreLimited();
	NoCandidatesMeansNoChange();
	return test::Summary("serverpick");
}
