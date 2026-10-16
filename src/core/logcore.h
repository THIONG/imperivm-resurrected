// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "core/log.h"

struct LogTime {
	int year = 1970;
	int month = 1;
	int day = 1;
	int hour = 0;
	int minute = 0;
	int second = 0;
	int ms = 0;
};

class LogSink {
public:
	virtual ~LogSink() {}
	virtual void Write(const char *data, size_t size) = 0;
};

struct LogSettings {
	LogLevel level = kLogInfo;
	std::map<std::string, LogLevel> channels;
	size_t maxFileBytes = 16u << 20;
	int floodPerSecond = 100;
};

struct LegacyLine {
	std::string channel;
	LogLevel level = kLogInfo;
	size_t skip = 0;
	std::string qualifier;
};

const size_t kLogMaxLine = 1024;

bool ParseLogLevel(const std::string &name, LogLevel &out);
std::map<std::string, LogLevel> ParseLogChannels(const std::string &text);
char LogLevelLetter(LogLevel level);
std::map<std::string, LogLevel> BetaLogChannels();
bool IsBetaVersion(const std::string &version);
LegacyLine ParseLegacyLine(const char *fmt, const char *text);

class LogCore {
public:
	LogCore(LogSink *sink, std::function<LogTime()> clock, std::function<unsigned()> threadId, unsigned mainThread);

	void Configure(const LogSettings &settings);
	bool Enabled(LogLevel level, const char *channel);

	void Write(LogLevel level, const char *channel, const void *key, const char *fmt, va_list args);
	void WriteLegacy(const char *fmt, va_list args);
	bool Due(const void *key, unsigned intervalMs);
	bool FirstTime(const void *key);
	void DumpContext(const char *reason);
	void Finish(const char *channel, const char *message);

	size_t BytesWritten();
	int Count(LogLevel level);

private:
	struct Slot {
		char text[232];
		uint16_t size;
		bool written;
	};
	struct Flood {
		const void *key = nullptr;
		int64_t window = 0;
		int count = 0;
		int suppressed = 0;
		char channel[24] = {};
		LogLevel level = kLogInfo;
	};

	void WriteLocked(LogLevel level, const char *channel, const void *key, const char *message, size_t length);
	void WriteFatal(const char *channel, const char *fmt, va_list args);
	size_t FormatLine(char *out, size_t cap, const LogTime &t, LogLevel level, const char *channel, const char *message, size_t length);
	void EmitRaw(const char *data, size_t size);
	void EmitNote(LogLevel level, const char *channel, const char *format, ...);
	void FlushRepeatLocked();
	void FlushFloodLocked();
	void DumpLocked(const char *reason, bool skipNewest);
	LogLevel EffectiveLevel(const char *channel) const;
	int64_t NowMs(const LogTime &t) const;

	LogSink *sink_;
	std::function<LogTime()> clock_;
	std::function<unsigned()> threadId_;
	unsigned mainThread_;
	LogSettings settings_;
	std::recursive_timed_mutex mutex_;
	std::vector<Slot> ring_;
	size_t ringNext_ = 0;
	size_t ringCount_ = 0;
	size_t bytes_ = 0;
	int counts_[6] = {};
	int lastDay_ = 0;
	int dumps_ = 0;
	bool capNoticed_ = false;
	std::string lastKind_;
	int repeat_ = 0;
	std::vector<Flood> flood_;
	std::map<const void *, int64_t> every_;
	std::set<const void *> once_;
};
