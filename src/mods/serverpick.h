// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct Json;

struct FileServer {
	std::string host;
	double weight = 1;
};

bool IsGbFileServer(const std::string &host);
int GbFileServerNumber(const std::string &host);
std::string GbFileServerHost(int number);
std::vector<FileServer> ParseFileservers(const Json &response);
std::vector<std::string> RendezvousOrder(const std::string &path, const std::vector<FileServer> &servers);
const size_t kRecentDownloads = 5;
std::vector<double> RememberRate(std::vector<double> recent, double bestThisDownload);
double ConnectionRate(const std::vector<double> &recent);

class ServerPicker {
public:
	enum Step { kStay, kTry, kKeepNew, kBack };

	static constexpr double kClearlyFaster = 1.5;
	static constexpr double kDropped = 0.5;
	static constexpr int kMaxTries = 3;

	ServerPicker(const std::string &assigned, double connectionRate);

	void SetCandidates(const std::vector<std::string> &ordered);
	bool CandidatesKnown() const { return m_known; }
	bool WantsCandidates(double rate) const;

	const std::string &Current() const { return m_current; }
	const std::string &Best() const { return m_best; }
	double BestRate() const { return m_bestRate; }
	double FastestSeen() const { return m_fastest; }
	bool RateWorthRemembering() const;
	bool Testing() const { return m_state == kTesting; }

	Step OnWindow(double rate);
	Step OnFailure();
	void NoteRate(const std::string &server, double rate);

private:
	enum State { kMeasuring, kTesting, kSettled };

	bool ShouldTry(double rate) const;
	bool NextCandidate(std::string &out);
	Step Try();

	State m_state = kMeasuring;
	std::string m_current, m_best;
	double m_bestRate = 0, m_fastest = 0, m_connection = 0;
	std::vector<std::string> m_candidates, m_measured;
	size_t m_next = 0;
	int m_tries = 0;
	bool m_known = false;
};
