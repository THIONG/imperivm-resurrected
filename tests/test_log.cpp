// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#include "check.h"

#include <cstdarg>
#include <string>
#include <vector>

#include "core/logcore.h"

namespace {

class MemorySink : public LogSink {
public:
	void Write(const char *data, size_t size) override { text.append(data, size); }
	std::string text;
};

struct Harness {
	MemorySink sink;
	LogTime now;
	unsigned thread = 1;
	LogCore core;

	Harness() : core(&sink, [this] { return now; }, [this] { return thread; }, 1)
	{
		now.year = 2026;
		now.month = 10;
		now.day = 16;
		now.hour = 3;
		now.minute = 2;
		now.second = 59;
		now.ms = 597;
	}

	void Configure(LogLevel level, size_t cap = 16u << 20)
	{
		LogSettings s;
		s.level = level;
		s.maxFileBytes = cap;
		core.Configure(s);
	}
	void W(LogLevel level, const char *channel, const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		core.Write(level, channel, fmt, fmt, args);
		va_end(args);
	}
	void Legacy(const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		core.WriteLegacy(fmt, args);
		va_end(args);
	}
	std::vector<std::string> Lines() const
	{
		std::vector<std::string> out;
		size_t from = 0;
		while (from < sink.text.size()) {
			size_t to = sink.text.find('\n', from);
			if (to == std::string::npos) to = sink.text.size();
			out.push_back(sink.text.substr(from, to - from));
			from = to + 1;
		}
		return out;
	}
	bool Has(const std::string &part) const { return sink.text.find(part) != std::string::npos; }
	size_t At(const std::string &part) const { return sink.text.find(part); }
};

void Format()
{
	Harness h;
	h.W(kLogInfo, "steam", "online ready as %s", "THIONG");
	CHECK_EQ(h.sink.text, "[03:02:59.597] I steam: online ready as THIONG\n");
	h.W(kLogWarn, "rejoin", "no tick %d data", 118);
	h.W(kLogError, "content", "pack %s is not loaded", "a");
	const auto lines = h.Lines();
	CHECK_EQ(lines.size(), size_t(3));
	CHECK_EQ(lines[1], "[03:02:59.597] W rejoin: no tick 118 data");
	CHECK_EQ(lines[2], "[03:02:59.597] E content: pack a is not loaded");
	h.thread = 7;
	h.W(kLogInfo, "steam", "from a worker");
	CHECK(h.Has("I steam#7: from a worker"));
	h.W(kLogInfo, "steam", "multi\nline\r\ntext");
	CHECK(h.Has("I steam#7: multi line  text\n"));
	CHECK_EQ(LogLevelLetter(kLogTrace), 'T');
	CHECK_EQ(LogLevelLetter(kLogFatal), 'F');
}

void Levels()
{
	Harness h;
	h.W(kLogDebug, "steam", "detail");
	h.W(kLogTrace, "steam", "per tick");
	CHECK(h.sink.text.empty());
	h.W(kLogInfo, "steam", "info");
	CHECK(h.Has("I steam: info"));
	h.Configure(kLogDebug);
	h.W(kLogDebug, "steam", "detail now");
	CHECK(h.Has("D steam: detail now"));
	h.W(kLogTrace, "steam", "still hidden");
	CHECK(!h.Has("still hidden"));
	CHECK(h.core.Enabled(kLogDebug, "steam"));
	CHECK(!h.core.Enabled(kLogTrace, "steam"));

	LogSettings s;
	s.level = kLogInfo;
	s.channels = ParseLogChannels("rejoin:trace, STEAM : debug ,bad,x:nope,:info");
	CHECK_EQ(s.channels.size(), size_t(2));
	h.core.Configure(s);
	h.W(kLogTrace, "rejoin", "rejoin trace");
	h.W(kLogDebug, "steam", "steam debug");
	h.W(kLogDebug, "room", "room debug");
	CHECK(h.Has("T rejoin: rejoin trace"));
	CHECK(h.Has("D steam: steam debug"));
	CHECK(!h.Has("room debug"));
}

void Parsing()
{
	LogLevel level = kLogInfo;
	CHECK(ParseLogLevel("Debug", level));
	CHECK_EQ(static_cast<int>(level), static_cast<int>(kLogDebug));
	CHECK(ParseLogLevel(" WARNING ", level));
	CHECK_EQ(static_cast<int>(level), static_cast<int>(kLogWarn));
	CHECK(ParseLogLevel("trace", level));
	CHECK(ParseLogLevel("fatal", level));
	CHECK(!ParseLogLevel("verbose", level));
	CHECK(!ParseLogLevel("", level));
	CHECK(ParseLogChannels("").empty());
	CHECK_EQ(static_cast<int>(ParseLogChannels("a:error").at("a")), static_cast<int>(kLogError));
	CHECK(IsBetaVersion("0.31.55"));
	CHECK(!IsBetaVersion("1.0.0"));
	CHECK(!IsBetaVersion(""));
	const auto beta = BetaLogChannels();
	CHECK_EQ(static_cast<int>(beta.at("rejoin")), static_cast<int>(kLogDebug));
	CHECK_EQ(static_cast<int>(beta.at("steam")), static_cast<int>(kLogDebug));
	CHECK(beta.count("render") == 0);
}

void Context()
{
	Harness h;
	h.W(kLogDebug, "net", "debug one");
	h.W(kLogTrace, "net", "trace one");
	h.W(kLogInfo, "net", "info one");
	h.W(kLogError, "net", "boom");
	const size_t info = h.At("info one"), banner = h.At("context before this point (error)"), debug = h.At("debug one"),
		trace = h.At("trace one"), end = h.At("end of context"), boom = h.At("E net: boom");
	CHECK(info != std::string::npos && banner != std::string::npos && boom != std::string::npos);
	CHECK(info < banner);
	CHECK(banner < debug && debug < trace && trace < end);
	CHECK(end < boom);
	CHECK(h.Has("2 lines below the log level"));
	CHECK_EQ(h.sink.text.find("info one", info + 1), std::string::npos);

	const size_t before = h.sink.text.size();
	h.W(kLogError, "net", "second boom");
	CHECK(h.sink.text.substr(before).find("context") == std::string::npos);

	h.W(kLogDebug, "net", "later detail");
	h.core.DumpContext("requested");
	CHECK(h.Has("context before this point (requested)"));
	CHECK(h.Has("later detail"));
}

void Ring()
{
	Harness h;
	for (int i = 0; i < 3000; i++) h.W(kLogDebug, "net", "line %d", i);
	CHECK(h.sink.text.empty());
	h.W(kLogError, "net", "boom");
	const auto lines = h.Lines();
	size_t context = 0;
	for (const std::string &l : lines) context += l.find(" D net: line ") != std::string::npos ? 1 : 0;
	CHECK_EQ(context, size_t(2047));
	CHECK(h.Has("D net: line 953\n"));
	CHECK(!h.Has("D net: line 952\n"));
	CHECK(h.Has("D net: line 2999\n"));
}

void Flood()
{
	Harness h;
	for (int i = 0; i < 50; i++) h.W(kLogInfo, "vfs", "normal %d", i);
	CHECK_EQ(h.Lines().size(), size_t(50));

	Harness f;
	for (int i = 0; i < 300; i++) f.W(kLogInfo, "vfs", "flood %d", i);
	CHECK_EQ(f.Lines().size(), size_t(100));
	f.now.second += 2;
	f.W(kLogInfo, "vfs", "flood %d", 1000);
	CHECK(f.Has("I vfs: (200 more like this suppressed)"));
	CHECK(f.Has("flood 1000"));

	Harness g;
	for (int i = 0; i < 150; i++) g.W(kLogInfo, "vfs", "flood %d", i);
	g.core.Finish("session", "ended cleanly");
	CHECK(g.Has("(50 more like this suppressed)"));
	CHECK(g.Has("I session: ended cleanly\n"));
	CHECK(g.At("more like this suppressed") < g.At("ended cleanly"));
}

void Repeats()
{
	Harness h;
	for (int i = 0; i < 4; i++) h.W(kLogInfo, "controls", "letters removed from the image");
	h.W(kLogInfo, "controls", "something else");
	const auto lines = h.Lines();
	CHECK_EQ(lines.size(), size_t(3));
	CHECK(lines[0].find("letters removed") != std::string::npos);
	CHECK(lines[1].find("(last message repeated 3 times)") != std::string::npos);
	CHECK(lines[2].find("something else") != std::string::npos);

	Harness g;
	g.W(kLogInfo, "a", "same");
	g.W(kLogInfo, "b", "same");
	g.W(kLogWarn, "a", "same");
	CHECK_EQ(g.Lines().size(), size_t(3));
}

void Limits()
{
	Harness h;
	const std::string big(3000, 'x');
	h.W(kLogInfo, "steam", "%s", big.c_str());
	const auto lines = h.Lines();
	CHECK_EQ(lines.size(), size_t(1));
	CHECK(lines[0].size() < kLogMaxLine + 64);
	CHECK(lines[0].size() > kLogMaxLine - 64);
	CHECK(lines[0].substr(lines[0].size() - 3) == "...");

	Harness c;
	c.Configure(kLogDebug, 2000);
	for (int i = 0; i < 60; i++) c.W(kLogInfo, "steam", "information line number %d", i);
	CHECK(c.core.BytesWritten() >= 2000);
	CHECK(c.Has("reached its size limit"));
	const size_t full = c.sink.text.size();
	c.W(kLogInfo, "steam", "dropped %d", 1);
	c.W(kLogError, "steam", "dropped %d", 2);
	CHECK_EQ(c.sink.text.size(), full);
	c.W(kLogFatal, "crash", "CRASH at %08x", 0x1234u);
	CHECK(c.Has("F crash: CRASH at 00001234"));

	Harness d;
	d.Configure(kLogDebug, 1000);
	size_t i = 0;
	while (d.core.BytesWritten() < 820) d.W(kLogInfo, "steam", "information line number %zu", i++);
	d.W(kLogDebug, "steam", "debug after 80 percent");
	CHECK(!d.Has("debug after 80 percent"));
	d.W(kLogInfo, "steam", "info still goes");
	CHECK(d.Has("info still goes"));
}

void Fatal()
{
	Harness h;
	h.W(kLogDebug, "net", "context line");
	h.W(kLogFatal, "crash", "CRASH %08lx", 0xC0000005ul);
	CHECK(h.Has("context before this point (fatal error)"));
	CHECK(h.Has("context line"));
	CHECK(h.Has("F crash: CRASH c0000005"));
	CHECK_EQ(h.core.Count(kLogFatal), 1);
}

void Legacy()
{
	Harness h;
	h.Legacy("Steam: online ready as %s", "THIONG");
	CHECK(h.Has("I steam: online ready as THIONG\n"));
	h.Legacy("Patch (%s): %s applied", "mods\\a", "x.xml");
	CHECK(h.Has("I patch: (mods\\a) x.xml applied\n"));
	h.Legacy("Game room: closed");
	CHECK(h.Has("I game-room: closed\n"));
	h.Legacy("Alt+Enter: %s", "windowed");
	CHECK(h.Has("I alt-enter: windowed\n"));
	h.Legacy("%s registers: eax=%08lx", "HANG", 1ul);
	CHECK(h.Has("I log: HANG registers: eax=00000001\n"));
	h.Legacy("  %ldx%ld", 1920l, 1080l);
	CHECK(h.Has("I log:   1920x1080\n"));
	h.Legacy("%s: dynamic prefix", "Steam");
	CHECK(h.Has("I log: Steam: dynamic prefix\n"));
	h.Legacy("just a sentence without a prefix");
	CHECK(h.Has("I log: just a sentence without a prefix\n"));
	h.Legacy("DESYNC at tick %u: my fingerprint %08x", 118u, 0xabcdu);
	CHECK(h.Has("E match: DESYNC at tick 118: my fingerprint 0000abcd\n"));
	h.Legacy("HANG: the main thread has not responded");
	CHECK(h.Has("E crash: HANG: the main thread has not responded\n"));
	h.Legacy("CRASH %08lx at %08lx", 1ul, 2ul);
	CHECK(h.Has("F crash: CRASH 00000001 at 00000002\n"));
	CHECK_EQ(h.core.Count(kLogError), 2);

	const LegacyLine q = ParseLegacyLine("Spectator (self-test): replay", "Spectator (self-test): replay");
	CHECK_EQ(q.channel, "spectator");
	CHECK_EQ(q.qualifier, "self-test");
	const LegacyLine none = ParseLegacyLine("x: y", "x: y");
	CHECK_EQ(none.channel, "log");
}

void Helpers()
{
	Harness h;
	int key = 0;
	CHECK(h.core.Due(&key, 1000));
	CHECK(!h.core.Due(&key, 1000));
	h.now.ms += 500;
	CHECK(!h.core.Due(&key, 1000));
	h.now.second += 1;
	CHECK(h.core.Due(&key, 1000));
	CHECK(h.core.FirstTime(&key));
	CHECK(!h.core.FirstTime(&key));
	int other = 0;
	CHECK(h.core.FirstTime(&other));
}

void Midnight()
{
	Harness h;
	h.W(kLogInfo, "boot", "before");
	h.now.day = 17;
	h.now.hour = 0;
	h.now.minute = 0;
	h.W(kLogInfo, "boot", "after");
	const size_t clock = h.At("I clock: date is now 2026-10-17");
	CHECK(clock != std::string::npos);
	CHECK(clock < h.At("after"));
	CHECK(h.At("before") < clock);
}

void Counts()
{
	Harness h;
	h.W(kLogWarn, "a", "w1");
	h.W(kLogWarn, "a", "w2");
	h.W(kLogDebug, "a", "hidden");
	h.W(kLogError, "a", "e1");
	CHECK_EQ(h.core.Count(kLogWarn), 2);
	CHECK_EQ(h.core.Count(kLogError), 1);
	CHECK_EQ(h.core.Count(kLogDebug), 1);
}

}

int main()
{
	Format();
	Levels();
	Parsing();
	Context();
	Ring();
	Flood();
	Repeats();
	Limits();
	Fatal();
	Legacy();
	Helpers();
	Midnight();
	Counts();
	return test::Summary("log");
}
